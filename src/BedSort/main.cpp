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
		setDescription("Sort the regions in a BED file.");
		//optional
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addFlag("with_name", "Uses name column (i.e. the 4th column) to sort if chr/start/end are equal.");
		addFlag("uniq", "If set, entries with the same chr/start/end are removed after sorting.");

		changeLog(2020,  5, 18, "Added 'with_name' flag.");
	}

	virtual void main()
	{
		BedFile file;
		file.load(getInfile("in"));
		if (getFlag("with_name"))
		{
			file.sortWithName();
		}
		else
		{
			file.sort();
		}
		if (getFlag("uniq")) file.removeDuplicates();
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
