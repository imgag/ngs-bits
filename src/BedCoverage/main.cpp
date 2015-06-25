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
		setDescription("Extracts the average coverage for input regions from one or several BAM file(s).");
		addInfile("in", "Input BED file containing the regions of interest. Note that overlapping regions will be merged before determining the coverage.", false, true);
		addInfileList("bam", "Input BAM file(s).", false, true);
		addFlag("anom", "Also consider anomalous reads.");
		addInt("min_mapq", "Minimum mapping quality.", true, 1);
		//optional
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
	}

	virtual void main()
	{
		//init
		bool anom = getFlag("anom");

		//load and merge regions
		BedFile file;
		file.load(getInfile("in"));
		file.merge(false);

		//get coverage info for bam files
		QString header = "#chr\tstart\tend";
		QStringList bams = getInfileList("bam");
		foreach(QString bam, bams)
		{
			Statistics::avgCoverage(file, bam, anom, getInt("min_mapq"));
			header += "\t" + QFileInfo(bam).baseName();
		}

		//store
		file.store(getOutfile("out"), header);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

