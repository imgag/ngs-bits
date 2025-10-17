#include "ToolBase.h"
#include "Log.h"
#include "BedFile.h"
#include "NGSD.h"

#include <QTextStream>

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
		setDescription("Converts a text file with gene names to a BED file.");
		addInfile("in", "Input TXT file with one gene symbol per line. If unset, reads from STDIN.", true, true);
		QStringList sources;
		sources << "ccds" << "ensembl";
		addEnum("source", "Transcript source database.", false, sources);
		QStringList modes;
		modes << "gene" << "exon";
		addEnum("mode", "Mode: gene = start/end of all transcripts, exon = start/end of all exons of all transcripts.", false, modes);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true, true);
		addFlag("fallback", "Allow fallback to all source databases, if no transcript for a gene is defined in the selected source database.");
		addFlag("anno", "Annotate transcript identifier in addition to gene name.");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2017,  2,  9, "Added option to annotate transcript names.");
	}

	virtual void main()
	{
		//init
		Transcript::SOURCE source = Transcript::stringToSource(getEnum("source"));
		QString mode = getEnum("mode");
		bool fallback = getFlag("fallback");
		bool anno = getFlag("anno");
		GeneSet genes = GeneSet::createFromFile(getInfile("in"));

		//process
		NGSD db(getFlag("test"));
		QTextStream messages(stderr);
		BedFile output = db.genesToRegions(genes, source, mode, fallback, anno, &messages);

		//store
		output.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
