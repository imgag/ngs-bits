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
		setDescription("Detects low-coverage regions from a BAM file.");
		addInfile("bam", "Input BAM file.", false);
		addInt("cutoff", "Minimum depth to consider a base 'high coverage'.", false);
		//optional
		addInfile("in", "Input BED file containing the regions of interest. If unset, reads from STDIN.", true);
        addFlag("wgs", "WGS mode without target region. Genome information is taken from the BAM file.");
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addInt("min_mapq", "Minimum mapping quality to consider a read.", true, 1);

		changeLog(2016,  6,  9, "The BED line name if the input BED file is now passed on to the output BED file.");
	}

	virtual void main()
    {
        //init
        QString in = getInfile("in");
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
            output = Statistics::lowCoverage(getInfile("bam"), getInt("cutoff"), getInt("min_mapq"));
        }
        else //ROI
        {
            BedFile file;
            file.load(in);
			file.merge(true, true);

            output = Statistics::lowCoverage(file, getInfile("bam"), getInt("cutoff"), getInt("min_mapq"));
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

