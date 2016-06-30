#include "ToolBase.h"
#include "Helper.h"
#include "NGSD.h"
#include "Exceptions.h"
#include <QFile>


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
		setDescription("Replaces gene symbols by approved symbols using the HGNC database.");
		//optional
		addInfile("in", "Input TXT file with one gene symbol per line. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output TXT file with approved gene symbols. If unset, writes to STDOUT.", true);
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QString in = getInfile("in");
		QString out = getOutfile("out");
		if(in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}

		//process input
		QSharedPointer<QFile> instream = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(out, true);
		while(!instream->atEnd())
		{
			QByteArray gene = instream->readLine().trimmed().toUpper();

			//skip empty/comment lines
			if (gene.isEmpty() || gene[0]=='#') continue;

			QPair<QByteArray, QByteArray> gene_info = db.geneToApproved(gene);
			outstream->write(gene_info.first + '\t' + gene_info.second + '\n');
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
