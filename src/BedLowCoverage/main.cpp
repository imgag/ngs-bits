#include "BedFile.h"
#include "ToolBase.h"
#include "Statistics.h"
#include "Exceptions.h"
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
		setDescription("Detects low-coverage regions from a BAM/CRAM file.");
		setExtendedDescription(QStringList() << "Note that only read start/end are used. Thus, deletions in the CIGAR string are treated as covered.");
		addInfile("bam", "Input BAM/CRAM file.", false);
		addInt("cutoff", "Minimum depth to consider a base 'high coverage'.", false);
		//optional
		addInfile("in", "Input BED file containing the regions of interest. If unset, reads from STDIN.", true);
		addFlag("random_access", "Use random access via index to get reads from BAM/CRAM instead of chromosome-wise sweep. Random access is quite slow, so use it only if a small subset of the file needs to be accessed.");
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addInt("min_mapq", "Minimum mapping quality to consider a read.", true, 1);
		addInt("min_baseq", "Minimum base quality to consider a base.", true, 0);
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);
		addInt("threads", "Number of threads used.", true, 1);
		addFlag("high_depth_mode", "Allows cutoffs over 255x (e.g. cfDNA).");
		addFlag("debug", "Enable debug output.");

		changeLog(2025,  4,  16, "Added 'high_depth_mode' parameter for deep samples.");
		changeLog(2024,  7,  3, "Added 'random_access' and 'debug' parameters and removed 'wgs' parameter.");
		changeLog(2022,  9, 19, "Added 'threads' parameter.");
		changeLog(2020,  11, 27, "Added CRAM support.");
		changeLog(2020,  5,  26, "Added parameter 'min_baseq'.");
		changeLog(2016,  6,  9, "The BED line name of the input BED file is now passed on to the output BED file.");
	}

	virtual void main()
    {
        //init
        QString in = getInfile("in");
		QString bam = getInfile("bam");

		//get low-cov regions and store them
		BedFile file;
		file.load(in);
		file.merge(true, true);
		BedFile output = Statistics::lowCoverage(file, bam, getInt("cutoff"), getInt("min_mapq"), getInt("min_baseq"), getInt("threads"), getInfile("ref"), getFlag("random_access"), getFlag("debug"), getFlag("high_depth_mode"));

		output.appendHeader("#BAM: " + QFileInfo(bam).fileName().toUtf8());
		output.appendHeader("#ROI: " + QFileInfo(in).fileName().toUtf8());
		output.appendHeader("#ROI regions: " + QByteArray::number(file.count()));
		output.appendHeader("#ROI bases: " + QByteArray::number(file.baseCount()));

        output.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

