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
		addInfile("in", "Input BAM/CRAM file.", false, true);
		//optional
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true);
		addInfile("roi", "Input target region BED file (for panel, WES, etc.).", true, true);
		addFlag("wgs", "WGS mode without target region. Genome information is taken from the BAM/CRAM file.");
		addFlag("rna", "RNA mode without target region. Genome information is taken from the BAM/CRAM file.");
		addFlag("txt", "Writes TXT format instead of qcML.");
		addInt("min_mapq", "Minmum mapping quality to consider a read mapped.", true, 1);
		addFlag("no_cont", "Disables sample contamination calculation, e.g. for tumor or non-human samples.");
		addFlag("debug", "Enables verbose debug outout.");
		addEnum("build", "Genome build used to generate the input (needed for contamination only).", true, QStringList() << "hg19" << "hg38", "hg19");
		addString("ref", "Reference genome for CRAM compression (compulsory for CRAM support).", true);

		//changelog
		changeLog(2020,  11, 27, "Added Cram support.");
		changeLog(2018,  7, 11, "Added build switch for hg38 support.");
		changeLog(2018, 03, 29, "Removed '3exons' flag.");
		changeLog(2016, 12, 20, "Added support for spliced RNA reads (relevant e.g. for insert size)");
	}

	virtual void main()
	{
		//init
		QString roi_file = getInfile("roi");
		bool wgs = getFlag("wgs");
        bool rna = getFlag("rna");
		QString in = getInfile("in");
		const QString ref_file = getString("ref");
		int min_maqp = getInt("min_mapq");
		bool debug = getFlag("debug");
		QString build = getEnum("build");
        // check that just one of roi_file, wgs, rna is set
        int parameters_set =  (roi_file!="" ? 1 : 0) +  wgs + rna;
        if (parameters_set!=1)
        {
            THROW(CommandLineParsingException, "You have to use exactly one of the parameters 'roi', 'wgs', or 'rna' !");
        }

		QStringList parameters;
		QCCollection metrics;
		if (wgs)
        {
			metrics = Statistics::mapping(in, min_maqp, ref_file);

			//parameters
			parameters << "-wgs";
		}
        else if(rna)
		{
			metrics = Statistics::mapping_rna(in, min_maqp, ref_file);

            //parameters
            parameters << "-rna";
        }
        else
        {
			//load ROI
            BedFile roi;
			roi.load(roi_file);
			roi.merge();

			//calculate metrics
			metrics = Statistics::mapping(roi, in, min_maqp, ref_file);

			//parameters
			parameters << "-roi" << QFileInfo(roi_file).fileName();
        }

		//sample contamination
		QCCollection metrics_cont;
		if (!getFlag("no_cont"))
		{
			metrics_cont = Statistics::contamination(build, in, ref_file, debug);
		}

		//special QC for 3 exons
		QMap<QString, int> precision_overwrite;
		precision_overwrite.insert("error estimation N percentage", 4);
		precision_overwrite.insert("error estimation SNV percentage", 4);
		precision_overwrite.insert("error estimation indel percentage", 4);

		//store output
		QString out = getOutfile("out");
		if (getFlag("txt"))
		{
			QStringList output;
			metrics.appendToStringList(output);
			output << "";
			metrics_cont.appendToStringList(output);
			Helper::storeTextFile(Helper::openFileForWriting(out, true), output);
		}
		else
		{
			metrics.insert(metrics_cont);
			metrics.storeToQCML(out, QStringList() << in, parameters.join(" "), precision_overwrite);
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

