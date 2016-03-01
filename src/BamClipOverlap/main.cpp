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
		addInfile("in", "Input bam file. Needs to be sorted by name.", false);
		addOutfile("out", "Output bam file.", false);
		//optional
		addFlag("amplicon", "Amplicon mode: one read of a pair will be clipped randomly.");
		addFlag("v", "Verbose mode.");
	}

	virtual void main()
	{
		//step 1: init
		int reads_count = 0;
		int reads_saved = 0;
		int reads_clipped = 0;
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

			//check preconditions and if unmet save read to out and continue
			if(!al.IsPaired() || !al.IsPrimaryAlignment())
			{
				writer.SaveAlignment(al);
				++reads_saved;
				continue;
			}
			if(!al.IsMapped() && !al.IsMateMapped())
			{
				writer.SaveAlignment(al);
				++reads_saved;
				continue;
			}
			if(al.RefID!=al.MateRefID)
			{
				writer.SaveAlignment(al);
				++reads_saved;
				continue;
			}
			if(al.CigarData.empty())
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

					if(s1<=s2 && e1<=e2)	// forward read left of reverse read
					{
						overlap = forward_read.GetEndPosition()-reverse_read.Position;
						clip_forward_read = static_cast<int>(overlap/2);
						clip_reverse_read = static_cast<int>(overlap/2);
						if(forward_read.IsFirstMate())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(s1>s2 && e1>e2)	// forward read right of reverse read
					{
						overlap = reverse_read.GetEndPosition()-forward_read.Position;
						clip_forward_read = static_cast<int>(overlap/2) + (forward_read.GetEndPosition()-reverse_read.GetEndPosition());
						clip_reverse_read = static_cast<int>(overlap/2) + (forward_read.Position-reverse_read.Position);
						if(forward_read.IsFirstMate())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(both_strands==true && s1>=s2 && e1<=e2)	// forward read within reverse read
					{
						overlap = forward_read.GetEndPosition()-forward_read.Position;
						clip_forward_read = static_cast<int>(overlap/2);
						clip_reverse_read = static_cast<int>(overlap/2) + (forward_read.Position-reverse_read.Position);
						if(forward_read.IsFirstMate())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(both_strands==true && s1<=s2 && e1>=e2)	//reverse read within forward read
					{
						overlap = reverse_read.GetEndPosition()-reverse_read.Position;
						clip_forward_read = static_cast<int>(overlap/2) + (forward_read.GetEndPosition()-reverse_read.GetEndPosition());
						clip_reverse_read = static_cast<int>(overlap/2);
						if(forward_read.IsFirstMate())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(both_strands==false && s1>=s2 && e1<=e2)	//forward read lies completely within reverse read
					{
						overlap = forward_read.GetEndPosition()-forward_read.Position;
						clip_forward_read = overlap;
						clip_reverse_read = 0;
					}
					else if(both_strands==false && s1<=s2 && e1>=e2)	//reverse read lies completely within foward read
					{
						overlap = reverse_read.GetEndPosition()-reverse_read.Position;
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

					if(verbose)	out << "forward: name - " << QString::fromStdString(forward_read.Name) << "region -" << forward_read.RefID << ":" << forward_read.Position << "-" << forward_read.GetEndPosition() << " is: "  << forward_read.InsertSize << "; m: " << forward_read.MatePosition << "; o: " << overlap << " CIGAR " << NGSHelper::Cigar2QString(forward_read.CigarData) << endl;
					if(verbose)	out << "reverse: name - " << QString::fromStdString(reverse_read.Name) << "region -" << forward_read.RefID << ":" << reverse_read.Position << "-" << reverse_read.GetEndPosition() << " is: "  << reverse_read.InsertSize << "; m: " << reverse_read.MatePosition << "; o: " << overlap << " CIGAR " << NGSHelper::Cigar2QString(reverse_read.CigarData) << endl;
					if(verbose)	out << " clip forward from " << (forward_read.GetEndPosition()-clip_forward_read+1) << " to " << forward_read.GetEndPosition() << endl;
					if(verbose)	out << " clip reverse from " << (reverse_read.Position+1) << " to " << (reverse_read.Position+clip_reverse_read) << endl;

					if(clip_forward_read>0)	NGSHelper::softClipAlignment(forward_read,(forward_read.GetEndPosition()-clip_forward_read+1),forward_read.GetEndPosition());
					if(clip_reverse_read>0)	NGSHelper::softClipAlignment(reverse_read,(reverse_read.Position+1),(reverse_read.Position+clip_reverse_read));

					//set new insert size and mate position
					forward_read.InsertSize = reverse_read.GetEndPosition()-forward_read.Position;	//positive value
					forward_read.MatePosition = reverse_read.Position;
					reverse_read.InsertSize = forward_read.Position-reverse_read.GetEndPosition();	//negative value
					reverse_read.MatePosition = forward_read.Position;

					if(verbose)	out << "-> forward: name - " << QString::fromStdString(forward_read.Name) << "region -" << forward_read.RefID << ":" << forward_read.Position << "-" << forward_read.GetEndPosition() << " is: "  << forward_read.InsertSize << "; m: " << forward_read.MatePosition << "; o: " << overlap << " CIGAR " << NGSHelper::Cigar2QString(forward_read.CigarData) << endl;
					if(verbose)	out << "-> reverse: name - " << QString::fromStdString(reverse_read.Name) << "region -" << forward_read.RefID << ":" << reverse_read.Position << "-" << reverse_read.GetEndPosition() << " is: "  << reverse_read.InsertSize << "; m: " << reverse_read.MatePosition << "; o: " << overlap << " CIGAR " << NGSHelper::Cigar2QString(reverse_read.CigarData) << endl;
					if(verbose)	out << endl;

					//return reads
					bases_clipped += overlap;
					reads_clipped += 2;
				}

				//save reads
				writer.SaveAlignment(forward_read);
				writer.SaveAlignment(reverse_read);
				reads_saved+=2;
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
		out << "Softclipped " << QString::number(reads_clipped) << " of " << QString::number(reads_count) << " reads (" << QString::number(((double)reads_clipped/(double)reads_count*100),'f',2) << " %)." << endl;
		out << "Softclipped " << QString::number(bases_clipped) << " of " << QString::number(bases_count) << " basepairs (" << QString::number((double)bases_clipped/(double)bases_count*100,'f',2) << " %)." << endl;
		out << "QC:2000040 " << QString::number(((double)reads_clipped/(double)reads_count*100),'f',2) << endl;
		out << "QC:2000041 " << QString::number((double)bases_clipped/(double)bases_count*100,'f',2) << endl;

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
