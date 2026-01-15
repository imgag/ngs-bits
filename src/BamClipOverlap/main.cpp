#include "Exceptions.h"
#include "ToolBase.h"
#include "Helper.h"
#include <QTextStream>
#include "NGSHelper.h"
#include "BamWriter.h"
#include <QHash>

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Softclipping of overlapping reads.");
		setExtendedDescription(QStringList()	<<	"Overlapping reads will be soft-clipped from start to end. " \
													"There are several parameters available for handling of mismatches in overlapping reads. " \
													"Within the overlap the higher base quality will be kept for each basepair."
							   );
		addInfile("in", "Input BAM/CRAM file. Needs to be sorted by name.", false);
		addOutfile("out", "Output BAM file.", false);
		//optional
		addFlag("overlap_mismatch_mapq", "Set mapping quality of pair to 0 if mismatch is found in overlapping reads.");
		addFlag("overlap_mismatch_remove", "Remove pair if mismatch is found in overlapping reads.");
		addFlag("overlap_mismatch_baseq", "Reduce base quality if mismatch is found in overlapping reads.");
		addFlag("overlap_mismatch_basen", "Set base to N if mismatch is found in overlapping reads.");
		addFlag("ignore_indels","Turn off indel detection in overlap.");
		addFlag("v", "Verbose mode.");
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);

		//changelog
		changeLog(2020,  11, 27, "Added CRAM support.");
		changeLog(2018,01,11,"Updated base quality handling within overlap.");
		changeLog(2017,01,16,"Added overlap mismatch filter.");
	}

	virtual void main()
	{
		//step 1: init
		int reads_count = 0;
		int reads_saved = 0;
		int reads_clipped = 0;
		int reads_mismatch = 0;
		quint64 bases_count = 0;
		quint64 bases_clipped = 0;
		QTextStream out(stderr);
		bool verbose = getFlag("v");
		bool ignore_indels = getFlag("ignore_indels");
		BamReader reader(getInfile("in"), getInfile("ref"));
		BamWriter writer(getOutfile("out"), getInfile("ref"));
		writer.writeHeader(reader);

		//step 2: get alignments and softclip if necessary
		BamAlignment al;
		QHash<QByteArray, BamAlignment> al_map;
		while (reader.getNextAlignment(al))
		{
			++reads_count;
			bases_count += al.length();
			bool skip_al = false;

			//check preconditions and if unmet save read to out and continue
			if(!al.isPaired() || al.isSecondaryAlignment() || al.isSupplementaryAlignment())
			{
				writer.writeAlignment(al);
				++reads_saved;
				continue;
			}
			if(al.isUnmapped() || al.isMateUnmapped())	// only mapped read pairs
			{
				writer.writeAlignment(al);
				++reads_saved;
				continue;
			}
			if(al.chromosomeID()!=al.mateChrosomeID())	// different chromosomes
			{
				writer.writeAlignment(al);
				++reads_saved;
				continue;
			}
			if(al.cigarIsOnlyInsertion())	// only reads with valid CIGAR data
			{
				writer.writeAlignment(al);
				++reads_saved;
				continue;
			}

			if(al_map.contains(al.name()))
			{
				BamAlignment mate = al_map.take(al.name());

				//check if reads are on different strands
				BamAlignment forward_read = mate;
				BamAlignment reverse_read = al;
				bool both_strands = false;
				if(forward_read.isReverseStrand()!=reverse_read.isReverseStrand())
				{
					both_strands = true;
					if(!reverse_read.isReverseStrand())
					{
						BamAlignment tmp_read = forward_read;
						forward_read = reverse_read;
						reverse_read = tmp_read;
					}
				}

				//check if reads overlap
				int s1 = forward_read.start();
				int e1 = forward_read.end();
				int s2 = reverse_read.start();
				int e2 = reverse_read.end();

				//check if reads overlap
				bool soft_clip = false;
				if(forward_read.chromosomeID()==reverse_read.chromosomeID())	// same chromosome
				{
					if(s1>=s2 && s1<=e2)	soft_clip = true;	// start read1 within read2
					else if(e1>=s2 && e1<=e2)	soft_clip = true;	// end read1 within read2
					else if(s1<=s2 && e1>=e2)	soft_clip = true;	// start and end read1 outisde of read2
				}

				//soft-clip overlapping reads
				if(soft_clip)
				{
					int clip_forward_read = 0;
					int clip_reverse_read = 0;
					int overlap = 0;
					int overlap_start = 0;
					int overlap_end = 0;

					if(s1<=s2 && e1<=e2)	// forward read left of reverse read
					{
						overlap = forward_read.end()-reverse_read.start()+1;
						overlap_start  = reverse_read.start()-1;
						overlap_end = forward_read.end();
						clip_forward_read = static_cast<int>(overlap/2);
						clip_reverse_read = static_cast<int>(overlap/2);
						if(forward_read.isRead1())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(s1>s2 && e1>e2)	// forward read right of reverse read
					{
						overlap = reverse_read.end()-forward_read.start()+1;
						overlap_start  = forward_read.start()-1;
						overlap_end = reverse_read.end();
						clip_forward_read = static_cast<int>(overlap/2) + (forward_read.end()-reverse_read.end());
						clip_reverse_read = static_cast<int>(overlap/2) + (forward_read.start()-reverse_read.start());
						if(forward_read.isRead1())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(both_strands==true && s1>=s2 && e1<=e2)	// forward read within reverse read
					{
						overlap = forward_read.end()-forward_read.start()+1;
						overlap_start  = forward_read.start()-1;
						overlap_end = forward_read.end();
						clip_forward_read = static_cast<int>(overlap/2);
						clip_reverse_read = static_cast<int>(overlap/2) + (forward_read.start()-reverse_read.start());
						if(forward_read.isRead1())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(both_strands==true && s1<=s2 && e1>=e2)	//reverse read within forward read
					{
						overlap = reverse_read.end()-reverse_read.start()+1;
						overlap_start  = reverse_read.start()-1;
						overlap_end = reverse_read.end();
						clip_forward_read = static_cast<int>(overlap/2) + (forward_read.end()-reverse_read.end());
						clip_reverse_read = static_cast<int>(overlap/2);
						if(forward_read.isRead1())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(both_strands==false && s1>=s2 && e1<=e2)	//forward read lies completely within reverse read
					{
						overlap = forward_read.end()-forward_read.start()+1;
						overlap_start  = forward_read.start()-1;
						overlap_end = forward_read.end();
						clip_forward_read = overlap;
						clip_reverse_read = 0;
					}
					else if(both_strands==false && s1<=s2 && e1>=e2)	//reverse read lies completely within foward read
					{
						overlap = reverse_read.end()-reverse_read.start()+1;
						overlap_start  = reverse_read.start()-1;
						overlap_end = reverse_read.end() ;
						clip_forward_read = 0;
						clip_reverse_read = overlap;
					}
					else
					{
						if(both_strands)
						{
							THROW(Exception, "Read orientation of forward read " + forward_read.name() + " ("+reader.chromosome(forward_read.chromosomeID()).str()+":"+QString::number(forward_read.start())+"-"+QString::number(forward_read.end())+") and reverse read "+reverse_read.name()+" ("+reader.chromosome(reverse_read.chromosomeID()).str()+":"+QString::number(reverse_read.start())+"-"+QString::number(reverse_read.end())+") was not identified.");
						}
						else
						{
							THROW(Exception, "Read orientation of read1 " + forward_read.name() + " ("+reader.chromosome(forward_read.chromosomeID()).str()+":"+QString::number(forward_read.start())+"-"+QString::number(forward_read.end())+") and read2 "+reverse_read.name()+" ("+reader.chromosome(reverse_read.chromosomeID()).str()+":"+QString::number(reverse_read.start())+"-"+QString::number(reverse_read.end())+") was not identified.");
						}
					}

					//verbose mode
                    if(verbose)	out << "forward read: name - " << forward_read.name() << ", region - " << reader.chromosome(forward_read.chromosomeID()).str() << ":" << (forward_read.start()-1) << "-" << forward_read.end() << ", insert size: "  << forward_read.insertSize() << " bp; mate: " << forward_read.mateStart() << ", CIGAR " << forward_read.cigarDataAsString() << ", overlap: " << overlap << " bp" << Qt::endl;
                    if(verbose)	out << "reverse read: name - " << reverse_read.name() << ", region - " << reader.chromosome(reverse_read.chromosomeID()).str() << ":" << (reverse_read.start()-1) << "-" << reverse_read.end() << ", insert size: "  << reverse_read.insertSize() << " bp; mate: " << reverse_read.mateStart() << ", CIGAR " << reverse_read.cigarDataAsString() << ", overlap: " << overlap << " bp" << Qt::endl;
                    if(verbose) out << "forward read bases " << forward_read.bases() << Qt::endl;
                    if(verbose) out << "forward read qualities " << forward_read.qualities() << Qt::endl;
                    if(verbose) out << "forward CIGAR " << forward_read.cigarDataAsString(true) << Qt::endl;
                    if(verbose) out << "reverse read bases " << reverse_read.bases() << Qt::endl;
                    if(verbose) out << "reverse read qualities " << reverse_read.qualities() << Qt::endl;
                    if(verbose) out << "reverse CIGAR " << reverse_read.cigarDataAsString(true) << Qt::endl;
                    if(verbose)	out << "  clip forward read from position " << (forward_read.end()-clip_forward_read+1) << " to " << forward_read.end() << Qt::endl;
                    if(verbose)	out << "  clip reverse read from position " << reverse_read.start() << " to " << (reverse_read.start()-1+clip_reverse_read) << Qt::endl;

					struct Overlap
					{
						QList<int> genome_pos;
						QList<int> read_pos;
						QList<char> base;
						QList<char> quality;
						QList<char> cigar;

						void append(char base, char cigar, char quality, int genome_pos, int read_pos)
						{
							this->base.append(base);
							this->cigar.append(cigar);
							this->quality.append(quality);
							this->genome_pos.append(genome_pos);
							this->read_pos.append(read_pos);
						}

						void insert(int at, char base, char cigar, char quality, int genome_pos, int read_pos)
						{
							this->base.insert(at, base);
							this->cigar.insert(at, cigar);
							this->quality.insert(at, quality);
							this->genome_pos.insert(at, genome_pos);
							this->read_pos.insert(at, read_pos);
						}

						QByteArray getBases() const
						{
							QByteArray output;
							for(int i=0; i<base.length(); ++i)
							{
								output.append(base[i]);
							}
							return output;
						}

						QByteArray getCigar() const
						{
							QByteArray output;
							for(int i=0; i<cigar.length(); ++i)
							{
								output.append(cigar[i]);
							}
							return output;
						}

						int length() const
						{
							if(read_pos.length()!=cigar.length()) THROW(Exception,"Lengths differ.");
							return read_pos.length();
						}
					};

					//check if bases in overlap match
                    if(verbose)	out << "  overlap found from " << QString::number(overlap_start) << " to " << QString::number(overlap_end) << Qt::endl;

					//
					bool has_indel = false; //INDEL ist around the clipping position
					int surrounding_nuc = 5;


					int genome_pos = forward_read.start()-1;
					int read_pos = 0;
					int clip_position = forward_read.end() - clip_forward_read;
					Overlap forward_overlap;
					QByteArray forward_bases = forward_read.bases();
					QByteArray forward_qualities = forward_read.qualities();
					QByteArray forward_cigar = forward_read.cigarDataAsString(true);
					for(int i = 0;i<forward_cigar.length();++i)
					{
						if(genome_pos>=overlap_start && genome_pos<overlap_end && forward_cigar[i]!='H' && forward_cigar[i]!='S')
						{
							char current_base = forward_bases[read_pos];
							char current_quality = forward_qualities[read_pos];
							if(forward_cigar[i]=='D')	current_base = '-';
							forward_overlap.append(current_base, forward_cigar[i], current_quality, genome_pos, read_pos);
						}

						if(!ignore_indels && genome_pos>(clip_position-surrounding_nuc) && genome_pos<(clip_position+surrounding_nuc))
						{
							if(forward_cigar[i]=='I' || forward_cigar[i]=='D')
							{
								has_indel = true;
							}
						}

						if(forward_cigar[i]=='H')	continue;
						else if(forward_cigar[i]=='S')	++read_pos;
						else if(forward_cigar[i]=='M')
						{
							++genome_pos;
							++read_pos;
						}
						else if(forward_cigar[i]=='D')
						{
							++genome_pos;
						}
						else if(forward_cigar[i]=='I')
						{
							++read_pos;
						}
						else
						{
							THROW(Exception, QByteArray("Unknown CIGAR character '") + forward_cigar[i] + "'")
						}
					}
                    if(verbose)	out << "  finished reading overlap forward bases " << forward_overlap.getBases() << Qt::endl;
                    if(verbose)	out << "  finished reading overlap forward cigar " << forward_overlap.getCigar() << Qt::endl;

					genome_pos = reverse_read.start()-1;
					read_pos = 0;
					clip_position = reverse_read.start() -1 + clip_reverse_read;
					Overlap reverse_overlap;
					QByteArray reverse_bases = reverse_read.bases();
					QByteArray reverse_qualities = reverse_read.qualities();
					QByteArray reverse_cigar = reverse_read.cigarDataAsString(true);
					for(int i=0; i<reverse_cigar.length();++i)
					{
						if(genome_pos>=overlap_start && genome_pos<overlap_end && reverse_cigar[i]!='H' && reverse_cigar[i]!='S')
						{
							char current_base = reverse_bases[read_pos];
							char current_quality = reverse_qualities[read_pos];
							if(reverse_cigar[i]=='D')	current_base = '-';
							reverse_overlap.append(current_base, reverse_cigar[i], current_quality, genome_pos, read_pos);
						}

						if(!ignore_indels && genome_pos>(clip_position-surrounding_nuc) && genome_pos<(clip_position+surrounding_nuc))
						{
							if(reverse_cigar[i]=='I' || reverse_cigar[i]=='D')
							{
								has_indel = true;
							}
						}

						if(reverse_cigar[i]=='H')	continue;
						else if(reverse_cigar[i]=='S')	++read_pos;
						else if(reverse_cigar[i]=='M')
						{
							++genome_pos;
							++read_pos;
						}
						else if(reverse_cigar[i]=='D')
						{
							++genome_pos;
						}
						else if(reverse_cigar[i]=='I')
						{
							++read_pos;
						}
						else
						{
							THROW(Exception, QByteArray("Unknown CIGAR character '") + reverse_cigar[i] + "'");
						}
					}
                    if(verbose)	out << "  finished reading overlap reverse bases " << reverse_overlap.getBases() << Qt::endl;
                    if(verbose)	out << "  finished reading overlap reverse cigar " << reverse_overlap.getCigar() << Qt::endl;

					//correct for insertions
					for(int i=0;i<forward_overlap.length();++i)
					{
						if(forward_overlap.cigar[i]!=reverse_overlap.cigar[i] && forward_overlap.cigar[i]=='I' && forward_overlap.base[i]!='+')
						{
							reverse_overlap.insert(i, '+', 'I', '0', reverse_overlap.genome_pos[i], reverse_overlap.read_pos[i]);
						}
						
						if(forward_overlap.cigar[i]!=reverse_overlap.cigar[i] && reverse_overlap.cigar[i]=='I' && reverse_overlap.base[i]!='+')
						{
							forward_overlap.insert(i, '+', 'I', '0', forward_overlap.genome_pos[i], forward_overlap.read_pos[i]);
						}
					}
                    if(verbose)	out << "  finished indel correction forward bases " << forward_overlap.getBases() << Qt::endl;
                    if(verbose)	out << "  finished indel correction forward cigar " << forward_overlap.getCigar() << Qt::endl;
                    if(verbose)	out << "  finished indel correction reverse bases " << reverse_overlap.getBases() << Qt::endl;
                    if(verbose)	out << "  finished indel correction reverse cigar " << reverse_overlap.getCigar() << Qt::endl;
					if(forward_overlap.length()!=reverse_overlap.length()) //both cigar and base string should now be equally long
					{
						THROW(Exception, "Length mismatch between forward/reverse overlap - forward:" + QByteArray::number(forward_overlap.length()) + " reverse:" + QByteArray::number(reverse_overlap.length()) + " in read with name '" + al.name() + "'");
					}

					//detect mismtaches(read pos for, read pos rev)
					QList<QPair<int,int>> mm_pos;
					for(int i=0;i<forward_overlap.length();++i)
					{
						if(forward_overlap.base[i]!=reverse_overlap.base[i])
						{
							int first = forward_overlap.read_pos[i];
							int second = reverse_overlap.read_pos[i];
							if(forward_overlap.base[i]=='-' || forward_overlap.base[i]=='+')	first = -1;
							if(reverse_overlap.base[i]=='-' || reverse_overlap.base[i]=='+')	second = -1;
							mm_pos.append(qMakePair(first,second));
						}
					}

					if(verbose && !mm_pos.isEmpty())
					{
                        out << "  overlap mismatch for read pair " << forward_read.name() << " - " << forward_overlap.getBases() << " != " << reverse_overlap.getBases() << "!" << Qt::endl;
					}

					bool map = getFlag("overlap_mismatch_mapq");
					bool rem = getFlag("overlap_mismatch_remove");
					bool base = getFlag("overlap_mismatch_baseq");
					bool basen = getFlag("overlap_mismatch_basen");
					if(base || rem || map || basen)
					{
						if(!mm_pos.isEmpty() && map)
						{
							forward_read.setMappingQuality(0);
							reverse_read.setMappingQuality(0);
							reads_mismatch += 2;
                            if(verbose) out << "  Set mapping quality to 0." << Qt::endl;
						}
						else if(!mm_pos.isEmpty() && rem)
						{
							reads_mismatch += 2;
							skip_al = true;
                            if(verbose) out << "   Removed pair." << Qt::endl;
						}
						else if(!mm_pos.isEmpty() && base)
						{
							reads_mismatch += 2;
							QByteArray orig_for = forward_read.qualities();
							QByteArray orig_rev = reverse_read.qualities();
							QByteArray new_for = orig_for;
							QByteArray new_rev = orig_rev;

							//set base quality for change qualities
							for(int i=0;i<mm_pos.length();++i)
							{
								if(mm_pos[i].first>=0)	new_for[mm_pos[i].first] = '!';
								if(mm_pos[i].second>=0)	new_rev[mm_pos[i].second] = '!';
							}
							forward_read.setQualities(new_for);
							reverse_read.setQualities(new_rev);
                            if(verbose) out << "   changed forward base qualities from " << orig_for << " to " << forward_read.qualities() << Qt::endl;
                            if(verbose) out << "   changed reverse base qualities from " << orig_rev << " to " << reverse_read.qualities() << Qt::endl;
						}
						else if(!mm_pos.isEmpty() && basen)
						{
							reads_mismatch += 2;
							QByteArray orig_for = forward_read.bases();
							QByteArray orig_rev = reverse_read.bases();
							QByteArray new_for = orig_for;
							QByteArray new_rev = orig_rev;

							//set Ns for mismatch bases
							for(int i=0;i<mm_pos.length();++i)
							{
								if(mm_pos[i].first>=0)	new_for[mm_pos[i].first] = 'N';
								if(mm_pos[i].second>=0)	new_rev[mm_pos[i].second] = 'N';
							}
							forward_read.setBases(new_for);
							reverse_read.setBases(new_rev);
                            if(verbose) out << "   changed forward sequences from " << orig_for << " to " << forward_read.bases() << Qt::endl;
                            if(verbose) out << "   changed reverse sequences from " << orig_rev << " to " << reverse_read.bases() << Qt::endl;
						}
						else
						{
                            if(verbose)	out << "  no overlap mismatch for read pair " << forward_read.name() << Qt::endl;
						}
					}

					//try to avoid soft-clipping indels in overlap
					if(has_indel)
					{
						if(reads_clipped%4==0)
						{
							clip_forward_read = 0;
							clip_reverse_read = overlap;
						}
						else
						{
							clip_forward_read = overlap;
							clip_reverse_read = 0;
						}
					}

					//actual soft clipping
					if(clip_forward_read>0)	NGSHelper::softClipAlignment(forward_read,(forward_read.end()-clip_forward_read+1),forward_read.end());
					if(clip_reverse_read>0)	NGSHelper::softClipAlignment(reverse_read,reverse_read.start(),(reverse_read.start()-1+clip_reverse_read));

					//set new insert size and mate position
					int forward_end = forward_read.end();
					int reverse_end = reverse_read.end();

					if(reverse_read.start() == reverse_read.end())
					{
						reverse_end -= 1;
					}
					if(forward_read.start() == forward_read.end())
					{
						forward_end -= 1;
					}

					int forward_insert_size = reverse_end-forward_read.start()+1;
					int reverse_insert_size = forward_read.start()-reverse_end-1;

					//qDebug() << "START ENDS: " << forward_read.start() <<  forward_read.end() << reverse_read.start() << reverse_read.end() << "\n";

					forward_read.setInsertSize(forward_insert_size);	//positive value
					forward_read.setMateStart(reverse_read.start());
					reverse_read.setInsertSize(reverse_insert_size);	//negative value
					reverse_read.setMateStart(forward_read.start());

                    if(verbose)	out << "  clipped forward read: name - " << forward_read.name() << ", region - " << reader.chromosome(forward_read.chromosomeID()).str() << ":" << (forward_read.start()-1) << "-" << forward_end << ", insert size: "  << forward_read.insertSize() << " bp; mate: " << forward_read.mateStart() << ", CIGAR " << forward_read.cigarDataAsString() << ", overlap: " << overlap << " bp" << Qt::endl;
                    if(verbose)	out << "  clipped reverse read: name - " << reverse_read.name() << ", region - " << reader.chromosome(reverse_read.chromosomeID()).str()  << ":" << (reverse_read.start()-1) << "-" << reverse_end << ", insert size: "  << reverse_read.insertSize() << " bp; mate: " << reverse_read.mateStart() << ", CIGAR " << reverse_read.cigarDataAsString() << ", overlap: " << overlap << " bp" << Qt::endl;
                    if(verbose)	out << Qt::endl;

					//return reads
					bases_clipped += overlap;
					reads_clipped += 2;
				}


				//save reads
				reads_saved+=2;
				if(skip_al)	continue;
				writer.writeAlignment(forward_read);
				writer.writeAlignment(reverse_read);
			}
			else    //keep in map
			{
				al_map.insert(al.name(), al);
			}
		}

		//step 3: save all remaining reads
		foreach(const BamAlignment& al, al_map)
		{
			writer.writeAlignment(al);
			++reads_saved;
		}

		//step 4: write out statistics
		if(reads_saved!=reads_count)	THROW(ToolFailedException, "Lost Reads: "+QString::number(reads_count-reads_saved)+"/"+QString::number(reads_count));
        out << "Overlap mismatch filtering was used for " << QString::number(reads_mismatch) << " of " << QString::number(reads_count) << " reads (" << QString::number((double)reads_mismatch/(double)reads_count*100,'f',2) << " %)." << Qt::endl;
        out << "Softclipped " << QString::number(reads_clipped) << " of " << QString::number(reads_count) << " reads (" << QString::number(((double)reads_clipped/(double)reads_count*100),'f',2) << " %)." << Qt::endl;
        out << "Softclipped " << QString::number(bases_clipped) << " of " << QString::number(bases_count) << " basepairs (" << QString::number((double)bases_clipped/(double)bases_count*100,'f',2) << " %)." << Qt::endl;
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
