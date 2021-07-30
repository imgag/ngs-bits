#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"
#include "VcfFile.h"
#include <QFile>
#include <QSharedPointer>

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
		setDescription("Annotates the INFO column of a VCF with data from a BED file.");
		QByteArray mapping_string;
		foreach (KeyValuePair replacement, VcfFile::INFO_URL_MAPPING)
		{
			mapping_string += replacement.key + " -> " + replacement.value + "; ";
		}
		setExtendedDescription(QStringList()	<< "Characters which are not allowed in the INFO column based on the VCF 4.2 definition are URL encoded."
												<< "The following characters are replaced:" << mapping_string);
		addInfile("bed", "BED file used as source of annotations (name column).", false, true);
		addString("name", "Annotation name in INFO column of output VCF file.", false);
		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);
		addString("sep", "Separator used if there are several matches for one variant.", true, ":");

		changeLog(2021,  6, 15, "Added 'sep' parameter.");
		changeLog(2019, 12,  6, "Added URL encoding for INFO values.");
		changeLog(2017,  3, 14, "Initial implementation.");
	}

	//TODO multi-threaded implementation.
	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");
		QByteArray bed = getInfile("bed").toLatin1();
		QByteArray name = getString("name").toLatin1().trimmed();
		QByteArray sep = getString("sep").toLatin1().trimmed();
		char tab = '\t';

		//load BED file
		BedFile bed_data;
		bed_data.load(bed);
		if (!bed_data.isSorted()) bed_data.sort();
		ChromosomalIndex<BedFile> bed_index(bed_data);

		//check BED file
		for(int i=0; i<bed_data.count(); ++i)
		{
			BedLine& line = bed_data[i];
			if (line.annotations().count()==0)
			{
				THROW(FileParseException, "BED line '" + line.toString(true) + "' has no name column: " + line.toString(true));
			}
			if (line.annotations()[0].contains(sep))
			{
				THROW(FileParseException, "BED line '" + line.toString(true) + "' name column contains separator: " + line.annotations()[0]);
			}
		}

		//open input/output streams
		if(!in.isEmpty() && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);

		while(!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();
			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//write out headers unchanged
			if (line.startsWith('#'))
			{
				//append header line for new annotation
				if (line.startsWith("#CHROM"))
				{
					out_p->write("##INFO=<ID=" + name + ",Number=.,Type=String,Description=\"Annotation from " + bed + " delimited by '" + sep + "'\">\n");
				}

				out_p->write(line);
				out_p->write("\n");
				continue;
			}

			//split line and extract variant infos
			QList<QByteArray> parts = line.split('\t');
			if (parts.count()<VcfFile::MIN_COLS) THROW(FileParseException, "VCF line with too few columns: " + line);
			Chromosome chr = parts[0];
			bool ok = false;
			int start = parts[1].toInt(&ok);
			if (!ok) THROW(FileParseException, "Could not convert VCF variant position '" + parts[1] + "' to integer!");
			int end = start + parts[3].length() - 1; //length of ref

			//get annotation data
			QByteArrayList annos;
			QVector<int> indices = bed_index.matchingIndices(chr, start, end);
			foreach(int index, indices)
			{
				annos << bed_data[index].annotations()[0];
			}

			//write output line
			if (annos.isEmpty())
			{
				out_p->write(line);
			}
			else
			{
				for(int i=0; i<parts.count(); ++i)
				{
					if (i!=0) out_p->write(&tab, 1);
					if (i==7)
					{
						if (!parts[7].isEmpty())
						{
							out_p->write(parts[7] + ";");
						}
						out_p->write(name + "=" + VcfFile::encodeInfoValue(annos.join(sep)).toUtf8());
					}
					else
					{
						out_p->write(parts[i]);
					}
				}
			}
			out_p->write("\n");
		}
    }

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

