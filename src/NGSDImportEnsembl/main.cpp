#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "NGSHelper.h"

#include <QFileInfo>

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
		setDescription("Imports Ensembl/CCDS transcript information into NGSD.");
		addInfile("in", "Ensembl transcript file (download and unzip ftp://ftp.ensembl.org/pub/grch37/release-87/gff3/homo_sapiens/Homo_sapiens.GRCh37.87.chr.gff3.gz for GRCh37 and ftp://ftp.ensembl.org/pub/release-104/gff3/homo_sapiens/Homo_sapiens.GRCh38.104.chr.gff3.gz for GRCh38).", false);

		//optional
		addInfileList("pseudogenes", "Pseudogene flat file(s) (download from http://pseudogene.org/psidr/psiDR.v0.txt).", true);
		addFlag("all", "If set, all transcripts are imported (the default is to skip transcripts not labeled as with the 'GENCODE basic' tag).");
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");

		changeLog(2021,  6, 9, "Added support for multiple pseudogene files and duplication check.");
        changeLog(2021,  1, 25, "Made pseudogene file optional");
		changeLog(2021,  1, 20, "Added import of pseudogene relations");
        changeLog(2019,  8, 12, "Added handling of HGNC identifiers to resolve ambiguous gene names");
		changeLog(2017,  7,  6, "Added first version");
	}

    int geneByHGNC(SqlQuery& query, const QByteArray& hgnc_id)
	{
        if(hgnc_id=="") return -1;

		//get NGSD gene ID
		query.bindValue(0, hgnc_id);
		query.exec();

		if (query.size()==0) return -1;

		query.next();
		return query.value(0).toInt();
	}

	int addTranscript(SqlQuery& query, int gene_id, const QByteArray& name, const QByteArray& source, const TranscriptData& t_data)
	{
		//QTextStream(stdout) << "Adding transcript name=" << name << " source=" << source << " gene=" << t_data.ngsd_gene_id << " start_coding=" << t_data.start_coding << " end_coding=" << t_data.end_coding << endl;
		query.bindValue(0, gene_id);
		query.bindValue(1, name);
		query.bindValue(2, source);
		query.bindValue(3, t_data.chr);
		if (t_data.start_coding!=-1 && t_data.end_coding!=-1)
		{
			query.bindValue(4, t_data.start_coding);
			query.bindValue(5, t_data.end_coding);
		}
		else
		{
			query.bindValue(4, QVariant());
			query.bindValue(5, QVariant());
		}
		query.bindValue(6, t_data.strand);
		query.exec();

		return query.lastInsertId().toInt();
	}

	void addExons(SqlQuery& query, int transcript_id, const BedFile& exons)
	{
		//QTextStream(stdout) << " " << exons.count() << endl;
		for (int i=0; i<exons.count(); ++i)
		{
			//out << "  Adding exon start=" << exons[i].start() << " end=" <<exons[i].end() << endl;
			query.bindValue(0, transcript_id);
			query.bindValue(1, exons[i].start());
			query.bindValue(2, exons[i].end());
			query.exec();
		}
	}

	void importPseudogenes(const QMap<QByteArray, QByteArray>& transcript_gene_relation, const QMap<QByteArray, QByteArray>& gene_name_relation, QString pseudogene_file_path)
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);

		// prepare db queries
//		SqlQuery q_select_pseudogene = db.getQuery();
//		q_select_pseudogene.prepare("SELECT id FROM gene_pseudogene_relation WHERE parent_gene_id=:0 AND pseudogene_gene_id=:1");
//		SqlQuery q_select_genename = db.getQuery();

		SqlQuery q_insert_pseudogene = db.getQuery();
		q_insert_pseudogene.prepare("INSERT INTO gene_pseudogene_relation (parent_gene_id, pseudogene_gene_id, gene_name) VALUES (:0, :1, :2);");

		// stats
		int n_missing_pseudogene_transcript_id = 0;
		int n_missing_parent_in_file = 0;
		int n_missing_parent_transcript_id = 0;
		int n_missing_gene_name = 0;
		int n_unknown_transcript = 0;
		int n_found_gene_gene_relations = 0;
		int n_found_gene_gene_relations_by_name_matching = 0;
		int n_found_gene_name_relations = 0;
		int n_duplicate_pseudogenes = 0;

		// extract file name
		QString filename = QFileInfo(pseudogene_file_path).fileName();
		// parse pseudogene file
		QSharedPointer<QFile> pseudogene_fp = Helper::openFileForReading(pseudogene_file_path);


		while(!pseudogene_fp->atEnd())
		{
			QByteArray line = pseudogene_fp->readLine().trimmed();
			if (line.isEmpty() || line.startsWith("#") || line.startsWith("Pseudogene_id") || line.startsWith("ID")) continue;
			QByteArrayList parts = line.split('\t');

			// parse ensembl transcript ids
			QByteArray pseudogene_transcript_ensembl_id = parts.at(0).split('.').at(0).trimmed();
			QByteArray parent_transcript_ensembl_id = parts.at(7).split('.').at(0).trimmed();

			// skip all entiries where no parent is listed or the parent transcript is not in the NGSD
			if (parent_transcript_ensembl_id.isEmpty())
			{
				n_missing_parent_in_file++;
				continue;
			}
			int parent_transcript_id = db.transcriptId(parent_transcript_ensembl_id, false);
			if (parent_transcript_id == -1)
			{
				n_missing_parent_transcript_id++;
				continue;
			}

			// get gene id for parent
			int parent_gene_id = db.getValue("SELECT gene_id FROM gene_transcript WHERE id=" + QByteArray::number(parent_transcript_id), false).toInt();


			// get gene id for pseudogene
			int pseudogene_transcript_id = db.transcriptId(pseudogene_transcript_ensembl_id, false);
			int pseudogene_gene_id = -1;
			if (pseudogene_transcript_id != -1)
			{
				pseudogene_gene_id = db.getValue("SELECT gene_id FROM gene_transcript WHERE id=" + QByteArray::number(pseudogene_transcript_id), false).toInt();

				// check existance
				if(db.getValue("SELECT id FROM gene_pseudogene_relation WHERE parent_gene_id=" + QString::number(parent_gene_id) + " AND pseudogene_gene_id=" + QString::number(pseudogene_gene_id), true) == QVariant())
				{
					// execute SQL query
					q_insert_pseudogene.bindValue(0, parent_gene_id);
					q_insert_pseudogene.bindValue(1, pseudogene_gene_id);
					q_insert_pseudogene.bindValue(2, QVariant());
					q_insert_pseudogene.exec();
					n_found_gene_gene_relations++;
				}
				else
				{
					n_duplicate_pseudogenes++;
				}


			}
			else // fallback1 lookup gene name in ensembl file
			{
				n_missing_pseudogene_transcript_id++;

				if (transcript_gene_relation.contains(pseudogene_transcript_ensembl_id))
				{
					QByteArray ensembl_gene_id = transcript_gene_relation.value(pseudogene_transcript_ensembl_id);
					if (gene_name_relation.contains(ensembl_gene_id))
					{
						QByteArray gene_name = gene_name_relation.value(ensembl_gene_id).split('.').at(0).trimmed();
						pseudogene_gene_id = db.geneToApprovedID(gene_name);

						// try to match by gene name
						if (pseudogene_gene_id != -1)
						{
							// check existance
							if(db.getValue("SELECT id FROM gene_pseudogene_relation WHERE parent_gene_id=" + QString::number(parent_gene_id) + " AND pseudogene_gene_id=" + QString::number(pseudogene_gene_id), true) == QVariant())
							{
								// store gene-pseudogene relation
								q_insert_pseudogene.bindValue(0, parent_gene_id);
								q_insert_pseudogene.bindValue(1, pseudogene_gene_id);
								q_insert_pseudogene.bindValue(2, QVariant());
								q_insert_pseudogene.exec();
								n_found_gene_gene_relations_by_name_matching++;
							}
							else
							{
								n_duplicate_pseudogenes++;
							}
						}
						else // fallback 2: annotate parent with pseudogene name and ensembl id
						{
							// check existance
							if(db.getValue("SELECT id FROM gene_pseudogene_relation WHERE parent_gene_id=" + QString::number(parent_gene_id) + " AND gene_name='" +  ensembl_gene_id + ";" + gene_name + "'", true) == QVariant())
							{
								// execute SQL query
								q_insert_pseudogene.bindValue(0, parent_gene_id);
								q_insert_pseudogene.bindValue(1, QVariant());
								q_insert_pseudogene.bindValue(2, ensembl_gene_id + ";" + gene_name);
								q_insert_pseudogene.exec();
								n_found_gene_name_relations++;
							}
							else
							{
								n_duplicate_pseudogenes++;
							}
						}
					}
					else
					{
						out << "No gene name found for ensembl gene id '" << ensembl_gene_id << "'! \n";
						n_missing_gene_name++;
					}
				}
				else
				{
					out << "Transcript '" << pseudogene_transcript_ensembl_id << "' not found in ensembl flat file! \n";
					n_unknown_transcript++;
				}
			}

		}
		// print stats:
		out << "pseudogene flat file: " << filename << "\n";
		out << "\t missing parent transcript ids in File: " << n_missing_parent_in_file << "\n";
		out << "\t missing pseudogene transcript ids in NGSD: " << n_missing_pseudogene_transcript_id << "\n";
		out << "\t missing parent transcript ids in NGSD: " << n_missing_parent_transcript_id << "\n";
		out << "\n\t found gene-gene relations: " << n_found_gene_gene_relations << "\n";
		out << "\t additional gene-gene relations by name matching: " << n_found_gene_gene_relations_by_name_matching << "\n";
		out << "\t found gene-name relations: " << n_found_gene_name_relations << "\n";
		out << "\t pseudogenes with no gene name: " << n_missing_gene_name << "\n";
		out << "\t pseudogenes with unknown transcript: " << n_unknown_transcript << "\n";
		out << "\t pseudogenes already in database: " << n_duplicate_pseudogenes << "\n";

	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);
		bool all = getFlag("all");

		//check tables exist
		db.tableExists("gene");
		db.tableExists("gene_alias");
		db.tableExists("gene_transcript");
		db.tableExists("gene_exon");
		db.tableExists("gene_pseudogene_relation");

		//clear tables if not empty
		if (!db.tableEmpty("gene_transcript") || !db.tableEmpty("gene_exon") || !db.tableEmpty("gene_pseudogene_relation"))
		{
			if (getFlag("force"))
			{
				db.clearTable("gene_exon");
				db.clearTable("gene_transcript");
				db.clearTable("gene_pseudogene_relation");
			}
			else
			{
				THROW(DatabaseException, "Tables already contain data! Use '-force' to overwrite old data!");
			}
		}

		// prepare queries
		SqlQuery q_trans = db.getQuery();
		q_trans.prepare("INSERT INTO gene_transcript (gene_id, name, source, chromosome, start_coding, end_coding, strand) VALUES (:0, :1, :2, :3, :4, :5, :6);");

		SqlQuery q_exon = db.getQuery();
		q_exon.prepare("INSERT INTO gene_exon (transcript_id, start, end) VALUES (:0, :1, :2);");

		SqlQuery q_gene = db.getQuery();
		q_gene.prepare("SELECT id FROM gene WHERE hgnc_id=:0;");

		// store trancript_id->gene_id and gene_id->gene_name relation for fallback in pseudogene db table
		QMap<QByteArray, QByteArray> transcript_gene_relation;
		QMap<QByteArray, QByteArray> gene_name_relation;

		//parse input - format description at https://www.gencodegenes.org/data_format.html and http://www.ensembl.org/info/website/upload/gff3.html
        QList<TranscriptData> trans_list = NGSHelper::loadGffFile(getInfile("in"), transcript_gene_relation, gene_name_relation, all);

        QSet<QByteArray> ccds_transcripts_added;

        foreach(TranscriptData t_data, trans_list)
        {
            //transform gene name to approved gene ID
            QByteArray gene = gene_name_relation[t_data.gene_symbol];
            int ngsd_gene_id = db.geneToApprovedID(gene);
            if(ngsd_gene_id==-1) //fallback to HGNC ID
            {
                ngsd_gene_id = geneByHGNC(q_gene, t_data.hgnc_id);
                if (ngsd_gene_id!=-1)
                {
                    out << "Notice: Gene " << t_data.gene_id << "/" << gene << " without HGNC-approved name identified by HGNC identifier." << endl;
                }
            }

            if (ngsd_gene_id==-1)
            {
                out << "Notice: Gene " << t_data.gene_id << "/" << gene << " without HGNC-approved name is skipped." << endl;
                continue;
            }

            //add Ensembl transcript
            int trans_id = addTranscript(q_trans, ngsd_gene_id, t_data.name, "ensembl", t_data);
            //add exons
            addExons(q_exon, trans_id, t_data.exons);

            //add CCDS transcript as well (only once)
            if(t_data.name_ccds!="" && !ccds_transcripts_added.contains(t_data.name_ccds))
            {
                int trans_id_ccds = addTranscript(q_trans, ngsd_gene_id, t_data.name_ccds , "ccds", t_data);
                //add exons (only coding part)
                t_data.exons.intersect(BedFile(t_data.exons[0].chr() ,t_data.start_coding, t_data.end_coding));
                addExons(q_exon, trans_id_ccds, t_data.exons);

                ccds_transcripts_added.insert(t_data.name_ccds);
            }
        }

		// parse Pseudogene file
		QStringList pseudogene_file_paths = getInfileList("pseudogenes");
		foreach (const QString& file_path, pseudogene_file_paths)
		{
			importPseudogenes(transcript_gene_relation, gene_name_relation, file_path);
		}

    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
