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
		QTextStream messages(stderr);
		NGSD db(getFlag("test"));

		//process input
		QSharedPointer<QFile> in = Helper::openFileForReading(getInfile("in"), true);
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		while(!in->atEnd())
		{
			QByteArray gene = in->readLine().trimmed().toUpper();

			//skip empty/comment lines
			if (gene.isEmpty() || gene[0]=='#') continue;

			//approved
			QPair<QByteArray, NGSD::ApprovedStatus> gene_info = db.geneToApproved(gene);
			switch (gene_info.second)
			{
				case NGSD::APPROVED:
					out->write(gene + '\n');
					break;
				case NGSD::PREVIOUS:
					out->write(gene_info.first + '\n');
					messages << "Notice: Gene name '" << gene + "' is a previous approved name. Replacing it by '" <<  gene_info.first << "'!\n";
					break;
				case NGSD::SYNONYM:
					out->write(gene_info.first + '\n');
					messages << "Notice: Gene name '" << gene + "' is a synonymous name. Replacing it by '" <<  gene_info.first << "'!\n";
					break;
				case NGSD::ERR_PREVIOUS:
					messages << "Warning: Gene name '" << gene << "' is a previous name for more than one gene.\n";
					out->write(gene + '\n');
					break;
				case NGSD::ERR_SYNONYM:
					messages << "Warning: Gene name '" << gene << "' is a synonymous name for more than one gene.\n";
					out->write(gene + '\n');
					break;
				case NGSD::ERR_UNKNOWN:
					messages << "Warning: Gene name '" << gene << "' not found in HGNC database. Skipping it!\n";
					break;
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
