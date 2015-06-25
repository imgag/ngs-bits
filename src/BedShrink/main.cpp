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
		setDescription("Shrinks the regions in a BED file.");
		addInfile("in", "Input BED file.", false, true);
		addOutfile("out", "Output BED file.", false, true);
		addInt("n", "The number of bases to shrink (on both sides of each region).", false);
	}

	virtual void main()
	{
		BedFile file;
		file.load(getInfile("in"));
		file.shrink(getInt("n"));
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
