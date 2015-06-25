#include "BedFile.h"
#include "ToolBase.h"
#include "Statistics.h"
#include <QFileInfo>

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
		setDescription("Detects low-coverage regions from a BAM file.");
		addInfile("in", "Input BED file containing the regions of interest.", false, true);
		addInfile("bam", "Input BAM file.", false, true);
		addInt("cutoff", "Minimum depth to consider a base 'high coverage'.", false);
		addOutfile("out", "Output BED file.", false, true);
		//optional
		addFlag("single", "Also consider singletons (not prooperly paired, or single-end mapping).");
		addInt("min_mapq", "Minimum mapping quality to consider a read.", true, 1);
	}

	virtual void main()
	{
		//load and merger regions
		BedFile file;
		file.load(getInfile("in"));
		file.merge();

		//get low-cov regions and store them
		BedFile ouput = Statistics::lowCoverage(file, getInfile("bam"), getInt("cutoff"), getInt("min_mapq"), getFlag("single"));
		ouput.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

