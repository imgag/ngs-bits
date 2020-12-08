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
		addFlag("wgs", "WGS mode without target region. Genome information is taken from the BAM/CRAM file.");
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addInt("min_mapq", "Minimum mapping quality to consider a read.", true, 1);
		addInt("min_baseq", "Minimum base quality to consider a base.", true, 0);
		addString("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);

		changeLog(2020,  11, 27, "Added CRAM support.");
		changeLog(2020,  5,  26, "Added parameter 'min_baseq'.");
		changeLog(2016,  6,  9, "The BED line name of the input BED file is now passed on to the output BED file.");
	}

	virtual void main()
    {
        //init
        QString in = getInfile("in");
		QString bam = getInfile("bam");
        bool wgs = getFlag("wgs");

        //check that either IN or WGS is given
        if (in=="" && !wgs)
        {
            THROW(CommandLineParsingException, "You have to provide the parameter 'in' or 'wgs'!");
        }

		//get low-cov regions and store them
		BedFile output;
        if (wgs) //WGS
        {
			output = Statistics::lowCoverage(bam, getInt("cutoff"), getInt("min_mapq"), getInt("min_baseq"), getString("ref"));

			output.appendHeader("#BAM: " + QFileInfo(bam).baseName().toLatin1());
        }
        else //ROI
        {
            BedFile file;
            file.load(in);
			file.merge(true, true);
			output = Statistics::lowCoverage(file, bam, getInt("cutoff"), getInt("min_mapq"), getInt("min_baseq"), getString("ref"));

			output.appendHeader("#BAM: " + QFileInfo(bam).fileName().toLatin1());
			output.appendHeader("#ROI: " + QFileInfo(in).fileName().toLatin1());
			output.appendHeader("#ROI regions: " + QByteArray::number(file.count()));
			output.appendHeader("#ROI bases: " + QByteArray::number(file.baseCount()));
        }

        output.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

