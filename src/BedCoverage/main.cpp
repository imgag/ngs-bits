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
		addInfileList("bam", "Input BAM file(s).", false);
		addInt("min_mapq", "Minimum mapping quality.", true, 1);
		//optional
		addInfile("in", "Input BED file (note that overlapping regions will be merged before processing). If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);

	}

	virtual void main()
	{
		//load and merge regions
		BedFile file;
		file.load(getInfile("in"));
		file.merge(false);

		//get coverage info for bam files
		QString header = "#chr\tstart\tend";
		QStringList bams = getInfileList("bam");
		foreach(QString bam, bams)
		{
			Statistics::avgCoverage(file, bam, getInt("min_mapq"));
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

