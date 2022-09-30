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
		setDescription("Detects high-coverage regions from a BAM/CRAM file.");
		setExtendedDescription(QStringList() << "Note that only read start/end are used. Thus, deletions in the CIGAR string are treated as covered.");
		addInfile("bam", "Input BAM/CRAM file.", false);
		addInt("cutoff", "Minimum depth to consider a base 'high coverage'.", false);
		//optional
		addInfile("in", "Input BED file containing the regions of interest. If unset, reads from STDIN.", true);
		addFlag("wgs", "WGS mode without target region. Genome information is taken from the BAM/CRAM file.");
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addInt("min_mapq", "Minimum mapping quality to consider a read.", true, 1);
		addInt("min_baseq", "Minimum base quality to consider a base.", true, 0);
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);
		addInt("threads", "Number of threads used.", true, 1);

		changeLog(2022,  9, 29, "Added 'threads' parameter.");
		changeLog(2020,  11, 27, "Added CRAM support.");
		changeLog(2020,  5,  26, "Added parameter 'min_baseq'.");
		changeLog(2020,  5,  14, "First version.");
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

		//get high-cov regions and store them
		BedFile output;
        if (wgs) //WGS
        {
			output = Statistics::highCoverage(bam, getInt("cutoff"), getInt("min_mapq"), getInt("min_baseq"), getInt("threads"), getInfile("ref"));
        }
		else //ROI
        {
            BedFile file;
            file.load(in);
			file.merge(true, true);
			output = Statistics::highCoverage(file, bam, getInt("cutoff"), getInt("min_mapq"), getInt("min_baseq"), getInt("threads"), getInfile("ref"));
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

