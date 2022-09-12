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
		addFlag("report_ambiguous", "Report all matching genes for ambiguous previous/synonymous symbols - instead of an error.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QString in = getInfile("in");
		QString out = getOutfile("out");
		bool report_ambiguous = getFlag("report_ambiguous");
		if(in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}

		//process input
		QSharedPointer<QFile> instream = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(out, true);
		while(!instream->atEnd())
		{
			QString gene = instream->readLine().trimmed().toUpper();

			//skip empty/comment lines
			if (gene.isEmpty() || gene[0]=='#') continue;

			if (report_ambiguous)
			{
				QList<QPair<QByteArray, QByteArray>> gene_infos = db.geneToApprovedWithMessageAndAmbiguous(gene.toUtf8());
				foreach(auto gene_info, gene_infos)
				{
					outstream->write(gene_info.first + '\t' + gene_info.second + '\n');
				}
			}
			else
			{
				QPair<QString, QString> gene_info = db.geneToApprovedWithMessage(gene);
				outstream->write(gene_info.first.toUtf8() + '\t' + gene_info.second.toUtf8() + '\n');
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
