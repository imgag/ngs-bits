#include "BedpeFile.h"
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
		setDescription("Sort a BEDPE file according to chromosomal position.");
		addInfile("in", "Input BEDPE file.", false);
		addOutfile("out", "Output BEDPE file.", false);

		changeLog(2022,  2, 17, "Initial version");
	}

	virtual void main()
	{
		BedpeFile file;
		file.load(getInfile("in"));
		file.sort();
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
