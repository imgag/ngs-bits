#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "Settings.h"
#include <QTextStream>
#include <QFileInfo>

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
		setDescription("Annotates BED file regions with information from a second BED file.");
		addInfile("in2", "BED file that is used as annotation source (4th column is used as annotation value, if not presend 'yes' is used).", false);
		//optional
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addFlag("clear", "Clear all annotations present in the input file.");

		changeLog(2017, 11, 28, "Added 'clear' flag.");
		changeLog(2017, 11, 03, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString in2 = getInfile("in2");
		QString out = getOutfile("out");

		//load annoation database
		BedFile anno_file;
		anno_file.load(in2);
		if (getFlag("clear"))
		{
			anno_file.clearAnnotations();
		}
		anno_file.sort();
		ChromosomalIndex<BedFile> anno_index(anno_file);

		//process
		BedFile file;
		file.load(in);
		for(int i=0; i<file.count(); ++i)
		{
			BedLine& line = file[i];

			QList<QString> annos;
			QVector<int> indices = anno_index.matchingIndices(line.chr(), line.start(), line.end());
			foreach(int index, indices)
			{
				if (anno_file[index].annotations().isEmpty())
				{
					annos.append("yes");
				}
				else
				{
					annos.append(anno_file[index].annotations()[0]);
				}
			}
			annos.removeDuplicates();
			line.annotations().append(annos.join(",").toLatin1());
		}

		//special handling of TSV files (handle header line)
		if (out.endsWith(".tsv"))
		{
			QVector<QByteArray> headers = file.headers();
			for(int i=0; i<headers.count(); ++i)
			{
				QByteArray& line = headers[i];
				if (line.startsWith("#") && !line.startsWith("##") && line.contains("\t"))
				{
					line += "\t" + QFileInfo(in2).baseName();
				}
			}
			file.setHeaders(headers);
		}

		//store
		file.store(out);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
