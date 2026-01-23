#include "ToolBase.h"
#include "NGSD.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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
		setDescription("Import genes with special interpretation guidelines from CSpect.");
		addInfile("in", "CSpect data JSON downloaded from 'https://cspec.genome.network/cspec/SequenceVariantInterpretation/id?detail=high&fields=ld.RuleSet,ldFor.Organization,entContent.states,entContent.legacyFullySuperseded,entContent.legacyReplaced,entId,ldhId,entContent.title&pgSize=1000'", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));

		//clear tables if not empty
		db.tableExists("cspec_data");
		if (!db.tableEmpty("cspec_data")) db.clearTable("cspec_data");

		//prepare SQL query
		SqlQuery q_ins = db.getQuery();
		q_ins.prepare("INSERT INTO cspec_data (gene) VALUES (:0);");

		//determine unique set of genes
		GeneSet genes;
		GeneSet genes_skipped;
		QSharedPointer<QFile> file_ptr = Helper::openFileForReading(getInfile("in"));
		QJsonDocument doc = QJsonDocument::fromJson(file_ptr->readAll());
		QJsonArray data = doc.object()["data"].toArray();
		for (int i=0; i<data.count(); ++i)
		{
			QJsonArray ruleset_array = data[i].toObject()["ld"].toObject()["RuleSet"].toArray();
			for (int j=0; j<ruleset_array.count(); ++j)
			{
				QJsonArray genes_array = ruleset_array[j].toObject()["entContent"].toObject()["genes"].toArray();
				for (int k=0; k<genes_array.count(); ++k)
				{
					QByteArray gene = genes_array[j].toObject()["gene"].toString().toUtf8();

					//convert to approved gene
					QByteArray gene_approved = db.geneToApproved(gene);
					if (gene_approved.isEmpty())
					{
						genes_skipped << gene;
						continue;
					}

					genes << gene_approved;
				}
			}
		}
		for(const QByteArray& gene : std::as_const(genes))
		{
			q_ins.bindValue(0, gene);
			q_ins.exec();
		}
		QTextStream out(stdout);
		out << "Parsed rulesets: " << data.count() << "\n";
		out << "Imported genes: " << genes.count() << "\n";
		out << "Skipped genes (not convertable to approved symbol): " << genes_skipped.count() << "\n";

		//add DB import info
		QString version = doc.object()["metadata"].toObject()["rendered"].toObject()["when"].toString().left(10);
		db.setDatabaseInfo("CSpec", version);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
