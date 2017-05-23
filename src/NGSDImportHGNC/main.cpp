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
		addInfile("in", "HGNC flat file (download and unzip ftp://ftp.ebi.ac.uk/pub/databases/genenames/hgnc_complete_set.txt.gz)", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");
	}

	void addAliases(SqlQuery& query, const QVariant& gene_id, const QByteArray& name_str, QVariant type)
	{
		QList<QByteArray> names = name_str.split(',');
		for(QByteArray name : names)
		{
			name = name.trimmed().toUpper();
			if (name.isEmpty()) continue;
			query.bindValue(0, gene_id);
			query.bindValue(1, name);
			query.bindValue(2, type);
			query.exec();
		}
	}

	void updateGeneinfoGermline(NGSD& db)
	{
		QTextStream out(stdout);
		out << "Updating entries in geneinfo_germline table..." << endl;
		//get all gene names
		SqlQuery query = db.getQuery();
		query.exec("SELECT symbol FROM geneinfo_germline");
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
			auto approved = db.geneToApproved(gene);

			if (approved.second.startsWith("ERROR:"))
			{
				query.exec("DELETE FROM `geneinfo_germline` WHERE symbol='" + gene + "'");
				++c_del;
			}
			if (approved.second.startsWith("REPLACED:"))
			{
				if (genes.contains(approved.first))
				{
					query.exec("DELETE FROM `geneinfo_germline` WHERE symbol='" + gene + "'");
					++c_del;
				}
				else
				{
					query.exec("UPDATE geneinfo_germline SET symbol='" + approved.first + "' WHERE symbol='" + gene + "'");
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
				db.clearTable("gene");
			}
			else
			{
				THROW(DatabaseException, "Tables already contain data! Use '-force' to overwrite old data!");
			}
		}

		//get enum data
		QStringList valid_chrs = db.getEnum("gene", "chromosome");
		QStringList valid_types = db.getEnum("gene", "type");

		//prepare SQL queries
		SqlQuery gene_query = db.getQuery();
		gene_query.prepare("INSERT INTO gene (hgnc_id, symbol, name, chromosome, type) VALUES (:0, :1, :2, :3, :4);");
		SqlQuery alias_query = db.getQuery();
		alias_query.prepare("INSERT INTO gene_alias (gene_id, symbol, type) VALUES (:0, :1, :2);");

		//parse file
		QSharedPointer<QFile> fp = Helper::openFileForReading(getInfile("in"));
		while(!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty() || line.startsWith("HGNC ID")) continue;
			QList<QByteArray> parts = line.split('\t');
			if (parts.count()<11) THROW(FileParseException, "Invalid line (too few values): " + line);

			//check status
			QByteArray status  = parts[3];
			if (status=="Entry Withdrawn" || status=="Symbol Withdrawn") continue;
			if (status!="Approved") THROW(FileParseException, "Unknown status '" + status + "' in line: " + line);

			//check locus
			QByteArray locus = parts[5];
			if (locus=="phenotype") continue;
			if (!valid_types.contains(locus)) THROW(FileParseException, "Unknown locus '" + locus + "' in line: " + line);

			//curate data
			QByteArray id = parts[0].mid(5);
			QByteArray symbol = parts[1].toUpper();
			QByteArray chr = parts[10].replace('q', ' ').replace('p', ' ').replace("cen", " ").replace("mitochondria", "MT").append(' ');
			chr = chr.left(chr.indexOf(' ')).trimmed();
			if (!valid_chrs.contains(chr)) chr = "none";

			//insert gene
			gene_query.bindValue(0, id);
			gene_query.bindValue(1, symbol);
			gene_query.bindValue(2, parts[2]);
			gene_query.bindValue(3, chr);
			gene_query.bindValue(4, locus);
			gene_query.exec();
			QVariant gene_id = gene_query.lastInsertId();

			addAliases(alias_query, gene_id, parts[6], "previous");
			addAliases(alias_query, gene_id, parts[8], "synonym");
		}

		//update gene symbols in geneinfo_germline table
		updateGeneinfoGermline(db);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
