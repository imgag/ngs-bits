#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "Settings.h"
#include <QTextStream>

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
		//load annoation database
		BedFile anno_file;
		anno_file.load(getInfile("in2"));
		if (getFlag("clear"))
		{
			anno_file.clearAnnotations();
		}
		anno_file.sort();
		ChromosomalIndex<BedFile> anno_index(anno_file);

		//process
		BedFile file;
		file.load(getInfile("in"));
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

		//store
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
