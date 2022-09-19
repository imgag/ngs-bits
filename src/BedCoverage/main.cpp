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
		setDescription("Annotates a BED file with the average coverage of the regions from one or several BAM/CRAM file(s).");
		addInfileList("bam", "Input BAM/CRAM file(s).", false);
		//optional
		addInt("min_mapq", "Minimum mapping quality.", true, 1);
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addInt("decimals", "Number of decimals used in output.", true, 2);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);
		addFlag("clear", "Clear previous annotation columns before annotating (starting from 4th column).");
		addInt("threads", "Number of threads used.", true, 1);

		changeLog(2022,  9, 16, "Added 'threads' parameter and removed 'dup' parameter.");
		changeLog(2022,  8, 12, "Added parameter to clear previous annotation columns.");
		changeLog(2022,  8,  9, "Removed mode parameter (panel mode is always used now).");
		changeLog(2020, 11, 27, "Added CRAM support.");
		changeLog(2017,  6,  2, "Added 'dup' parameter.");
	}

	virtual void main()
	{
		//load and merge regions
		BedFile file;
		file.load(getInfile("in"));

		//clear previous annotations
		if (getFlag("clear"))
		{
			file.clearHeaders();
			file.clearAnnotations();
		}

		//get coverage info for bam files
		QByteArray header = "#chr\tstart\tend";
		QStringList bams = getInfileList("bam");
		foreach(QString bam, bams)
		{
			Statistics::avgCoverage(file, bam, getInt("min_mapq"), getInt("threads"), getInt("decimals"), getInfile("ref"));
			header += "\t" + QFileInfo(bam).baseName();
		}

		//store
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

