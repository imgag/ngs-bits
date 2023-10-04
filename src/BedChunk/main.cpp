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
		addInt("n", "The desired chunk size. Note: Not all chunks will have this size. Regions are split to chunks that are closest to the desired size.", false);
		//optional
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
	}

	virtual void main()
	{
		BedFile file;
		file.load(getInfile("in"));
		file.chunk(getInt("n"));
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
