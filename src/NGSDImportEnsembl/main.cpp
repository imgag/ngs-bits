#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "NGSHelper.h"
#include "Transcript.h"

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
		addInfile("in", "Ensembl transcript file (download and unzip https://ftp.ensembl.org/pub/grch37/release-87/gff3/homo_sapiens/Homo_sapiens.GRCh37.87.gff3.gz for GRCh37 and https://ftp.ensembl.org/pub/release-107/gff3/homo_sapiens/Homo_sapiens.GRCh38.107.gff3.gz for GRCh38).", false);
		addInfile("ensembl_canonical", "Ensembl canonical transcript TSV file (download and unzip from https://ftp.ensembl.org/pub/release-105/tsv/homo_sapiens/Homo_sapiens.GRCh38.105.canonical.tsv.gz", false);
		addInfile("mane", "GFF file with MANE information (download and unzip from https://ftp.ncbi.nlm.nih.gov/refseq/MANE/MANE_human/release_1.0/MANE.GRCh38.v1.0.ensembl_genomic.gff.gz", false);
		//optional
		addInfileList("pseudogenes", "Pseudogene flat file(s) (download from http://pseudogene.org/psidr/psiDR.v0.txt and http://pseudogene.org/psicube/data/gencode.v10.pgene.parents.txt).", true);
		addFlag("all", "If set, all transcripts are imported (the default is to skip transcripts that do not have at least one of the flags 'GENCODE basic', 'Ensembl canonical', 'MANE select' or 'MANE plus clinical').");
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");

		changeLog(2022, 10, 17, "Added transcript versions.");
		changeLog(2022,  5, 29, "Added parameters 'ensembl_canonical' and 'mane'.");
		changeLog(2021,  6,  9, "Added support for multiple pseudogene files and duplication check.");
        changeLog(2021,  1, 25, "Made pseudogene file optional");
		changeLog(2021,  1, 20, "Added import of pseudogene relations");
        changeLog(2019,  8, 12, "Added handling of HGNC identifiers to resolve ambiguous gene names");
		changeLog(2017,  7,  6, "Added first version");
	}

	void loadEnsemblCanonical(QString filename, QSet<QByteArray>& ensembl_canonical)
	{
		QSharedPointer<QFile> file = Helper::openFileForReading(filename);
		while(!file->atEnd())
		{
			QByteArray line = file->readLine().trimmed();
			if (line.isEmpty() || line.startsWith("gene_stable_id")) continue;

			QByteArrayList parts = line.split('\t');
			if (parts.count()<3) continue;

			QByteArray transcript_id = parts[1];
			if (transcript_id.contains('.'))
			{
				transcript_id = transcript_id.split('.')[0];
			}

			ensembl_canonical << transcript_id;
		}
	}

	void loadMane(QString filename, QSet<QByteArray>& mane_select, QSet<QByteArray>& mane_plus_clinical)
	{
		QSharedPointer<QFile> file = Helper::openFileForReading(filename);
		while(!file->atEnd())
		{
			QByteArrayList parts = file->readLine().split('\t');
			if (parts.count()<9) continue;

			if (!parts[8].startsWith("ID=ENST")) continue;

			QByteArray transcript_id;
			foreach(const QByteArray& part, parts[8].split(';'))
			{
				//parse transcript id
				if (part.startsWith("ID="))
				{
					transcript_id = part.mid(3);
					if (transcript_id.contains('.'))
					{
						transcript_id = transcript_id.split('.')[0];
					}
				}

				//parse tags
				if (part.startsWith("tag="))
				{
					if (part.contains("MANE_Select")) mane_select << transcript_id;
					if (part.contains("MANE_Plus_Clinical")) mane_plus_clinical << transcript_id;
				}
			}
		}
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

	int addTranscript(SqlQuery& query, int gene_id, const QByteArray& name, int version, const QByteArray& source, const Transcript& t, bool is_gencode_basic, bool is_ensembl_canonical, bool is_mane_select, bool is_mane_plus_clinical)
	{
		//QTextStream(stdout) << "Adding transcript name=" << name << " version=" << version << " source=" << source << endl;
		query.bindValue(0, gene_id);
		query.bindValue(1, name);
		query.bindValue(2, version);
		query.bindValue(3, source);
		query.bindValue(4, t.chr().str());
        if (t.codingStart()!=0 && t.codingEnd()!=0)
        {
            //Transcript class encodes actual coding start (ATG), but coding_start < coding_end is required here
            int coding_start = std::min(t.codingStart(), t.codingEnd());
            int coding_end = std::max(t.codingStart(), t.codingEnd());
			query.bindValue(5, coding_start);
			query.bindValue(6, coding_end);
		}
		else
		{
			query.bindValue(5, QVariant());
			query.bindValue(6, QVariant());
		}
        QByteArray strand = t.strand() == Transcript::PLUS ? "+" : "-";
		query.bindValue(7, strand);
		query.bindValue(8, Transcript::biotypeToString(t.biotype()));
		query.bindValue(9, is_gencode_basic);
		query.bindValue(10, is_ensembl_canonical);
		query.bindValue(11, is_mane_select);
		query.bindValue(12, is_mane_plus_clinical);
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

	void importPseudogenes(const QHash<QByteArray, QByteArray>& enst2ensg, const QHash<QByteArray, QByteArray>& ensg2symbol, QString pseudogene_file_path)
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);

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

				if (enst2ensg.contains(pseudogene_transcript_ensembl_id))
				{
					QByteArray ensembl_gene_id = enst2ensg.value(pseudogene_transcript_ensembl_id);
					if (ensg2symbol.contains(ensembl_gene_id))
					{
						QByteArray gene_symbol = ensg2symbol.value(ensembl_gene_id).split('.').at(0).trimmed();
						pseudogene_gene_id = db.geneId(gene_symbol);

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
							if(db.getValue("SELECT id FROM gene_pseudogene_relation WHERE parent_gene_id=" + QString::number(parent_gene_id) + " AND gene_name='" +  ensembl_gene_id + ";" + gene_symbol + "'", true) == QVariant())
							{
								// execute SQL query
								q_insert_pseudogene.bindValue(0, parent_gene_id);
								q_insert_pseudogene.bindValue(1, QVariant());
								q_insert_pseudogene.bindValue(2, ensembl_gene_id + ";" + gene_symbol);
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
					out << "Pseudogene transcript '" << pseudogene_transcript_ensembl_id << "' not found in ensembl flat file! \n";
					n_unknown_transcript++;
				}
			}
		}

		// print stats
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
		q_trans.prepare("INSERT INTO gene_transcript (gene_id, name, version, source, chromosome, start_coding, end_coding, strand, biotype, is_gencode_basic, is_ensembl_canonical, is_mane_select, is_mane_plus_clinical) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12)");

		SqlQuery q_exon = db.getQuery();
		q_exon.prepare("INSERT INTO gene_exon (transcript_id, start, end) VALUES (:0, :1, :2);");

		SqlQuery q_gene = db.getQuery();
		q_gene.prepare("SELECT id FROM gene WHERE hgnc_id=:0;");

		//load tag data
		QSet<QByteArray> ensembl_canonical;
		loadEnsemblCanonical(getInfile("ensembl_canonical"), ensembl_canonical);
		QSet<QByteArray> mane_select;
		QSet<QByteArray> mane_plus_clinical;
		loadMane(getInfile("mane"), mane_select, mane_plus_clinical);

		//parse input - format description at https://www.gencodegenes.org/data_format.html and http://www.ensembl.org/info/website/upload/gff3.html
		GffSettings gff_settings;
		gff_settings.print_to_stdout = true;
		gff_settings.skip_not_gencode_basic = false;
		GffData data = NGSHelper::loadGffFile(getInfile("in"), gff_settings);
        QSet<QByteArray> ccds_transcripts_added;
		foreach(const Transcript& t, data.transcripts)
        {
			QByteArray transcript_id = t.name();
			bool is_gencode_basic = data.gencode_basic.contains(transcript_id);
			bool is_ensembl_canonical = ensembl_canonical.contains(transcript_id);
			bool is_mane_select = mane_select.contains(transcript_id);
			bool is_mane_plus_clinical = mane_plus_clinical.contains(transcript_id);

			//if not 'all' or has important flag > skip it
			if (!all && !is_gencode_basic && !is_ensembl_canonical && !is_mane_select && !is_mane_plus_clinical) continue;

            //transform gene name to approved gene ID
			QByteArray gene = t.gene();
			int ngsd_gene_id = db.geneId(gene);
            if(ngsd_gene_id==-1) //fallback to HGNC ID
            {
                ngsd_gene_id = geneByHGNC(q_gene, t.hgncId());
                if (ngsd_gene_id!=-1)
                {
                    out << "Notice: Gene " << t.geneId() << "/" << gene << " without HGNC-approved name identified by HGNC identifier." << endl;
                }
            }

            if (ngsd_gene_id==-1)
            {
                out << "Notice: Gene " << t.geneId() << "/" << gene << " without HGNC-approved name is skipped." << endl;
                continue;
            }

			//add Ensembl transcript
			int trans_id = addTranscript(q_trans, ngsd_gene_id, transcript_id, t.version(), "ensembl", t, is_gencode_basic, is_ensembl_canonical, is_mane_select, is_mane_plus_clinical);
            //add exons
            addExons(q_exon, trans_id, t.regions());

            //add CCDS transcript as well (only once)
            if(t.nameCcds()!="" && !ccds_transcripts_added.contains(t.nameCcds()))
            {
				QByteArrayList parts = t.nameCcds().split('.');
				if (parts.count()!=2) THROW(FileParseException, "CCDS transcript name does not contain two parts separated by '.': " + t.nameCcds());
				QByteArray name = parts[0];
				int version = Helper::toInt(parts[1], "CCDS transcript version");

				int trans_id_ccds = addTranscript(q_trans, ngsd_gene_id, name, version, "ccds", t, false, false, false, false);
                //add exons (only coding part)
				BedFile exons = t.regions();
                //Transcript class encodes actual coding start (ATG), but coding_start < coding_end is required here
                int coding_start = std::min(t.codingStart(), t.codingEnd());
                int coding_end = std::max(t.codingStart(), t.codingEnd());
                exons.intersect(BedFile(exons[0].chr(), coding_start, coding_end));
                addExons(q_exon, trans_id_ccds, exons);

                ccds_transcripts_added.insert(t.nameCcds());
            }
        }

		// parse Pseudogene file
		QStringList pseudogene_file_paths = getInfileList("pseudogenes");
		foreach (const QString& file_path, pseudogene_file_paths)
		{
			importPseudogenes(data.enst2ensg, data.ensg2symbol, file_path);
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
