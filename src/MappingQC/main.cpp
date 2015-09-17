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
		addInfile("roi", "Input target region BED file.", true, true);
        addEnum("wgs", "Calculates statistics for WGS instead of a specific target region.", true, QStringList() << "" << "hg19", "");
		addFlag("txt", "Writes TXT format instead of qcML.");
		addFlag("3exons", "Adds special QC terms estimating the sequencing error on reads from three exons.");
	}

	virtual void main()
	{
        //check that either ROI or WGS is given
        if (getInfile("roi")=="" && getEnum("wgs")=="")
        {
            THROW(CommandLineParsingException, "You have to provide the parameter 'roi' or 'wgs'!");
        }

		QStringList parameters;
		QCCollection metrics;
        if (getEnum("wgs")!="")
        {
			metrics = Statistics::mapping(getEnum("wgs"), getInfile("in"));

			//parameters
			parameters << "-wgs";
		}
        else
        {
			//load ROI
            BedFile roi;
			roi.load(getInfile("roi"));
			roi.merge();

			//calculate metrics
			metrics = Statistics::mapping(roi, getInfile("in"));

			//parameters
			parameters << "-roi" << QFileInfo(getInfile("roi")).fileName();
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

