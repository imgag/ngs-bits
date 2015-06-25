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
		setDescription("Subtracts the regions in one BED file from another.");
		addInfile("in1", "Input BED file from which is subtracted.", false, true);
		addInfile("in2", "Input BED file which is subtracted.", false, true);
		addOutfile("out", "Output BED file.", false, true);
	}

	virtual void main()
	{
		//input
		BedFile file1;
		file1.load(getInfile("in1"));
		BedFile file2;
		file2.load(getInfile("in2"));
		
		//sort/merge file 2 if necessary
		if (!file2.isMergedAndSorted()) file2.merge();

		//subtract
		file1.subtract(file2);
		
		//output
		file1.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
