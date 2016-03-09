#include "ToolBase.h"
#include "api/BamReader.h"
#include "api/BamWriter.h"
#include "api/BamAlgorithms.h"
#include "NGSHelper.h"
#include "Helper.h"
#include <QTime>
#include <QString>
#include "FastqFileStream.h"
#include <QDebug>
#include <QDataStream>

using namespace BamTools;

class grouping
{
	public:
		int start_pos;
		int end_pos;
		QString barcode;


		bool operator==(const grouping &g1) const
		{
			return ((g1.start_pos == start_pos)&&(g1.end_pos == end_pos)&&(g1.barcode==barcode));
		}
};

inline uint qHash(const grouping &g1)
{
	return qHash(QString::number(g1.start_pos) + QString::number(g1.end_pos) + g1.barcode);
}

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

private:
	QHash <QString,QString> createReadIndexHash(QString indexfile)
	{

		FastqFileStream indexstream(indexfile, false);
		QHash <QString,QString> headers2barcodes;
		while (!indexstream.atEnd())//foreach index read
		{
			FastqEntry read1;
			indexstream.readEntry(read1);
			QString string_header = read1.header;
			QStringList header_parts=string_header.split(" ");
			headers2barcodes[header_parts[0]]=read1.bases;
		}
		return headers2barcodes;
	}

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Removes duplicates from a bam file based on a molecular barcode file.");
		addInfile("bam", "Input BAM file.", false);
		addInfile("index", "Index FASTQ file.", false);
		addOutfile("out", "Output BAM file.", false);
		addFlag("flag", "flag duplicate reads insteadt of deleting them");
	}

	virtual void main()
	{

		using readPair = QPair < BamAlignment, BamAlignment>;


		//step 1: init
		QHash <QString,QString> read_headers2barcodes=createReadIndexHash(getInfile("index"));//build read_header => index hash
		BamReader reader;
		NGSHelper::openBAM(reader, getInfile("bam"));
		BamWriter writer;
		QHash <grouping, readPair > read_groups;
		writer.Open(getOutfile("out").toStdString(), reader.GetConstSamHeader(), reader.GetReferenceData());

		BamAlignment al;
		QHash<QString, BamAlignment> al_map;
		int counter = 1;

		int last_start_pos=0;
		int last_ref=-1;
		bool chrom_change=false;

		while (reader.GetNextAlignment(al))
		{
			if (((counter%10000)==0)||(chrom_change))//write after every 10000th reads to reduce memory requirements
			{
				QHash <grouping, readPair > read_groups_new;
				QHash <grouping, readPair >::iterator i;
				for (i = read_groups.begin(); i != read_groups.end(); ++i)
				{
					/*make sure that no duplicate is missed because of trimmed second read
					(won't work if bam is not sorted by position TODO: Test if it works on sorted bams)*/
					if ((i.key().end_pos)<last_start_pos||(chrom_change))
					{
						writer.SaveAlignment(i.value().first);
						writer.SaveAlignment(i.value().second);
					}
					else
					{
						read_groups_new[i.key()]=i.value();
					}
				}
				read_groups=read_groups_new;
				chrom_change = false;
			}
			if((!al.IsPrimaryAlignment())||(!al.IsPaired())) continue;

			if((al_map.contains(QString::fromStdString(al.Name))))//if paired end and mate has been seen already
			{
					BamAlignment mate;
					mate = al_map.take(QString::fromStdString(al.Name));

					grouping act_group;
					act_group.barcode= read_headers2barcodes["@"+QString::fromStdString(al.Name)];
					act_group.start_pos= qMin(al.GetEndPosition(),qMin(al.Position,qMin(mate.GetEndPosition(),mate.Position)));
					act_group.end_pos= qMax(al.GetEndPosition(),qMax(al.Position,qMax(mate.GetEndPosition(),mate.Position)));
					//write pair to group, possibly overwriting previous read pairs of same group;

					readPair act_read_pair(al,mate);
					read_groups[act_group]=act_read_pair;
			}
			else//if paired end and mate has not been seen yet
			{
				al_map.insert(QString::fromStdString(al.Name), al);
				continue;
			}

			last_start_pos=qMin(al.Position,al.GetEndPosition());

			if (al.RefID!=last_ref)
			{
				last_ref=al.RefID;
				chrom_change=true;
			}

		}

		//write remaining pairs
		QHash <grouping, readPair >::iterator i;
		for (i = read_groups.begin(); i != read_groups.end(); ++i)
		{
			writer.SaveAlignment(i.value().first);
			writer.SaveAlignment(i.value().second);
		}


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
