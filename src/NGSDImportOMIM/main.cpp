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
		setDescription("Imports OMIM genes/phenotypes into the NGSD.");
		setExtendedDescription(QStringList() << "Note: This is an optional step since you might need and have a license for OMIM download.");
		addInfile("gene", "OMIM 'mim2gene.txt' file from 'http://omim.org/downloads/'.", false);
		addInfile("morbid", "OMIM 'morbidmap.txt' file from 'http://omim.org/downloads/'.", false);
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
		db.tableExists("omim_gene");
		db.tableExists("omim_phenotype");

		//clear tables if not empty
		if (!db.tableEmpty("omim_gene") || !db.tableEmpty("omim_phenotype"))
		{
			if (getFlag("force"))
			{
				db.clearTable("omim_phenotype");
				db.clearTable("omim_gene");
			}
			else
			{
				THROW(DatabaseException, "Tables already contain data! Use '-force' to overwrite old data!");
			}
		}

		//prepare SQL queries
		SqlQuery qi_gene = db.getQuery();
		qi_gene.prepare("INSERT INTO omim_gene (gene, mim) VALUES (:0, :1);");
		SqlQuery qi_pheno = db.getQuery();
		qi_pheno.prepare("INSERT INTO omim_phenotype (omim_gene_id, phenotype) VALUES (:0, :1);");

		//import genes
        out << "Importing OMIM genes..." << Qt::endl;
		QMap<QByteArray, QByteArray> mim2gene_id;
		QSharedPointer<QFile> fp = Helper::openFileForReading(getInfile("gene"));
		while(!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty() || line[0]=='#') continue;

			QByteArrayList parts = line.split('\t'); //mim, type, NCBI ID, HGNC symbol, Ensembl ID
			if (parts.count()<4) continue;

			//check type "gene"
			QByteArray type = parts[1].trimmed();
			if (!type.contains("gene")) continue;

			//check gene
			QByteArray gene = parts[3].trimmed();
			if (gene.isEmpty()) continue;

			//convert gene to approved symbol
			QByteArray gene_approved = db.geneToApproved(gene);
			if (gene_approved.isEmpty())
			{
                out << "Gene '" << gene << "' could not be converted to approved symbol! Using it anyway." << Qt::endl;
				gene_approved = gene;
			}

			//insert
			QByteArray mim = parts[0];
			qi_gene.bindValue(0, gene_approved);
			qi_gene.bindValue(1, mim);
			qi_gene.exec();

			//cache
			mim2gene_id[mim] = qi_gene.lastInsertId().toByteArray();
		}
		fp->close();

		//output
		int c_genes = db.getValues("SELECT gene FROM omim_gene").count();
		int c_genes_distinct = db.getValues("SELECT DISTINCT gene FROM omim_gene").count();
        out << "Imported " << c_genes << " genes (" << (c_genes-c_genes_distinct) << " duplicate genes)" << Qt::endl;

		//import gene-phenotype relations
        out << Qt::endl;
        out << "Importing OMIM gene-phenotype relations..." << Qt::endl;
		fp = Helper::openFileForReading(getInfile("morbid"));
		while(!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty() || line[0]=='#') continue;

			QByteArrayList parts = line.split('\t'); //phenotype, gene symbols, gene MIM, cyto location
			if (parts.count()<3) continue;

			//check phenotype
			QByteArray phenotype = parts[0].trimmed();
			if (phenotype.isEmpty()) continue;

			//check gene MIM
			QByteArray gene_mim = parts[2].trimmed();
			if (gene_mim.isEmpty()) continue;
			if (!mim2gene_id.contains(gene_mim)) continue;

			//insert
			qi_pheno.bindValue(0, mim2gene_id[gene_mim]);
			qi_pheno.bindValue(1, phenotype);
			qi_pheno.exec();
		}
		fp->close();


		//output
        out << "Imported " << db.getValue("SELECT COUNT(*) FROM omim_phenotype").toInt() << " phenotypes" << Qt::endl;
		int c_genes_pheno = db.getValues("SELECT DISTINCT omim_gene_id FROM omim_phenotype").count();
        out << c_genes_pheno << " out of the " << c_genes << " genes have phenotype information" << Qt::endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
