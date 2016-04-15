#include "Exceptions.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Log.h"
#include "BedFile.h"
#include "Settings.h"
#include "NGSD.h"
#include <QSet>
#include <QFile>
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
		sources << "ccds" << "ucsc";
		addEnum("source", "Transcript source database.", false, sources);
		QStringList modes;
		modes << "gene" << "exon";
		addEnum("mode", "Mode: gene = start/end of gene, exon = start/end of all exons of all splice variants.", false, modes);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true, true);
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		QString source = getEnum("source");
		QString mode = getEnum("mode");
		QSharedPointer<QFile> infile = Helper::openFileForReading(getInfile("in"), true);
		QStringList genes = Helper::loadTextFile(infile, true, '#', true);

		//process
		NGSD db(getFlag("test"));
		QTextStream messages(stderr);
		BedFile output = db.genesToRegions(genes, source, mode, &messages);

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
