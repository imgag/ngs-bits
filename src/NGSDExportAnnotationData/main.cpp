#include "ToolBase.h"
#include "NGSD.h"
#include "Auxilary.h"
#include "ThreadCoordinator.h"
#include "Settings.h"

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
		setDescription("Export information aboug germline variants, somatic variants and genes form NGSD for use as annotation source, e.g. in megSAP.");
		addOutfile("germline", "Export germline variants (VCF format).", true);
		addOutfile("somatic", "Export somatic variants (VCF format).", true);
		addOutfile("genes", "Exports BED file containing genes and gene information.", true);
		addInfile("reference", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addFloat("max_af", "Maximum allel frequency of exported variants (germline).",  true, 0.05);
		addInt("gene_offset", "Defines the number of bases by which the regions of genes are extended (genes).", true, 5000);
		addFlag("vicc_config_details", "Includes details about VICC interpretation (somatic).");
		addInt("threads", "Number of threads to use.", true, 5);
		addFlag("verbose", "Enables verbose debug output.");
		addInt("max_vcf_lines", "Maximum number of VCF lines to write per chromosome - for debugging.", true, -1);
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2023,  6, 18, "Refactoring of command line parameters and parallelization of somatic export.");
		changeLog(2023,  6, 16, "Added support for 'germline_mosaic' column in 'variant' table and added parallelization.");
		changeLog(2021,  7, 19, "Code and parameter refactoring.");
		changeLog(2021,  7, 19, "Added support for 'germline_het' and 'germline_hom' columns in 'variant' table.");
		changeLog(2019, 12,  6, "Comments are now URL encoded.");
		changeLog(2019,  9, 25, "Added somatic mode.");
		changeLog(2019,  7, 29, "Added BED file for genes.");
		changeLog(2019,  7, 25, "Initial version of this tool.");
	}

	virtual void main()
	{
		//init
		QString ref_file = getInfile("reference");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");

		//annotate
		ExportParameters params;
		params.ref_file = ref_file;
		params.germline = getOutfile("germline");
		params.somatic = getOutfile("somatic");
		if (params.germline.isEmpty() && params.somatic.isEmpty()) THROW(CommandLineParsingException, "At least one of the parameters 'germline' and 'somatic' needs to be given!");
		params.vicc_config_details = getFlag("vicc_config_details");
		params.max_af = getFloat("max_af");
		if (params.max_af < 0) THROW(CommandLineParsingException, "Maximum AF has to be a positive value!");
		params.max_vcf_lines = getInt("max_vcf_lines");
		params.threads = getInt("threads");
		if (params.threads < 0) THROW(CommandLineParsingException, "Number of threads has to be a positive value!");
		params.genes = getOutfile("genes");
		params.gene_offset = getInt("gene_offset");
		if (params.gene_offset < 0) THROW(CommandLineParsingException, "Gene offset has to be a positive value!");
		params.use_test_db = getFlag("test");
		params.verbose = getFlag("verbose");
		params.version = version();
		params.datetime = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
		ThreadCoordinator* coordinator = new ThreadCoordinator(this, params);
		connect(coordinator, SIGNAL(finished()), QCoreApplication::instance(), SLOT(quit()));
		setExitEventLoopAfterMain(false);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
