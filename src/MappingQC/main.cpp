#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Statistics.h"
#include "Exceptions.h"
#include "Settings.h"
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
		addEnum("build", "Genome build used to generate the input (needed for contamination only).", true, QStringList() << "hg19" << "hg38", "hg38");
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addFlag("cfdna", "Add additional QC parameters for cfDNA samples. Only supported mit '-roi'.");
		addInfile("somatic_custom_bed", "Somatic custom region of interest (subpanel of actual roi). If specified, additional depth metrics will be calculated.", true, true);

		//changelog
		changeLog(2022,  5, 25, "Added new QC metrics to WGS mode.");
		changeLog(2021,  2,  9, "Added new QC metrics for uniformity of coverage (QC:2000057-QC:2000061).");
		changeLog(2020, 11, 27, "Added CRAM support.");
		changeLog(2018,  7, 11, "Added build switch for hg38 support.");
		changeLog(2018,  3, 29, "Removed '3exons' flag.");
		changeLog(2016, 12, 20, "Added support for spliced RNA reads (relevant e.g. for insert size)");
	}

	virtual void main()
	{
		//init
		QString roi_file = getInfile("roi");
		bool wgs = getFlag("wgs");
		bool rna = getFlag("rna");
		QString in = getInfile("in");
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		bool cfdna = getFlag("cfdna");

		int min_mapq = getInt("min_mapq");
		bool debug = getFlag("debug");
		GenomeBuild build = stringToBuild(getEnum("build"));

		QString somatic_custom_roi_file = getInfile("somatic_custom_bed");

		// check that just one of roi_file, wgs, rna is set
		int parameters_set =  (roi_file!="" ? 1 : 0) +  wgs + rna;
		if (parameters_set!=1)
		{
			THROW(CommandLineParsingException, "You have to use exactly one of the parameters 'roi', 'wgs', or 'rna' !");
		}
		if (cfdna && (roi_file == ""))
		{
			 THROW(CommandLineParsingException, "The flag 'cfdna' can only be used with parameter 'roi'!");
		}
		QStringList parameters;
		QCCollection metrics;
		if (wgs)
		{
			QString genome_region = "";
			if (build == GenomeBuild::HG19)
			{
				genome_region = "://resources/hg19_439_omim_genes.bed";
			}
			else if (build == GenomeBuild::HG38)
			{
				genome_region = "://resources/hg38_440_omim_genes.bed";
			}

			metrics = Statistics::mapping_wgs(in, genome_region, min_mapq, ref_file);

			//parameters
			parameters << "-wgs";
		}
		else if(rna)
		{
			metrics = Statistics::mapping(in, min_mapq, ref_file);

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
			metrics = Statistics::mapping(roi, in, ref_file, min_mapq, cfdna);

			//parameters
			parameters << "-roi" << QFileInfo(roi_file).fileName();
			if (cfdna) parameters << "-cfdna";
		}

		//sample contamination
		QCCollection metrics_cont;
		if (!getFlag("no_cont"))
		{
			metrics_cont = Statistics::contamination(build, in, ref_file, debug);
		}

		if(somatic_custom_roi_file != "")
		{
			BedFile custom_bed;
			custom_bed.load(somatic_custom_roi_file);
			custom_bed.merge();
			QCCollection custom_depths = Statistics::somaticCustomDepth(custom_bed, in, ref_file, min_mapq);
			metrics.insert(custom_depths);
			parameters << "-somatic_custom_bed " + somatic_custom_roi_file;
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

