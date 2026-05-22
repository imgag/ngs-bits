#include "ToolBase.h"
#include "Helper.h"
#include "Transcript.h"
#include "NGSD.h"
#include <QDate>
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
		setDescription("Computes the protein sequence for each transcript name given.");
		addOutfile("out", "Output TSV file.", false);
		//optional
		addInfile("in", "Input file. If unset, reads from STDIN. Expects one transcript ID (ENSEMBLE or REFSEQ) per line.", true);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addFlag("test", "Uses the test database instead of on the production database.");
		QStringList builds;
		builds << "hg19" << "hg38";
		addEnum("build", "Genome build", true, builds, "hg38");

		changeLog(2026,  5, 4, "Initial version");
	}



	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		FastaFileIndex ref_index(ref_file);

		QSharedPointer<QFile> instream = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(out, false);

		if (getEnum("build") == "hg19")
		{
			QTextStream out(stderr);
			out << "Warning: When using the hg19 build, it is neccessary to also use a NGSD instance containing hg19 data and a hg19 reference genome.\n";
		}

		QStringList tsv_headers = {"", ""}; //fallback in case there is no header
		NGSD db(getFlag("test"));

		while (!instream->atEnd())
		{
			QByteArray line = instream->readLine();
			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

			//ignore empty lines and comments at the start of the file
			if (line.trimmed().isEmpty()) continue;
			//ignore headers
			if (line.startsWith('#')) continue;

			int transcript_id = db.transcriptId(line, false);
			if (transcript_id == -1)
			{
				QTextStream out(stderr);
				out << line << "\t Skipped: no matching transcript found in NGSD\n";
				continue;
			}

			Transcript transcript = db.transcript(transcript_id);

			if (transcript.biotype() != Transcript::PROTEIN_CODING)
			{
				QTextStream out(stderr);
				out << line << "\t Skipped: Transcript is not protein coding\n";
				continue;
			}
			QByteArray protein_sequence;
			try
			{
				protein_sequence = transcript.proteinSequence(ref_index, false, true);
			}
			catch (Exception e)
			{
				QTextStream out(stderr);
				out << line << "\t Skipped: Error converting dna sequence to protein sequence: " + e.message() + "\n";
				continue;
			}

			outstream->write(line + "\t" + protein_sequence +"\n");
		}
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

