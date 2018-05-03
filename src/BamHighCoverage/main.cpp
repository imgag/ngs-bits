#include "BedFile.h"
#include "ToolBase.h"
#include "Statistics.h"

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
		setDescription("Determines high-coverage regions in a BAM file.");
		addInfile("in", "Input BAM file.", false);
		addInt("cutoff", "Minimum depth to consider a chromosomal position 'high coverage'.", false);
		//optional
		addInt("min_mapq", "Minimum mapping quality.", true, 1);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);

		changeLog(2018,  5,  3, "Initial version of the tool.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		int cutoff = getInt("cutoff");
		int min_mapq = getInt("min_mapq");

		//calculate
		BedFile output = Statistics::highCoverage(in, cutoff, min_mapq);

		//store
		output.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

