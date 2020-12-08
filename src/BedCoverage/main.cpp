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
		setDescription("Extracts the average coverage for input regions from one or several BAM/CRAM file(s).");
		addInfileList("bam", "Input BAM/CRAM file(s).", false);
		//optional
		addInt("min_mapq", "Minimum mapping quality.", true, 1);
		addFlag("dup", "Include reads marked as duplicates.");
		addInfile("in", "Input BED file (note that overlapping regions will be merged before processing). If unset, reads from STDIN.", true);
		addEnum("mode", "Mode to optimize run time. Use 'panel' mode if only a small part of the data in the BAM file is accessed, e.g. a sub-panel of an exome.", true, QStringList() << "default" << "panel", "default");
		addInt("decimals", "Number of decimals used in output.", true, 2);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addString("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);

		changeLog(2020,  11, 27, "Added CRAM support.");
		changeLog(2017,  6,  2, "Added 'dup' parameter.");
	}

	virtual void main()
	{
		//load and merge regions
		BedFile file;
		file.load(getInfile("in"));
		file.merge(false);

		//get coverage info for bam files
		QByteArray header = "#chr\tstart\tend";
		QStringList bams = getInfileList("bam");
		foreach(QString bam, bams)
		{
			Statistics::avgCoverage(file, bam, getInt("min_mapq"), getFlag("dup"), getEnum("mode")=="panel", getInt("decimals"), getString("ref"));
			header += "\t" + QFileInfo(bam).baseName();
		}

		//store
		file.clearHeaders();
		file.appendHeader(header);
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

