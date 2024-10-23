#include "ToolBase.h"
#include "NGSD.h"

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
		setDescription("Converts a text file with gene names to a TSV file with two columns (transcript, gene name).");
		addInfile("in", "Input TXT file with one gene symbol per line. If unset, reads from STDIN.", true, true);
		QStringList modes;
		modes << "all" << "best" << "relevant";
		addEnum("mode", "Mode: all = all transcripts, best = best transcript, relevant = all relevant transcripts.", false, modes);
		addFlag("version", "Append transcript version to transcript name.");
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true, true);
		addFlag("test", "Uses the test database instead of on the production database.");

		setExtendedDescription(QStringList() << "Best transcript is determined according this order: 'preferred transcript in NGSD', 'MANE select', 'Ensembl canonical', 'longest coding transcript', 'longest transcript'"
											 << "Relevant transcripts are: 'preferred transcript in NGSD', 'MANE select', 'MANE plus clinical', 'Ensembl canonical' (if none of those exist, the longest coding or longest transcript is used)");

		changeLog(2023,  5,  26, "First version.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString mode = getEnum("mode");
		bool version = getFlag("version");
		QString out = getOutfile("out");
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
		NGSD db(getFlag("test"));
		QTextStream error_stream(stderr);

		QStringList output;
		QSharedPointer<QFile> file = Helper::openFileForReading(in, true);
		QTextStream stream(file.data());
		while(!stream.atEnd())
		{
			QByteArray gene = stream.readLine().trimmed().toLatin1();

			if(gene.isEmpty() || gene.startsWith('#')) continue;

			//get gene ID in NGSD
			int gene_id = db.geneId(gene);
			if (gene_id==-1)
			{
				error_stream << "Gene symbol " + gene + " not found in NGSD!" << endl;
				continue;
			}

			//determine transcripts
			TranscriptList transcripts;
			if (mode=="best")
			{
				Transcript trans = db.bestTranscript(gene_id);
				if (trans.isValid()) //invalid transcript is returned if none is found
				{
					transcripts << trans;
				}
			}
			else if (mode=="relevant")
			{
				transcripts = db.relevantTranscripts(gene_id);
			}
			else //all
			{
				transcripts = db.transcripts(gene_id, Transcript::ENSEMBL, false);
			}

			//output
			foreach(const Transcript& trans, transcripts)
			{
				out_p->write((version ? trans.nameWithVersion() : trans.name()) + "\t" + gene + "\n");
			}
			if (transcripts.isEmpty())
			{
				error_stream << "No transcript found for gene " + gene + "!" << endl;
			}
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
