#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QFile>
#include <QSharedPointer>
#include <QThreadPool>
#include <QDateTime>
#include "ChunkProcessor.h"
#include "OutputWorker.h"
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
        setDescription("Annotates a VCF file with MaxEntScan scores.");
        addInfile("gff", "Ensembl-style GFF file with transcripts, e.g. from https://ftp.ensembl.org/pub/release-107/gff3/homo_sapiens/Homo_sapiens.GRCh38.107.gff3.gz.", false);
        //optional
		addOutfile("out", "Output VCF file containing the MaxEntScan scores in the INFO column. If unset, writes to STDOUT.", true);
        addInfile("in", "Input VCF file. If unset, reads from STDIN.", true);
		addFlag("swa", "Enables sliding window approach, i.e. predictions of de-novo acceptor/donor sites.");
		addFlag("all", "If set, all transcripts are used for annotation (the default is to skip transcripts not labeled with the 'GENCODE basic' tag).");
		addString("tag", "Info entry name used for native splice site scores.", true, "MES");
		addString("tag_swa", "Info entry name used for SWA scores.", true, "MES_SWA");
		addInt("decimals", "Number of decimals of output scores.", true, 2);
		addFloat("min_score", "Minimum score to report.", true, -1000.0);
		addInt("threads", "The number of threads used to process VCF line chunk.", true, 1);
		addInt("block_size", "Number of VCF lines processed in one chunk.", true, 10000);
		addInt("prefetch", "Maximum number of chunks that may be pre-fetched into memory.", true, 64);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true);
		addFlag("debug", "Enables debug output (use only with one thread).");
    }

    QStringList extendedDescription()
	{
		QStringList desc;

		desc << "This is essentially a multithreaded C++ reimplementation of the MaxEntScan plugin for VEP (https://github.com/Ensembl/VEP_plugins/blob/release/109/MaxEntScan.pm). MaxEntScan was first introduced by Shamsani et al. (https://doi.org/10.1093/bioinformatics/bty960).";
        desc << "Benchmarking of this tool showed that it is up to 10x faster than the VEP plugin when using one thread.";
		desc << "The standard MES scores are only computed for variants which are close to known splice sites (which are computed from the provided GFF file).";
		desc << "De-novo splice sites can be found by using the MES scores from the sliding window approach (SWA).";
		desc << "Intergenic variants never get a MES scores.";

		return desc;
	}

    virtual void main()
    {
		QTextStream out(stdout);

		Parameters params;
		params.in = getInfile("in");
		params.out = getOutfile("out");
		params.tag = getString("tag");
		params.tag_swa = getString("tag_swa");
		params.decimals = getInt("decimals");
		params.min_score = getFloat("min_score");
		params.threads = getInt("threads");
		params.prefetch = getInt("prefetch");
		params.block_size = getInt("block_size");
		params.debug = getFlag("debug");
		params.swa = getFlag("swa");

		//check parameters
		if (params.block_size < 1) THROW(ArgumentException, "Parameter 'block_size' has to be greater than zero!");
		if (params.threads < 1) THROW(ArgumentException, "Parameter 'threads' has to be greater than zero!");
		if (params.prefetch < params.threads) THROW(ArgumentException, "Parameter 'prefetch' has to be at least number of used threads!");
		if (params.in!="" && params.in==params.out) THROW(ArgumentException, "Input and output files must be different when streaming!");

        // read in reference genome
		QTime timer;
		timer.start();
        QString ref_file = getInfile("ref");
        if (ref_file=="") ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
        FastaFileIndex reference(ref_file);
		out << "Reading reference took: " << Helper::elapsedTime(timer) << endl;

        // read in matrices
		timer.start();
		QHash<QByteArray,float> score5_rest_ = read_matrix_5prime(":/resources/score5_matrix.tsv");
        QHash<int,QHash<int,float>> score3_rest_ = read_matrix_3prime(":/resources/score3_matrix.tsv");
		out << "Parsing matrices from resources took: " << Helper::elapsedTime(timer) << endl;

		// parse GFF file
		timer.start();
        QString gff_path = getInfile("gff");
        bool all = getFlag("all");
        GffSettings gff_settings;
		gff_settings.print_to_stdout = true;
		gff_settings.skip_not_gencode_basic = !all;
		GffData gff_file = NGSHelper::loadGffFile(gff_path, gff_settings);
		out << "Parsed " << QString::number(gff_file.transcripts.count()) << " transcripts from input GFF file." << endl;
		out << "Parsing transcripts took: " << Helper::elapsedTime(timer) << endl;
        gff_file.transcripts.sortByPosition();

		QByteArrayList annotation_header_lines;
		annotation_header_lines.append("##INFO=<ID="+params.tag.toLatin1()+",Number=1,Type=String,Description=\"The MaxEntScan scores. FORMAT: A | separated list of maxentscan_ref&maxentscan_alt&transcript_name items.\">\n");
		if (params.swa)
		{
			annotation_header_lines.append("##INFO=<ID="+params.tag_swa.toLatin1()+",Number=1,Type=String,Description=\"The MaxEntScan SWA scores. FORMAT: A | separated list of maxentscan_ref_donor&maxentscan_alt_donor&maxentscan_donor_comp&maxentscan_ref_acceptor&maxentscan_alt_acceptor&maxentscan_acceptor_comp&transcript_name items.\">\n");
		}

		//set up meta data
		timer.start();
		MetaData meta(ref_file, gff_file.transcripts, score5_rest_, score3_rest_, annotation_header_lines);
		out << "Input file: \t" << params.in << "\n";
		out << "Output file: \t" << params.out << "\n";
		out << "Threads: \t" << params.threads << "\n";
		out << "Block (Chunk) size: \t" << params.block_size << "\n";
		out << "Parsing transcripts took: " << Helper::elapsedTime(timer) << endl;

		//create coordinator instance
		out << "Performing annotation" << endl;
		out << QDateTime::currentDateTime().toString(Qt::ISODate) << "Thread coordinator is started..." << endl;

		ThreadCoordinator* coordinator = new ThreadCoordinator(this, params, meta);
		connect(coordinator, SIGNAL(finished()), QCoreApplication::instance(), SLOT(quit()));
    }

private:


	


    QHash<QByteArray,float> read_matrix_5prime(const QByteArray& path) {
        QHash<QByteArray,float> result;
        QStringList lines = Helper::loadTextFile(path, true, '#', true);
        foreach(const QString& line, lines){
            QByteArrayList parts = line.toUtf8().split('\t');
            if (parts.count()==2){
                QByteArray sequence = parts[0];
                float score = parts[1].toFloat();
                result.insert(sequence, score);
            }
        }
        return result;
    }

    QHash<int,QHash<int,float>> read_matrix_3prime(const QByteArray& path) {
        QHash<int,QHash<int,float>> result;
        QHash<int,float> current_inner;
        QStringList lines = Helper::loadTextFile(path, true, '#', true);
        int last_index = -1;

        foreach(const QString& line, lines){
            QByteArrayList parts = line.toUtf8().split('\t');
            
            if (parts.count()==3){
                int index = parts[0].toInt();
                int sequence_hash = parts[1].toInt();
                float score = parts[2].toFloat();
                if (last_index != index) {
                    result.insert(last_index, current_inner);
                    current_inner = {{sequence_hash, score}};
                } else {
                    current_inner.insert(sequence_hash, score);
                }

                last_index = index;
            }
        }
        result.insert(last_index, current_inner);
        return result;
    }

};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

