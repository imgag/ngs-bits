#include "ToolBase.h"
#include "Settings.h"
#include "NGSHelper.h"
#include "Helper.h"
#include "VcfFile.h"
#include "VariantHgvsAnnotator.h"
#include "Auxilary.h"
#include "ChunkProcessor.h"
#include "OutputWorker.h"
#include <QThreadPool>
#include "ThreadCoordinator.h"


class ConcreteTool
        : public ToolBase
{
    Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
		setExitEventLoopAfterMain(false);
	}

	virtual void setup()
	{
		setDescription("Adds transcript-specific consequence predictions to a VCF file.");
		setExtendedDescription(extendedDescription());
		addInfile("in", "Input VCF file to annotate.", false);
		addInfile("gff", "Ensembl-style GFF file with transcripts, e.g. from https://ftp.ensembl.org/pub/release-112/gff3/homo_sapiens/Homo_sapiens.GRCh38.112.gff3.gz.", false);

		//optional
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addOutfile("out", "Output VCF file annotated with predicted consequences for each variant.", false);
		addInt("threads", "The number of threads used to read, process and write files.", true, 1);
		addInt("block_size", "Number of lines processed in one chunk.", true, 5000);
		addInt("prefetch", "Maximum number of blocks that may be pre-fetched into memory.", true, 64);
		addFlag("all", "If set, all transcripts are imported. The default is to skip transcripts not labeled as 'GENCODE basic' for Ensembl and not with RefSeq/BestRefSeq origin for Refseq.");
		addFlag("skip_not_hgnc", "Skip genes that do not have a HGNC identifier.");
		addString("tag", "Tag that is used for the consequence annotation.", true, "CSQ");
		addInt("max_dist_to_trans", "Maximum distance between variant and transcript.", true, 5000);
		addInt("splice_region_ex", "Number of bases at exon boundaries that are considered to be part of the splice region.", true, 3);
		addInt("splice_region_in5", "Number of bases at intron boundaries (5') that are considered to be part of the splice region.", true, 20);
		addInt("splice_region_in3", "Number of bases at intron boundaries (3') that are considered to be part of the splice region.", true, 20);
		addEnum("source", "GFF source.", true, QStringList() << "ensembl" << "refseq", "ensembl");
		addFlag("debug", "Enable debug output");

		changeLog(2024, 7, 26, "Added support for RefSeq GFF format (source parameter).");
		changeLog(2022, 7,  7, "Change to event-driven multithreaded implementation.");
	}

	QStringList extendedDescription()
	{
		QStringList output;

		output << "Variants are normalized accoring to the HGVS 3' rule to assess the effect, i.e. they are moved as far as possible in translation direction.";
		output << "Note: HGVS consequence calcualtion is only done if the variant is fully inside the transcript region.";

		return output;
	}

	virtual void main()
	{
		// init
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");

		QString in_file = getInfile("in");
		QString gff_file = getInfile("gff");
		QString out_file = getOutfile("out");
		bool all = getFlag("all");
		bool skip_not_hgnc = getFlag("skip_not_hgnc");

		int max_dist_to_trans = getInt("max_dist_to_trans");
		int splice_region_ex = getInt("splice_region_ex");
		int splice_region_in_5 = getInt("splice_region_in5");
		int splice_region_in_3 = getInt("splice_region_in3");

		if(max_dist_to_trans <= 0 || splice_region_ex <= 0 || splice_region_in_5 <= 0 || splice_region_in_3 <= 0)
		{
			THROW(CommandLineParsingException, "Distance to transcript and splice region parameters must be >= 1!");
		}

		Parameters params;

		//init multithreading
		params.block_size = getInt("block_size");
		params.threads = getInt("threads");
		params.prefetch = getInt("prefetch");
		params.debug = getFlag("debug");

		// check parameter:
		if (params.block_size < 1) THROW(ArgumentException, "Parameter 'block_size' has to be greater than zero!");
		if (params.threads < 1) THROW(ArgumentException, "Parameter 'threads' has to be greater than zero!");
		if (params.prefetch < params.threads) THROW(ArgumentException, "Parameter 'prefetch' has to be at least number of used threads!");

		//open input/output streams
		if(in_file != "" && in_file == out_file)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		params.in = in_file;
		params.out = out_file;


		//parse GFF file
		QTextStream stream(stdout);
		QTime timer;
		timer.start();

		GffSettings gff_settings;
		gff_settings.source = getEnum("source");
		gff_settings.print_to_stdout = true;
		gff_settings.include_all = all;
		gff_settings.skip_not_hgnc = skip_not_hgnc;
		GffData data = NGSHelper::loadGffFile(gff_file, gff_settings);
		stream << "Parsing transcripts took: " << Helper::elapsedTime(timer) << endl;

		//ceate transcript index
		data.transcripts.sortByPosition();
		MetaData meta(getString("tag").toUtf8(), ref_file, data.transcripts);
		meta.annotation_parameters.max_dist_to_transcript = max_dist_to_trans;
		meta.annotation_parameters.splice_region_ex = splice_region_ex;
		meta.annotation_parameters.splice_region_in_3 = splice_region_in_3;
		meta.annotation_parameters.splice_region_in_5 = splice_region_in_5;

		ThreadCoordinator* coordinator = new ThreadCoordinator(this, params, meta);
		connect(coordinator, SIGNAL(finished()), QCoreApplication::instance(), SLOT(quit()));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
