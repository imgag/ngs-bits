#include "ToolBase.h"
#include "NGSD.h"
#include "Helper.h"

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
		setDescription("Imports gene-specific information into NGSD.");
		addInfile("constraint", "ExAC gene contraint file from 'ftp://ftp.broadinstitute.org/pub/ExAC_release/current/functional_gene_constraint/'.", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);

		//check tables exist
		db.tableExists("geneinfo_germline");

		//update gene names to approved symbols
		out << "Updating gene names..." << endl;
		SqlQuery query = db.getQuery();
		query.exec("SELECT symbol FROM geneinfo_germline WHERE symbol NOT IN (SELECT symbol FROM gene)");
		while(query.next())
		{
			QString symbol = query.value(0).toString();
			auto approved = db.geneToApproved(symbol);
			if (!approved.second.startsWith("KEPT:"))
			{
				out << "  skipped " << symbol << ": " << approved.second << endl;
			}
		}
		out << endl;

		//import ExAC pLI scores
		out << "Importing ExAC pLI scores..." << endl;

		SqlQuery update_query = db.getQuery();
		update_query.prepare("INSERT INTO geneinfo_germline (symbol, inheritance, exac_pli, comments) VALUES (:0, 'n/a', :1, '') ON DUPLICATE KEY UPDATE exac_pli=:2");

		auto file = Helper::openFileForReading(getInfile("constraint"));
		while(!file->atEnd())
		{
			QString line = file->readLine().trimmed();
			if (line.isEmpty() || line.startsWith("transcript\t")) continue;

			QStringList parts = line.split('\t');
			if (parts.count()<22) continue;

			//gene
			QString gene = parts[1];
			auto approved = db.geneToApproved(gene);
			if (approved.second.startsWith("ERROR:"))
			{
				out << "  skipped " << gene << ": " << approved.second << endl;
				continue;
			}
			gene = approved.first;
			update_query.bindValue(0, gene);

			//pLI
			double pLI = Helper::toDouble(parts[19], "ExAC pLI score");
			update_query.bindValue(1, pLI);
			update_query.bindValue(2, pLI);
			update_query.exec();
		}
		out << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
