#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BasicStatistics.h"
#include "Helper.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QSet>
#include "NGSHelper.h"
#include "api/BamReader.h"
#include "api/BamWriter.h"
#include "api/BamAlgorithms.h"
#include <QHash>

using namespace BamTools;


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
		setExtendedDescription(QStringList()	<< "Overlapping reads will be soft-clipped from start to end. "
												<< "If mismatches are found in the overlap of both reads, the mapping quality of both reads will be set to zero."
							   );
		addInfile("in", "Input bam file. Needs to be sorted by name.", false);
		addOutfile("out", "Output bam file.", false);
		//optional
		addFlag("amplicon", "Amplicon mode: one read of a pair will be clipped randomly.");
		addFlag("overlap_mismatch_mapq", "Set mapping quality of pair to 0 if mismatch is found in overlapping reads.");
		addFlag("overlap_mismatch_remove", "Remove pair if mismatch is found in overlapping reads.");
		addFlag("overlap_mismatch_baseq", "Reduce base quality if mismatch is found in overlapping reads.");
		addFlag("v", "Verbose mode.");

		//changelog
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
		BamReader reader;
		NGSHelper::openBAM(reader, getInfile("in"));
		BamWriter writer;
		writer.Open(getOutfile("out").toStdString(), reader.GetConstSamHeader(), reader.GetReferenceData());

		//step 2: get alignments and softclip if necessary
		BamAlignment al;
		QHash<QString, BamAlignment> al_map;
		while (reader.GetNextAlignment(al))
		{
			++reads_count;
			bases_count += al.Length;
			bool skip_al = false;

			//check preconditions and if unmet save read to out and continue
			if(!al.IsPaired() || !al.IsPrimaryAlignment())
			{
				writer.SaveAlignment(al);
				++reads_saved;
				continue;
			}
			if(!al.IsMapped() && !al.IsMateMapped())	// only mapped reads
			{
				writer.SaveAlignment(al);
				++reads_saved;
				continue;
			}
			if(al.RefID!=al.MateRefID)	// different chromosomes
			{
				writer.SaveAlignment(al);
				++reads_saved;
				continue;
			}
			if(al.CigarData.empty())	// only with CIGAR data
			{
				writer.SaveAlignment(al);
				++reads_saved;
				continue;
			}

			if(al_map.contains(QString::fromStdString(al.Name)))
			{
				BamAlignment mate;
				mate = al_map.take(QString::fromStdString(al.Name));

				//check if reads are on different strands
				BamAlignment forward_read = mate;
				BamAlignment reverse_read = al;
				bool both_strands = false;
				if(forward_read.IsReverseStrand()!=reverse_read.IsReverseStrand())
				{
					both_strands = true;
					if(!reverse_read.IsReverseStrand())
					{
						BamAlignment tmp_read = forward_read;
						forward_read = reverse_read;
						reverse_read = tmp_read;
					}
				}
				int s1 = forward_read.Position+1;
				int e1 = forward_read.GetEndPosition();
				int s2 = reverse_read.Position+1;
				int e2 = reverse_read.GetEndPosition();

				//check if reads overlap
				bool soft_clip = false;
				if(forward_read.RefID==reverse_read.RefID)	// same chromosome
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
						overlap = forward_read.GetEndPosition()-reverse_read.Position;
						overlap_start  = reverse_read.Position;
						overlap_end = forward_read.GetEndPosition();
						clip_forward_read = static_cast<int>(overlap/2);
						clip_reverse_read = static_cast<int>(overlap/2);
						if(forward_read.IsFirstMate())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(s1>s2 && e1>e2)	// forward read right of reverse read
					{
						overlap = reverse_read.GetEndPosition()-forward_read.Position;
						overlap_start  = forward_read.Position;
						overlap_end = reverse_read.GetEndPosition();
						clip_forward_read = static_cast<int>(overlap/2) + (forward_read.GetEndPosition()-reverse_read.GetEndPosition());
						clip_reverse_read = static_cast<int>(overlap/2) + (forward_read.Position-reverse_read.Position);
						if(forward_read.IsFirstMate())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(both_strands==true && s1>=s2 && e1<=e2)	// forward read within reverse read
					{
						overlap = forward_read.GetEndPosition()-forward_read.Position;
						overlap_start  = forward_read.GetEndPosition();
						overlap_end = forward_read.Position;
						clip_forward_read = static_cast<int>(overlap/2);
						clip_reverse_read = static_cast<int>(overlap/2) + (forward_read.Position-reverse_read.Position);
						if(forward_read.IsFirstMate())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(both_strands==true && s1<=s2 && e1>=e2)	//reverse read within forward read
					{
						overlap = reverse_read.GetEndPosition()-reverse_read.Position;
						overlap_start  = reverse_read.GetEndPosition();
						overlap_end = reverse_read.Position;
						clip_forward_read = static_cast<int>(overlap/2) + (forward_read.GetEndPosition()-reverse_read.GetEndPosition());
						clip_reverse_read = static_cast<int>(overlap/2);
						if(forward_read.IsFirstMate())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(both_strands==false && s1>=s2 && e1<=e2)	//forward read lies completely within reverse read
					{
						overlap = forward_read.GetEndPosition()-forward_read.Position;
						overlap_start  = forward_read.GetEndPosition();
						overlap_end = forward_read.Position;
						clip_forward_read = overlap;
						clip_reverse_read = 0;
					}
					else if(both_strands==false && s1<=s2 && e1>=e2)	//reverse read lies completely within foward read
					{
						overlap = reverse_read.GetEndPosition()-reverse_read.Position;
						overlap_start  = reverse_read.GetEndPosition();
						overlap_end = reverse_read.Position;
						clip_forward_read = 0;
						clip_reverse_read = overlap;
					}
					else
					{
						if(both_strands)
						{
							THROW(Exception, "Read orientation of forward read "+QString::fromStdString(forward_read.Name)+" (chr"+QString::number(forward_read.RefID)+":"+QString::number(forward_read.Position)+"-"+QString::number(forward_read.GetEndPosition())+") and reverse read "+QString::fromStdString(reverse_read.Name)+" (chr"+QString::number(reverse_read.RefID)+":"+QString::number(reverse_read.Position)+"-"+QString::number(reverse_read.GetEndPosition())+") was not identified.");
						}
						else
						{
							THROW(Exception, "Read orientation of read1 "+QString::fromStdString(forward_read.Name)+" (chr"+QString::number(forward_read.RefID)+":"+QString::number(forward_read.Position)+"-"+QString::number(forward_read.GetEndPosition())+") and read2 "+QString::fromStdString(reverse_read.Name)+" (chr"+QString::number(reverse_read.RefID)+":"+QString::number(reverse_read.Position)+"-"+QString::number(reverse_read.GetEndPosition())+") was not identified.");
						}
					}

					//amplicon mode
					if(getFlag("amplicon"))
					{
						bool read = qrand()%2;
						if(read)
						{
							clip_forward_read = overlap;
							clip_reverse_read = 0;
						}
						else
						{
							clip_forward_read = 0;
							clip_reverse_read = overlap;
						}
					}

					//verbose mode
					if(verbose)	out << "forward read: name - " << QString::fromStdString(forward_read.Name) << ", region - chr" << forward_read.RefID << ":" << forward_read.Position << "-" << forward_read.GetEndPosition() << ", insert size: "  << forward_read.InsertSize << " bp; mate: " << forward_read.MatePosition << ", CIGAR " << NGSHelper::Cigar2QString(forward_read.CigarData) << ", overlap: " << overlap << " bp" << endl;
					if(verbose)	out << "reverse read: name - " << QString::fromStdString(reverse_read.Name) << ", region - chr" << reverse_read.RefID << ":" << reverse_read.Position << "-" << reverse_read.GetEndPosition() << ", insert size: "  << reverse_read.InsertSize << " bp; mate: " << reverse_read.MatePosition << ", CIGAR " << NGSHelper::Cigar2QString(reverse_read.CigarData) << ", overlap: " << overlap << " bp" << endl;
					if(verbose) out << "forward read bases " << QString::fromStdString(forward_read.QueryBases) << endl;
					if(verbose) out << "forward read qualities " << QString::fromStdString(forward_read.Qualities) << endl;
					if(verbose) out << "reverse read bases " << QString::fromStdString(reverse_read.QueryBases) << endl;
					if(verbose) out << "reverse read qualities " << QString::fromStdString(reverse_read.Qualities) << endl;
					if(verbose)	out << "  clip forward read from position " << (forward_read.GetEndPosition()-clip_forward_read+1) << " to " << forward_read.GetEndPosition() << endl;
					if(verbose)	out << "  clip reverse read from position " << (reverse_read.Position+1) << " to " << (reverse_read.Position+clip_reverse_read) << endl;

					struct Overlap
					{
						QList<int> genome_pos;
						QList<int> read_pos;
						QList<QString> base;
						QList<QString> cigar;

						void overlap(QString base, QString cigar, int genome_pos, int read_pos)
						{
							this->base.append(base);
							this->cigar.append(cigar);
							this->genome_pos.append(genome_pos);
							this->read_pos.append(read_pos);
						}

						void append(QString base, QString cigar, int genome_pos, int read_pos)
						{
							this->base.append(base);
							this->cigar.append(cigar);
							this->genome_pos.append(genome_pos);
							this->read_pos.append(read_pos);
						}

						void insert(int at, QString base, QString cigar, int genome_pos, int read_pos)
						{
							this->base.insert(at,base);
							this->cigar.insert(at,cigar);
							this->genome_pos.insert(at,genome_pos);
							this->read_pos.insert(at,read_pos);
						}

						QString getBases()
						{
							QString string;
							for(int i=0; i<base.length(); ++i)
							{
								string += base[i];
							}
							return string;
						}

						QString getCigar()
						{
							QString string;
							for(int i=0; i<cigar.length(); ++i)
							{
								string += cigar[i];
							}
							return string;
						}

						int length()
						{
							if(read_pos.length()!=cigar.length())	THROW(Exception,"Lengths differ.");
							return read_pos.length();
						}
					};


					//check if bases in overlap match
					int start = overlap_start;
					int end = overlap_end;

					int genome_pos = forward_read.Position;
					int read_pos = 0;
					Overlap forward_overlap;
					QString forward_bases = QString::fromStdString(forward_read.QueryBases);
					QString forward_cigar = NGSHelper::Cigar2QString(forward_read.CigarData,true);
					for(int i = 0;i<forward_cigar.length();++i)
					{
						if(start<=genome_pos && end>genome_pos)
						{
							QChar current_base = forward_bases[read_pos];
							if(forward_cigar[i]=='D')	current_base = '-';
							forward_overlap.append(QString(current_base), QString(forward_cigar[i]), genome_pos, read_pos);
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
						else	THROW(Exception, "Unknown CIGAR character '" + forward_cigar[i] + "'")

					}
					if(verbose)	out << "  finished reading overlap forward bases " << forward_overlap.getBases() << endl;
					if(verbose)	out << "  finished reading overlap forward cigar " << forward_overlap.getCigar() << endl;

					genome_pos = reverse_read.Position;
					read_pos = 0;
					Overlap reverse_overlap;
					QString reverse_bases = QString::fromStdString(reverse_read.QueryBases);
					QString reverse_cigar = NGSHelper::Cigar2QString(reverse_read.CigarData,true);
					for(int i=0; i<reverse_cigar.length();++i)
					{
						if(start<=genome_pos && end>genome_pos)
						{
							QChar current_base = reverse_bases[read_pos];
							if(reverse_cigar[i]=='D')	current_base = '-';
							reverse_overlap.append(QString(current_base), QString(reverse_cigar[i]), genome_pos, read_pos);
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
						else	THROW(Exception, "Unknown CIGAR character '" + reverse_cigar[i] + "'")
					}
					if(verbose)	out << "  finished reading overlap reverse bases " << reverse_overlap.getBases() << endl;
					if(verbose)	out << "  finished reading overlap reverse cigar " << reverse_overlap.getCigar() << endl;

					//correct for insertions
					for(int i=0;i<reverse_overlap.length();++i)
					{
						if(forward_overlap.cigar[i]!=reverse_overlap.cigar[i] && reverse_overlap.cigar[i]=="I")
						{
							forward_overlap.insert(i,"+","I",forward_overlap.genome_pos[i],forward_overlap.read_pos[i]);
						}
					}
					for(int i=0;i<forward_overlap.length();++i)
					{
						if(forward_overlap.cigar[i]!=reverse_overlap.cigar[i] && forward_overlap.cigar[i]=="I")
						{
							reverse_overlap.insert(i,"+","I",reverse_overlap.genome_pos[i],reverse_overlap.read_pos[i]);
						}
					}
					if(forward_overlap.length()!=reverse_overlap.length())	THROW(Exception, "Length mismatch.");	//both cigar and base string should now be equally long
					if(verbose)	out << "finished indel correction forward bases " << forward_overlap.getBases() << endl;
					if(verbose)	out << "finished indel correction forward cigar " << forward_overlap.getCigar() << endl;
					if(verbose)	out << "finished indel correction reverse bases " << reverse_overlap.getBases() << endl;
					if(verbose)	out << "finished indel correction reverse cigar " << reverse_overlap.getCigar() << endl;

					//detect mismtaches (read pos for, read pos rev)
					QList<QPair<int,int>> mm_pos;
					for(int i=0;i<forward_overlap.length();++i)
					{
						if(forward_overlap.base[i]!=reverse_overlap.base[i])
						{
							int first = forward_overlap.read_pos[i];
							int second = reverse_overlap.read_pos[i];
							if(forward_overlap.base[i]=="-" || forward_overlap.base[i]=="+")	first = -1;
							if(reverse_overlap.base[i]=="-" || reverse_overlap.base[i]=="+")	second = -1;
							mm_pos.append(qMakePair(first,second));
						}
					}
					if(verbose && !mm_pos.isEmpty()) out << "  overlap mismatch for read pair " << QString::fromStdString(forward_read.Name) << " - " << forward_overlap.getBases() << " != " << reverse_overlap.getBases() << "!" << endl;

					bool map = getFlag("overlap_mismatch_mapq");
					bool rem = getFlag("overlap_mismatch_remove");
					bool base = getFlag("overlap_mismatch_baseq");
					if(base || rem || map)
					{
						if(!mm_pos.isEmpty() && map)
						{
							forward_read.MapQuality = 0;
							reverse_read.MapQuality = 0;
							reads_mismatch += 2;
							if(verbose) out << "  Set mapping quality to 0." << endl;
						}
						else if(!mm_pos.isEmpty() && rem)
						{
							reads_mismatch += 2;
							skip_al = true;
							if(verbose) out << "   Removed pair." << endl;
						}
						else if(!mm_pos.isEmpty() && base)
						{

							reads_mismatch += 2;
							QString orig_for = QString::fromStdString(forward_read.Qualities);
							QString orig_rev = QString::fromStdString(reverse_read.Qualities);
							QString new_for = orig_for;
							QString new_rev = orig_rev;

							//set base quality for change qualities
							for(int i=0;i<mm_pos.length();++i)
							{
								if(mm_pos[i].first>=0)	new_for[mm_pos[i].first] = '!';
								if(mm_pos[i].second>=0)	new_rev[mm_pos[i].second] = '!';
							}
							forward_read.Qualities = new_for.toStdString();
							reverse_read.Qualities = new_rev.toStdString();
							if(verbose) out << "   changed forward base qualities from " << orig_for << " to " << QString::fromStdString(forward_read.Qualities) << endl;
							if(verbose) out << "   changed reverse base qualities from " << orig_rev << " to " << QString::fromStdString(reverse_read.Qualities) << endl;
						}
						else
						{
							if(verbose)	out << "  no overlap mismatch for read pair " << QString::fromStdString(forward_read.Name) << endl;

						}
					}

					//actual soft clipping
					if(clip_forward_read>0)	NGSHelper::softClipAlignment(forward_read,(forward_read.GetEndPosition()-clip_forward_read+1),forward_read.GetEndPosition());
					if(clip_reverse_read>0)	NGSHelper::softClipAlignment(reverse_read,(reverse_read.Position+1),(reverse_read.Position+clip_reverse_read));

					//set new insert size and mate position
					forward_read.InsertSize = reverse_read.GetEndPosition()-forward_read.Position;	//positive value
					forward_read.MatePosition = reverse_read.Position;
					reverse_read.InsertSize = forward_read.Position-reverse_read.GetEndPosition();	//negative value
					reverse_read.MatePosition = forward_read.Position;

					if(verbose)	out << "  clipped forward read: name - " << QString::fromStdString(forward_read.Name) << ", region - chr" << forward_read.RefID << ":" << forward_read.Position << "-" << forward_read.GetEndPosition() << ", insert size: "  << forward_read.InsertSize << " bp; mate: " << forward_read.MatePosition << ", CIGAR " << NGSHelper::Cigar2QString(forward_read.CigarData) << ", overlap: " << overlap << " bp" << endl;
					if(verbose)	out << "  clipped reverse read: name - " << QString::fromStdString(reverse_read.Name) << ", region - chr" << forward_read.RefID << ":" << reverse_read.Position << "-" << reverse_read.GetEndPosition() << ", insert size: "  << reverse_read.InsertSize << " bp; mate: " << reverse_read.MatePosition << ", CIGAR " << NGSHelper::Cigar2QString(reverse_read.CigarData) << ", overlap: " << overlap << " bp" << endl;
					if(verbose)	out << endl;

					//return reads
					bases_clipped += overlap;
					reads_clipped += 2;
				}


				//save reads
				reads_saved+=2;
				if(skip_al)	continue;
				writer.SaveAlignment(forward_read);
				writer.SaveAlignment(reverse_read);
			}
			else    //keep in map
			{
				al_map.insert(QString::fromStdString(al.Name), al);
			}
		}

		//step 3: save all remaining reads in QHash
		QHash<QString, BamAlignment>::Iterator i;
		for(i=al_map.begin(); i!=al_map.end(); ++i)
		{
			writer.SaveAlignment(i.value());
			++reads_saved;
		}

		//step 4: write out statistics
		if(reads_saved!=reads_count)	THROW(ToolFailedException, "Lost Reads: "+QString::number(reads_count-reads_saved)+"/"+QString::number(reads_count));
		out << "Overlap mismatch filtering was used for " << QString::number(reads_mismatch) << " of " << QString::number(reads_count) << " reads (" << QString::number((double)reads_mismatch/(double)reads_count*100,'f',2) << " %)." << endl;
		out << "Softclipped " << QString::number(reads_clipped) << " of " << QString::number(reads_count) << " reads (" << QString::number(((double)reads_clipped/(double)reads_count*100),'f',2) << " %)." << endl;
		out << "Softclipped " << QString::number(bases_clipped) << " of " << QString::number(bases_count) << " basepairs (" << QString::number((double)bases_clipped/(double)bases_count*100,'f',2) << " %)." << endl;

		//done
		reader.Close();
		writer.Close();
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
