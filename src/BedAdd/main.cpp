#include "BedFile.h"
#include "ToolBase.h"

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
		setDescription("Merges regions from several BED files.");
		addInfileList("in", "Input BED files.", false);
		//optional
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);

		changeLog(2018, 04, 03, "Removed 'in2' argument and made 'in' a file list.");
	}

	virtual void main()
	{
		//init
		BedFile out;

		//merge
		QStringList bed_files = getInfileList("in");
		foreach(QString bed_file, bed_files)
		{
			BedFile in;
			in.load(bed_file);
			out.add(in);
		}

		//store
		out.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
