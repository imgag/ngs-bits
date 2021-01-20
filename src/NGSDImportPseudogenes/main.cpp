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
		setDescription("Imports gene-pseudogene relation into NGSD.");
		addInfile("ensembl", "Ensembl transcript file (download and unzip ftp://ftp.ensembl.org/pub/grch37/release-87/gff3/homo_sapiens/Homo_sapiens.GRCh37.87.chr.gff3.gz).", false);
		addInfile("hgnc", "HGNC flat file (download ftp://ftp.ebi.ac.uk/pub/databases/genenames/hgnc/tsv/hgnc_complete_set.txt)", false);
		addInfile("pseudogene", "Pseudogene flat file (download http://www.pseudogene.org/Human/Human90.txt)", false);
		addOutfile("out", "Output file prefix", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
//		addFlag("force", "If set, overwrites old data.");

		changeLog(2021,  1,  18, "Added first version");
	}

//	void parse_ensemble_file(QMultiMap<QByteArray,int>& ensembl_hgnc_relation, QList<int>& pseudogenes, QString ensembl_file_path)
//	{
//		QSharedPointer<QFile> ensembl_fp = Helper::openFileForReading(ensembl_file_path);

//		// stats
//		int missing_hgnc_ids = 0;
//		int n_genes = 0;
//		int n_pseudogenes = 0;
//		QTextStream out(stdout);
//		while(!ensembl_fp->atEnd())
//		{
//			QByteArray line = ensembl_fp->readLine().trimmed();
//			if (line.isEmpty() || line.startsWith("#")) continue;
//			QByteArrayList parts = line.split('\t');
//			n_genes ++;

//			// extract ensembl gene identifier and HGNC id:
//			if (parts.at(8).startsWith("ID=gene:"))
//			{
//				QByteArrayList key_value_list = parts.at(8).split(';');
//				QByteArray ensembl_id = key_value_list.at(0).split(':').at(1).trimmed();

//				// get HGNC id
//				int hgnc_id = -1;
//				foreach (const QByteArray& key_value_pair, key_value_list)
//				{
//					if(key_value_pair.startsWith("description="))
//					{
//						//extract HGNC identifier
//						int start = key_value_pair.indexOf("[Source:HGNC Symbol%3BAcc:HGNC:");
//						if (start==-1)
//						{
//							missing_hgnc_ids++;
//							continue;
//						}
//						start += 31;
//						int end = key_value_pair.indexOf("]", start);
//						if (end==-1)
//						{
//							missing_hgnc_ids++;
//							continue;
//						}
//						hgnc_id = Helper::toInt(key_value_pair.mid(start, end-start), ensembl_id);
//						// store gene-pseudogene relation
//						ensembl_hgnc_relation.insert(ensembl_id, hgnc_id);

//						// store hgnc id for pseudo genes
//						if (parts.at(2).trimmed() == "pseudogene")
//						{
//							pseudogenes.append(hgnc_id);
//							n_pseudogenes++;
//							break;
//						}
//					}
//				}
//				if (hgnc_id == -1) missing_hgnc_ids++;
//			}
//		}

//		// print stats:
//		out << "ensembl flat file:\n";
//		out << "\t gene entries: " << n_genes << "\n";
//		out << "\t genes without hgnc id: " << missing_hgnc_ids << "\n";
//		out << "\t pseudogenes: " << n_pseudogenes << "\n";
//	}

//	void parse_hgnc_file(QMultiMap<int, QByteArray>& hgnc_pseudogene_relation, QString hgnc_file_path)
//	{
//		//parse file
//		QSharedPointer<QFile> hgnc_fp = Helper::openFileForReading(hgnc_file_path);

//		// stats
//		int n_hgnc = 0;
//		int n_hgnc_pseudogenes = 0;
//		QTextStream out(stdout);
//		while(!hgnc_fp->atEnd())
//		{
//			QByteArray line = hgnc_fp->readLine().trimmed();
//			if (line.isEmpty() || line.startsWith("#") || line.startsWith("HGNC ID")) continue;
//			QByteArrayList parts = line.split('\t');

//			// extract HGNC id
//			int hgnc_id = Helper::toInt(parts.at(0).split(':').at(1));
//			n_hgnc++;

//			// extract pseudogene id
//			QByteArrayList special_db_link_list = parts.at(20).split(' ');
//			QByteArray pseudogene_id;
//			foreach (const QByteArray& link, special_db_link_list)
//			{
//				if (link.startsWith("PGOHUM"))
//				{
//					pseudogene_id = link.split('<').at(0).trimmed();
//					hgnc_pseudogene_relation.insert(hgnc_id, pseudogene_id);
//					n_hgnc_pseudogenes++;
//					break;
//				}
//			}
//		}
//		// print stats:
//		out << "HGNC flat file:\n";
//		out << "\t hgnc entries: " << n_hgnc << "\n";
//		out << "\t hgnc entries with pseudogene id: " << n_hgnc_pseudogenes << "\n";
//	}

//	void parse_pseudogene_file(QMultiMap<QByteArray, QByteArray>& pseudogene_ensembl_relation, QString pseudogene_file_path)
//	{
//		QSharedPointer<QFile> pseudogene_fp = Helper::openFileForReading(pseudogene_file_path);

//		// stats
//		int n_pseudogenes = 0;
//		QTextStream out(stdout);
//		while(!pseudogene_fp->atEnd())
//		{
//			QByteArray line = pseudogene_fp->readLine().trimmed();
//			if (line.isEmpty() || line.startsWith("#")) continue;
//			QByteArrayList parts = line.split('\t');

//			QByteArray pseudogene_id = parts.at(0).trimmed();
//			QByteArray ensembl_id = parts.at(8).trimmed();
//			pseudogene_ensembl_relation.insert(pseudogene_id, ensembl_id);
//			n_pseudogenes++;
//		}
//		// print stats:
//		out << "pseudogene flat file:\n";
//		out << "\t pseudogene relations: " << n_pseudogenes << "\n";
//	}

//	void parse_transcript_hgnc_relation(QMap<QByteArray, QByteArray>& transcript_gene_relation, QMap<QByteArray, int>& gene_hgnc_relation, QString ensembl_file_path)
//	{
//		QSharedPointer<QFile> ensembl_fp = Helper::openFileForReading(ensembl_file_path);

//		// stats
//		QTextStream out(stdout);
//		int n_missing_hgnc_id = 0;
//		int n_missing_parent_gene = 0;
		
//		while(!ensembl_fp->atEnd())
//		{
//			QByteArray line = ensembl_fp->readLine().trimmed();
//			if (line.isEmpty() || line.startsWith("#")) continue;
//			QByteArrayList parts = line.split('\t');

//			// extract ensembl gene identifier and HGNC id:
//			if (parts.at(8).startsWith("ID=gene:"))
//			{
//				QByteArrayList key_value_list = parts.at(8).split(';');
//				QByteArray ensembl_gene_id = key_value_list.at(0).split(':').at(1).trimmed();

//				// get HGNC id
//				int hgnc_id = -1;
//				foreach (const QByteArray& key_value_pair, key_value_list)
//				{
//					if(key_value_pair.startsWith("description="))
//					{
//						//extract HGNC identifier
//						int start = key_value_pair.indexOf("[Source:HGNC Symbol%3BAcc:HGNC:");
//						if (start==-1)
//						{
//							n_missing_hgnc_id++;
//							continue;
//						}
//						start += 31;
//						int end = key_value_pair.indexOf("]", start);
//						if (end==-1)
//						{
//							n_missing_hgnc_id++;
//							continue;
//						}
//						hgnc_id = Helper::toInt(key_value_pair.mid(start, end-start), ensembl_gene_id);
//						// store gene-pseudogene relation
//						gene_hgnc_relation.insert(ensembl_gene_id, hgnc_id);
//						break;
//					}
//				}
//				// description missing
//				if (hgnc_id == -1) n_missing_hgnc_id++;
//			}
//			else if (parts.at(8).startsWith("ID=transcript:"))
//			{
//				// parse transcript --> gene
//				QByteArrayList key_value_list = parts.at(8).split(';');
//				QByteArray ensembl_transcript_id = key_value_list.at(0).split(':').at(1).trimmed();
				
//				// get parent gene id
//				QByteArray parent_gene_id;
//				foreach (const QByteArray& key_value_pair, key_value_list)
//				{
//					if(key_value_pair.startsWith("Parent=gene:"))
//					{
//						//extract ensemble gene identifier
//						parent_gene_id = key_value_pair.split(':').at(1).trimmed();
//						// store transcript-gene relation
//						gene_hgnc_relation.insert(ensembl_transcript_id, parent_gene_id);
//						break;
//					}
//				}
//				// parent gene missing
//				if ( parent_gene_id.isEmpty()) n_missing_parent_gene++;
//			}
//		}

//		// print stats:
//		out << "ensembl flat file:\n";
//		out << "\t gene entries: " << n_genes << "\n";
//		out << "\t genes without hgnc id: " << missing_hgnc_ids << "\n";
//		out << "\t pseudogenes: " << n_pseudogenes << "\n";
//	}

	
	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);		

		//get input files
		QString pseudogene_file_path = getInfile("pseudogene");

		// prepare db queries


		// parse pseudogene file
		QSharedPointer<QFile> pseudogene_fp = Helper::openFileForReading(pseudogene_file_path);

		// stats
		int n_missing_pseudogene_transcript_id = 0;
		int n_missing_parent = 0;
		int n_missing_parent_transcript_id = 0;
		int n_found_transcript_relations = 0;

		while(!pseudogene_fp->atEnd())
		{
			QByteArray line = pseudogene_fp->readLine().trimmed();
			if (line.isEmpty() || line.startsWith("#") || line.startsWith("Pseudogene_id")) continue;
			QByteArrayList parts = line.split('\t');

			// parse ensembl transcript ids
			QByteArray pseudogene_transcript_ensembl_id = parts.at(0).split('.').at(0).trimmed();
			QByteArray parent_transcript_ensembl_id = parts.at(7).split('.').at(0).trimmed();

			if (parent_transcript_ensembl_id.isEmpty())
			{
				n_missing_parent++;
				continue;
			}

			// get corresponding gene id
			int pseudogene_transcript_id = db.transcriptId(pseudogene_transcript_ensembl_id, false);
			if (pseudogene_transcript_id == -1)
			{
				out << "No pseudogene transcript with name '" << pseudogene_transcript_ensembl_id << "' found in NGSD!\n";
				n_missing_pseudogene_transcript_id++;
			}
			int parent_transcript_id = db.transcriptId(parent_transcript_ensembl_id, false);
			if (parent_transcript_id == -1)
			{
				out << "No parenttranscript with name '" << parent_transcript_ensembl_id << "' found in NGSD!\n";
				n_missing_parent_transcript_id++;
			}

			if ((pseudogene_transcript_id != -1) && (parent_transcript_id != -1))
			{
				n_found_transcript_relations++;
			}
		}
		// print stats:
		out << "pseudogene flat file:\n";
		out << "\t missing parent transcript ids in File: " << n_missing_parent << "\n";
		out << "\t missing pseudogene transcript ids in NGSD: " << n_missing_pseudogene_transcript_id << "\n";
		out << "\t missing parent transcript ids in NGSD: " << n_missing_parent_transcript_id << "\n";
		out << "\n\t found transcript relations in NGSD: " << n_found_transcript_relations << "\n";



//		//extract (pseudo-)genes from ensembl file
//		QMultiMap<QByteArray,int> ensembl_hgnc_relation;
//		QList<int> pseudogenes; //list of pseudogenes as hgnc id list
//		parse_ensemble_file(ensembl_hgnc_relation, pseudogenes, ensembl_file_path);

//		// extract pseudogene id from HGNC file:
//		QMultiMap<int, QByteArray> hgnc_pseudogene_relation;
//		parse_hgnc_file(hgnc_pseudogene_relation, hgnc_file_path);

//		// import pseudogene-ensembl relation:
//		QMultiMap<QByteArray, QByteArray> pseudogene_ensembl_relation;
//		parse_pseudogene_file(pseudogene_ensembl_relation, pseudogene_file_path);


//		// combine QMaps to pseudogene-gene relation
//		QList<QPair<int,int>> pseudogene_gene_relation = QList<QPair<int,int>>();
//		//TODO: remove
//		QByteArrayList test;

//		//stats
//		int missing_pseudogene_relation = 0;
//		int missing_ensembl_relation = 0;
//		int missing_hgnc_relation = 0;
//		foreach (int hgnc_id_pseudogene, pseudogenes)
//		{
//			if (hgnc_pseudogene_relation.contains(hgnc_id_pseudogene))
//			{
//				foreach (const QByteArray& pseudogene_id, hgnc_pseudogene_relation.values(hgnc_id_pseudogene))
//				{
//					if (pseudogene_ensembl_relation.contains(pseudogene_id))
//					{
//						foreach (const QByteArray&  ensembl_id,  pseudogene_ensembl_relation.values(pseudogene_id))
//						{
//							if (ensembl_hgnc_relation.contains(ensembl_id))
//							{
//								foreach (int hgnc_id_gene, ensembl_hgnc_relation.values(ensembl_id))
//								{
//									pseudogene_gene_relation.append(QPair<int,int>(hgnc_id_pseudogene, hgnc_id_gene));
//								}
//							}
//							else
//							{
//								missing_hgnc_relation++;
//							}
//						}
//					}
//					else
//					{
//						missing_ensembl_relation++;
//						test.append(pseudogene_id);
//					}
//				}
//			}
//			else
//			{
//				missing_pseudogene_relation++;
//			}
//		}

//		// print stats:
//		out << "resulting gene-gene relation:\n";
//		out << "\t missing_pseudogene_relation: " << missing_pseudogene_relation << "\n";
//		out << "\t missing_ensembl_relation: " << missing_ensembl_relation << "\n";
//		out << "\t missing_hgnc_relation: " << missing_hgnc_relation << "\n";
//		out << "\t pseudogene-gene relation: " << pseudogene_gene_relation.size() << "\n";

//		//TODO: remove
//		//debug test file
//		QSharedPointer<QFile> test_file = Helper::openFileForWriting(getOutfile("out"));

//		test_file -> write(test.join("\n"));
//		test_file -> flush();
//		test_file -> close();
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
