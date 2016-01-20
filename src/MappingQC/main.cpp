#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
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
		setDescription("Calculates QC metrics based on mapped NGS reads.");
		addInfile("in", "Input BAM file.", false, true);
		//optional
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true);
		addInfile("roi", "Input target region BED file (for panel, WES, etc.).", true, true);
		addFloat("wgs", "Overall genome size (for WGS).", true, -1.0);
		addFlag("txt", "Writes TXT format instead of qcML.");
		addFlag("3exons", "Adds special QC terms estimating the sequencing error on reads from three exons.");
	}

	virtual void main()
	{
		//init
		QString roi_file = getInfile("roi");
		double genome_size = getFloat("wgs");

        //check that either ROI or WGS is given
		if (roi_file=="" && genome_size<=0.0)
        {
            THROW(CommandLineParsingException, "You have to provide the parameter 'roi' or 'wgs'!");
        }

		QStringList parameters;
		QCCollection metrics;
		if (genome_size>0.0)
        {
			metrics = Statistics::mapping(genome_size, getInfile("in"));

			//parameters
			parameters << "-wgs";
		}
        else
        {
			//load ROI
            BedFile roi;
			roi.load(roi_file);
			roi.merge();

			//calculate metrics
			metrics = Statistics::mapping(roi, getInfile("in"));

			//parameters
			parameters << "-roi" << QFileInfo(roi_file).fileName();
        }

		//special QC for 3 exons
		QMap<QString, int> precision_overwrite;
		precision_overwrite.insert("error estimation N percentage", 4);
		precision_overwrite.insert("error estimation SNV percentage", 4);
		precision_overwrite.insert("error estimation indel percentage", 4);
		QCCollection metrics_3exons;
		if (getFlag("3exons"))
		{
			metrics_3exons = Statistics::mapping3Exons(getInfile("in"));

			//parameters
			parameters << "-3exons";
		}

		//store output
		if (getFlag("txt"))
		{
			QStringList output;
			metrics.appendToStringList(output);
			output << "";
			metrics_3exons.appendToStringList(output, precision_overwrite);
			Helper::storeTextFile(Helper::openFileForWriting(getOutfile("out"), true), output);
		}
		else
		{
			metrics.insert(metrics_3exons);
			metrics.storeToQCML(getOutfile("out"), QStringList() << getInfile("in"), parameters.join(" "), precision_overwrite);
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

