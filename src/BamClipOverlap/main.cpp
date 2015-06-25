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

				//check if reads overlap
				int s1 = al.Position+1;
				int e1 = al.GetEndPosition();
				int s2 = mate.Position+1;
				int e2 = mate.GetEndPosition();
				bool soft_clip = false;
				if(s1<=s2 && s2<=e1)	soft_clip = true;
				else if(s2<=s1 && s1<=e2)	soft_clip = true;

				//soft-clip overlapping reads
				if(soft_clip)
				{

					//identify left and right read and prepare soft_clipping
					BamAlignment left_read;
					BamAlignment right_read;
					int clip_left_read = 0;
					int clip_right_read = 0;
					int overlap = 0;
					if((s1<s2 && e1<e2) ||
					   (s1==s2 && e1==e2) ||
					   (s1<s2 && e1==e2) ||
					   (s1==s2 && e1<e2))	//al is on the left of mate or both are completely overlapping
					{
						left_read = al;
						right_read = mate;

						overlap = left_read.GetEndPosition()-right_read.Position;
						clip_left_read = static_cast<int>(overlap/2);
						clip_right_read = static_cast<int>(overlap/2);
						if(left_read.IsFirstMate())	clip_left_read +=  overlap%2;
						else	clip_right_read +=  overlap%2;
					}
					else if((s2<s1 && e2<e1) ||
							(s1==s2 && e1>e2) ||
							(s2<s1 && e1==e2))	//al is on the right of mate
					{
						left_read = mate;
						right_read = al;

						overlap = left_read.GetEndPosition()-right_read.Position;
						clip_left_read = static_cast<int>(overlap/2);
						clip_right_read = static_cast<int>(overlap/2);
						if(left_read.IsFirstMate())	clip_left_read +=  overlap%2;
						else	clip_right_read +=  overlap%2;
					}
					else if(s1>s2 && e1<e2)	//al lies completely within mate (soft-clipping in some cases possible)
					{
						left_read = al;
						right_read = mate;

						overlap = left_read.GetEndPosition()-left_read.Position;
						clip_left_read = overlap;
						clip_right_read = 0;
					}
					else if(s2>s1 && e2<e1)	//mate lies completely within al (soft-clipping in some cases possible)
					{
						left_read = mate;
						right_read = al;

						overlap = left_read.GetEndPosition()-left_read.Position;
						clip_left_read = overlap;
						clip_right_read = 0;
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
							clip_left_read = overlap;
							clip_right_read = 0;
						}
						else
						{
							clip_left_read = 0;
							clip_right_read = overlap;
						}
					}

					if(verbose)	qDebug()<< "left: " << left_read.Position << "-" << left_read.GetEndPosition() << " l: " << " - " << " d: "  << left_read.InsertSize << "; m: " << left_read.MatePosition << "; o: " << overlap << " CIGAR " << NGSHelper::Cigar2QString(left_read.CigarData);
					if(verbose)	qDebug()<< "right: " << right_read.Position << "-" << right_read.GetEndPosition() << " l: " << " - " << " d: "  << right_read.InsertSize << "; m: " << right_read.MatePosition << "; o: " << overlap << " CIGAR " << NGSHelper::Cigar2QString(right_read.CigarData);
					if(verbose)	qDebug() << " clip left from " << (left_read.GetEndPosition()-clip_left_read+1) << " to " << left_read.GetEndPosition();
					if(verbose)	qDebug() << " clip right from " << (right_read.Position+1) << " to " << (right_read.Position+clip_right_read);

					if(clip_left_read>0)	NGSHelper::softClipAlignment(left_read,(left_read.GetEndPosition()-clip_left_read+1),left_read.GetEndPosition());
					if(clip_right_read>0)	NGSHelper::softClipAlignment(right_read,(right_read.Position+1),(right_read.Position+clip_right_read));

					//set new insert size and mate position
					left_read.InsertSize = right_read.GetEndPosition()-left_read.Position;	//positive value
					left_read.MatePosition = right_read.Position;
					right_read.InsertSize = left_read.Position-right_read.GetEndPosition();	//negative value
					right_read.MatePosition = left_read.Position;

					if(verbose)	qDebug()<< "-> left: " << left_read.Position << "-" << left_read.GetEndPosition() << " l: " << " - " << " d: "  << left_read.InsertSize << "; m: " << left_read.MatePosition << "; o: " << overlap << " CIGAR " << NGSHelper::Cigar2QString(left_read.CigarData);
					if(verbose)	qDebug()<< "-> right: " << right_read.Position << "-" << right_read.GetEndPosition() << " l: " << " - " << " d: "  << right_read.InsertSize << "; m: " << right_read.MatePosition << "; o: " << overlap << " CIGAR " << NGSHelper::Cigar2QString(right_read.CigarData);
					if(verbose)	qDebug();

					//return reads
					al = left_read;
					mate = right_read;
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
