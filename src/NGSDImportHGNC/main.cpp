#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
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
		setDescription("Imports genes from the HGNC flat file.");
		addInfile("in", "HGNC flat file (download ftp://ftp.ebi.ac.uk/pub/databases/genenames/hgnc/tsv/hgnc_complete_set.txt)", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");
	}

	void addAliases(SqlQuery& query, const QVariant& gene_id, const QByteArray& name_str, QVariant type)
	{
		QList<QByteArray> names = name_str.split('|');
		for(QByteArray name : names)
		{
			name.replace("\"", "");
			name = name.trimmed().toUpper();
			if (name.isEmpty()) continue;
			query.bindValue(0, gene_id);
			query.bindValue(1, name);
			query.bindValue(2, type);
			query.exec();
		}
	}


	//Update gene symbols in table with table_name. name for gene symbol column must be "symboL"
	void updateTable(NGSD& db, QString table_name)
	{
		QTextStream out(stdout);
		out << "Updating entries in " + table_name + " table..." << endl;
		//get all gene names
		SqlQuery query = db.getQuery();
		query.exec("SELECT symbol FROM " + table_name);
		QSet<QString> genes;
		while (query.next())
		{
			genes << query.value(0).toString();
		}

		//check if gene names are approved symbols
		int c_del = 0;
		int c_upd = 0;
		foreach(QString gene, genes)
		{
			auto approved = db.geneToApprovedWithMessage(gene);

			if (approved.second.startsWith("ERROR:"))
			{
				query.exec("DELETE FROM `" + table_name + "` WHERE symbol='" + gene + "'");
				++c_del;
			}
			if (approved.second.startsWith("REPLACED:"))
			{
				if (genes.contains(approved.first))
				{
					query.exec("DELETE FROM `" + table_name + "` WHERE symbol='" + gene + "'");
					++c_del;
				}
				else
				{
					query.exec("UPDATE " + table_name +" SET symbol='" + approved.first + "' WHERE symbol='" + gene + "'");
					++c_upd;
				}
			}
		}
		out << "  updated  " << c_upd << " entries" << endl;
		out << "  deleted  " << c_del << " entries" << endl;
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));

		//check tables exist
		db.tableExists("gene");
		db.tableExists("gene_alias");

		//clear tables if not empty
		if (!db.tableEmpty("gene") || !db.tableEmpty("gene_alias"))
		{
			if (getFlag("force"))
			{

				db.clearTable("gene_exon");
				db.clearTable("gene_transcript");
				db.clearTable("gene_alias");
				db.clearTable("gene_pseudogene_relation");
				db.clearTable("gene");
			}
			else
			{
				THROW(DatabaseException, "Tables already contain data! Use '-force' to overwrite old data!");
			}
		}

		//get enum data
		QStringList valid_types = db.getEnum("gene", "type");

		//prepare SQL queries
		SqlQuery gene_query = db.getQuery();
		gene_query.prepare("INSERT INTO gene (hgnc_id, symbol, name, type) VALUES (:0, :1, :2, :3);");
		SqlQuery alias_query = db.getQuery();
		alias_query.prepare("INSERT INTO gene_alias (gene_id, symbol, type) VALUES (:0, :1, :2);");

		//parse file
		QSharedPointer<QFile> fp = Helper::openFileForReading(getInfile("in"));
		while(!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty() || line.startsWith("hgnc_id")) continue;
			QList<QByteArray> parts = line.split('\t');
			if (parts.count()<11) THROW(FileParseException, "Invalid line (too few values): " + line);

			//check status
			QByteArray status  = parts[5];
			if (status=="Entry Withdrawn") continue;
			if (status!="Approved") THROW(FileParseException, "Unknown status '" + status + "' in line: " + line);

			//check locus
			QByteArray locus = parts[3];
			if (locus=="phenotype") continue;
			if (!valid_types.contains(locus)) THROW(FileParseException, "Unknown locus '" + locus + "' in line: " + line);

			//curate data
			QByteArray id = parts[0].mid(5);
			QByteArray symbol = parts[1].toUpper();

			//insert gene
			gene_query.bindValue(0, id);
			gene_query.bindValue(1, symbol);
			gene_query.bindValue(2, parts[2]);
			gene_query.bindValue(3, locus);
			gene_query.exec();
			QVariant gene_id = gene_query.lastInsertId();

			addAliases(alias_query, gene_id, parts[10], "previous");
			addAliases(alias_query, gene_id, parts[8], "synonym");
		}

		//update gene symbols in geneinfo_germline and somatic_gene_role table
		updateTable(db, "geneinfo_germline");
		updateTable(db, "somatic_gene_role");
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
