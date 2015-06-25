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
		addInfile("in", "Input BED file.", false, true);
		addOutfile("out", "Output BED file.", false, true);
		addFlag("uniq", "If set, subsequent duplicate entries are removed after sorting.");
	}

	virtual void main()
	{
		BedFile file;
		file.load(getInfile("in"));
		file.sort(getFlag("uniq"));
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
