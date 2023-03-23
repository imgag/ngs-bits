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
		addInfile("ensembl", "Ensembl gene file (gff3) to resolve duplicate ENSG identifier (same as NGSDImportEnsembl 'in' parameter).", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");
	}

	void addAliases(SqlQuery& query, const QVariant& gene_id, const QByteArray& name_str, QVariant type)
	{
		QList<QByteArray> names = name_str.split('|');
		foreach(QByteArray name, names)
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


	//Get Ensembl - HGNC mapping for duplicate ENSG ids:
	QMap<QByteArray,QByteArray> getEnsemblHGNCMapping(const QSet<QByteArray>& duplicate_ensg_ids, const QString& ensembl_file_path)
	{
		QMap<QByteArray,QByteArray> ensg_hgnc_mapping;
		QSharedPointer<QFile> fp = Helper::openFileForReading(ensembl_file_path);
		while (!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty() || line.startsWith("#")) continue;
			QList<QByteArray> parts = line.split('\t');
			if (parts.size() < 9) THROW(FileParseException, "Invalid line (too few values): " + line);
			const QByteArray& comment = parts[8];

			// skip none gene lines
			if(!comment.startsWith("ID=gene:")) continue;

			// skip genes
			QByteArray ensg_id = comment.split(';').at(0).split(':').at(1).trimmed();
			if(!duplicate_ensg_ids.contains(ensg_id)) continue;

			// parse comment entries
			QByteArrayList comments = comment.split(';');
			foreach (const QByteArray& kv_pair, comments)
			{
				if(kv_pair.startsWith("description="))
				{
					//extract HGNC number
					if(!kv_pair.contains("Source:HGNC Symbol%3BAcc:HGNC")) THROW(FileParseException, "Gene line doesn't contain HGNC identifier: " + line);
					QByteArray hgnc = kv_pair.split('[').at(1).split(']').at(0).split(':').last();
					ensg_hgnc_mapping.insert(ensg_id, "HGNC:" + hgnc);
					break;
				}
			}

			//skip if all ids have been found
			if (ensg_hgnc_mapping.size() == duplicate_ensg_ids.size()) break;
		}

		if (ensg_hgnc_mapping.size() != duplicate_ensg_ids.size()) THROW(FileParseException, "Couldn't find all duplicate ENSG ids in ensembl file.")

		return ensg_hgnc_mapping;
	}


	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);

		//check tables exist
		db.tableExists("gene");
		db.tableExists("gene_alias");

		//start transaction
		db.transaction();

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



		//debug
		QSet<QByteArray> ensg_ids;
		QSet<QByteArray> duplicate_ensg_ids;

		//parse file
		QSharedPointer<QFile> fp = Helper::openFileForReading(getInfile("in"));

		//get duplicate ENSG ids
		out << "Extract duplicate ENSG ids..." << endl;
		while (!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty() || line.startsWith("hgnc_id")) continue;
			QList<QByteArray> parts = line.split('\t');
			if (parts.count()<11) THROW(FileParseException, "Invalid line (too few values): " + line);
			// skip lines with no ENSG id
			if (parts.size() < 20) continue;
			if (parts[19].trimmed().isEmpty()) continue;

			QByteArray ensg_id = parts[19].trimmed();

			if(ensg_ids.contains(ensg_id))
			{
				duplicate_ensg_ids.insert(ensg_id);
			}
			else
			{
				ensg_ids.insert(ensg_id);
			}
		}
		out << "Multiple ENSG ids (" << QByteArray::number(duplicate_ensg_ids.size()) << "): " << duplicate_ensg_ids.toList().join(", ") << endl;


		//get ENSG->HGNC mapping from ensembl file
		out << "Get ENSG -> HGNC mapping from ensembl file..." << endl;
		QMap<QByteArray,QByteArray> ensg_hgnc_mapping;
		if (duplicate_ensg_ids.size() > 0) ensg_hgnc_mapping = getEnsemblHGNCMapping(duplicate_ensg_ids, getInfile("ensembl"));

		//final parse for import
		out << "Parse HGNC file and import genes into the NGSD..." << endl;
		fp->seek(0);
		int n_imported = 0;

		//prepare SQL queries
		SqlQuery gene_query = db.getQuery();
		gene_query.prepare("INSERT INTO gene (hgnc_id, symbol, name, type, ensembl_id, ncbi_id) VALUES (:0, :1, :2, :3, :4, :5);");
		SqlQuery alias_query = db.getQuery();
		alias_query.prepare("INSERT INTO gene_alias (gene_id, symbol, type) VALUES (:0, :1, :2);");

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

			//extract Ensembl ID:
			QVariant ensg_id = QVariant();
			if ((parts.size() > 19) && (!parts[19].trimmed().isEmpty()))
			{
				ensg_id = QVariant(parts[19].trimmed());

				// remove ENSG id of duplicates if it is not linked in the ensembl file
				if (duplicate_ensg_ids.contains(ensg_id.toByteArray()))
				{
					if(ensg_hgnc_mapping.value(id) != ensg_id.toByteArray())
					{
						ensg_id = QVariant();
					}
				}
			}

			//extract ncbi id (entrez_id):
			QVariant ncbi_id;
			QByteArray entrez_id = parts[18].trimmed();
			if (!entrez_id.isEmpty())
			{
				ncbi_id = Helper::toInt(entrez_id, "entrez_id", line);
			}

			//insert gene
			gene_query.bindValue(0, id);
			gene_query.bindValue(1, symbol);
			gene_query.bindValue(2, parts[2]);
			gene_query.bindValue(3, locus);
			gene_query.bindValue(4, ensg_id);
			gene_query.bindValue(5, ncbi_id);
			gene_query.exec();
			QVariant gene_id = gene_query.lastInsertId();
			n_imported++;

			addAliases(alias_query, gene_id, parts[10], "previous");
			addAliases(alias_query, gene_id, parts[8], "synonym");
		}

		out << "  " << n_imported << " genes imported into the NGSD" << endl;

		//update gene symbols in geneinfo_germline and somatic_gene_role table
		updateTable(db, "geneinfo_germline");
		updateTable(db, "expression");
		updateTable(db, "somatic_gene_role");
		updateTable(db, "somatic_pathway_gene");

		//commit changes
		db.commit();

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
