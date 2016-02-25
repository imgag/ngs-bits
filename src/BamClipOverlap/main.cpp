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
		int reads_clipped = 0;
		int bases_clipped = 0;
		int reads_count = 0;
		int reads_saved = 0;
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
				int s1 = mate.Position+1;
				int e1 = mate.GetEndPosition();
				int s2 = al.Position+1;
				int e2 = al.GetEndPosition();
				if(al.IsReverseStrand()!=mate.IsReverseStrand())
				{
					both_strands = true;
					if(!al.IsReverseStrand())
					{
						forward_read = al;
						reverse_read = mate;
						s1 = al.Position+1;
						e1 = al.GetEndPosition();
						s2 = mate.Position+1;
						e2 = mate.GetEndPosition();
					}
				}

				//check if reads overlap
				bool soft_clip = false;
				if(s1<=s2 && s2<=e1)	soft_clip = true;
				else if(s2<=s1 && s1<=e2)	soft_clip = true;

				//soft-clip overlapping reads
				if(soft_clip)
				{
					int clip_forward_read = 0;
					int clip_reverse_read = 0;
					int overlap = 0;

					if(s1<=s2 && e1<=e2)	//al is on the left of mate or both are completely overlapping
					{
						overlap = forward_read.GetEndPosition()-reverse_read.Position;
						clip_forward_read = static_cast<int>(overlap/2);
						clip_reverse_read = static_cast<int>(overlap/2);
						if(forward_read.IsFirstMate())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(s1>s2 && e1>e2)	//al is on the right of mate
					{
						overlap = reverse_read.GetEndPosition()-forward_read.Position;
						clip_forward_read = static_cast<int>(overlap/2) + (forward_read.GetEndPosition()-reverse_read.GetEndPosition());
						clip_reverse_read = static_cast<int>(overlap/2) + (forward_read.Position-reverse_read.Position);
						if(forward_read.IsFirstMate())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(both_strands==false && s1>s2 && e1<e2)	//al lies completely within mate (soft-clipping in some cases possible)
					{
						overlap = forward_read.GetEndPosition()-forward_read.Position;
						clip_forward_read = overlap;
						clip_reverse_read = 0;
					}
					else if(both_strands==false && s1<s2 && e1>e2)	//mate lies completely within al (soft-clipping in some cases possible)
					{
						overlap = reverse_read.GetEndPosition()-reverse_read.Position;
						clip_forward_read = 0;
						clip_reverse_read = overlap;
					}
					else if(both_strands==true && s1>s2 && e1<e2)
					{
						overlap = forward_read.GetEndPosition()-forward_read.Position;
						clip_forward_read = static_cast<int>(overlap/2);
						clip_reverse_read = static_cast<int>(overlap/2) + (forward_read.Position-reverse_read.Position);
						if(forward_read.IsFirstMate())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else if(both_strands==true && s1<s2 && e1>e2)	//mate lies completely within al (soft-clipping in some cases possible)
					{
						overlap = reverse_read.GetEndPosition()-reverse_read.Position;
						clip_forward_read = static_cast<int>(overlap/2) + (forward_read.GetEndPosition()-reverse_read.GetEndPosition());
						clip_reverse_read = static_cast<int>(overlap/2);
						if(forward_read.IsFirstMate())	clip_forward_read +=  overlap%2;
						else	clip_reverse_read +=  overlap%2;
					}
					else
					{
						THROW(Exception, "Read orientation of reads "+QString::fromStdString(al.Name)+" ("+QString::number(al.RefID)+":"+QString::number(al.Position)+"-"+QString::number(al.GetEndPosition())+") and "+QString::fromStdString(mate.Name)+" ("+QString::number(mate.RefID)+":"+QString::number(mate.Position)+"-"+QString::number(mate.GetEndPosition())+") was not identified.");
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
					al = forward_read;
					mate = reverse_read;
					bases_clipped += overlap;
					reads_clipped += 2;
				}

				//save reads
				writer.SaveAlignment(al);
				writer.SaveAlignment(mate);
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
		out << "Softclipped " << QString::number(reads_clipped) << " of " << QString::number(reads_count) << " reads." << endl;
		out << "Softclipped " << QString::number(bases_clipped) << " bases." << endl;

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
