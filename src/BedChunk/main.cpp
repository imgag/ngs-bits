#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"

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
		setDescription("Splits all regions to chunks of an approximate desired size.");
		addInfile("in", "Input BED file.", false, true);
		addOutfile("out", "Output BED file.", false, true);
		addInt("n", "The desired chunk size. Note: Not all chunks will have this size. Regions are split to chunks that are closest to the the desired size.", false);
	}

	virtual void main()
	{
		//load and extend file
		BedFile file;
		file.load(getInfile("in"));

		//process
		file.chunk(getInt("n"));

		//store file
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
