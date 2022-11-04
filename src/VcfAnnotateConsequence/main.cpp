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
		addInfile("gff", "Ensembl-style GFF file with transcripts, e.g. from ftp://ftp.ensembl.org/pub/release-104/gff3/homo_sapiens/Homo_sapiens.GRCh38.104.chr.gff3.gz.", false);

		//optional
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addOutfile("out", "Output VCF file annotated with predicted consequences for each variant.", false);
		addInt("threads", "The number of threads used to read, process and write files.", true, 1);
		addInt("block_size", "Number of lines processed in one chunk.", true, 5000);
		addInt("prefetch", "Maximum number of blocks that may be pre-fetched into memory.", true, 64);
		addFlag("all", "If set, all transcripts are imported (the default is to skip transcripts not labeled with the 'GENCODE basic' tag).");
		addString("tag", "Tag that is used for the consequence annotation.", true, "CSQ");
		addInt("max_dist_to_trans", "Maximum distance between variant and transcript.", true, 5000);
		addInt("splice_region_ex", "Number of bases at exon boundaries that are considered to be part of the splice region.", true, 3);
		addInt("splice_region_in5", "Number of bases at intron boundaries (5') that are considered to be part of the splice region.", true, 20);
		addInt("splice_region_in3", "Number of bases at intron boundaries (3') that are considered to be part of the splice region.", true, 20);
		addFlag("debug", "Enable debug output");

		changeLog(2022, 7, 7, "Change to event-driven multithreaded implementation.");
	}

	QStringList extendedDescription()
	{
		//TODO describe rules of HGVSc/p annotation.
		//all variants are normalized towards the 3' end of the transcript (for everything! HGVS, consequence, ...)
		//HGVS descritpion is only possible if the variant is (fully) inside the transcript.
		//examples?
		return QStringList();
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

		GffData data;
		NGSHelper::loadGffFile(gff_file, data);
		stream << "Parsed " << QString::number(data.transcripts.count()) << " transcripts from input GFF file." << endl;
		stream << "Parsing transcripts took: " << Helper::elapsedTime(timer) << endl;

		//remove transcripts not flagged as GENCODE basic
		if (!all)
		{
			timer.start();
			int c_removed = 0;
			for (int i=data.transcripts.count()-1; i>=0; --i)
			{
				QByteArray trans_id = data.transcripts[i].name();
				if (!data.gencode_basic.contains(trans_id))
				{
					data.transcripts.removeAt(i);
					++c_removed;
				}
			}
			stream << "Removed " << QString::number(c_removed) << " transcripts because they are not flagged as 'GENCODE basic'." << endl;
			stream << "Number of transcripts remaining: " << data.transcripts.length() << endl;
			stream << "Removing transcripts took: " << Helper::elapsedTime(timer) << endl;
		}
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
