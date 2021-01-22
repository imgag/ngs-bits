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
		setDescription("Imports Ensembl/CCDS transcript information into NGSD.");
		addInfile("in", "Ensembl transcript file (download and unzip ftp://ftp.ensembl.org/pub/grch37/release-87/gff3/homo_sapiens/Homo_sapiens.GRCh37.87.chr.gff3.gz).", false);
		addInfile("pseudogenes", "Pseudogene flat file (download from http://pseudogene.org/psidr/psiDR.v0.txt).", false);

		//optional
		addFlag("all", "If set, all transcripts are imported (the default is to skip transcripts not labeled as with the 'GENCODE basic' tag).");
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");

		changeLog(2021,  1, 20, "Added import of pseudogene relations");
		changeLog(2019,  8, 12, "Added handling of HGNC identifiers to resolve ambiguous gene names");
		changeLog(2017,  7,  6, "Added first version");
	}

	int geneByHGNC(SqlQuery& query, const QByteArray& description)
	{
		//extract HGNC identifier
		int start = description.indexOf("[Source:HGNC Symbol%3BAcc:");
		if (start==-1) return -1;
		start += 26;
		int end = description.indexOf("]", start);
		if (end==-1) return  -1;
		QByteArray hgnc_id = description.mid(start, end-start);

		//get NGSD gene ID
		query.bindValue(0, hgnc_id);
		query.exec();

		if (query.size()==0) return -1;

		query.next();
		return query.value(0).toInt();
	}

	QMap<QByteArray, QByteArray> parseAttributes(const QByteArray& attributes)
	{
		QMap<QByteArray, QByteArray> output;

		QByteArrayList parts = attributes.split(';');
		foreach(QByteArray part, parts)
		{
			int split_index = part.indexOf('=');
			output[part.left(split_index)] = part.mid(split_index+1);
		}

		return output;
	}

	struct TranscriptData
	{
		QByteArray name;
		QByteArray name_ccds;
		QByteArray chr;
		int start_coding = -1;
		int end_coding = -1;
		QByteArray strand;

		BedFile exons;

		int ngsd_gene_id;
	};

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
		SqlQuery q_pseudogene = db.getQuery();
		q_pseudogene.prepare("INSERT INTO gene_pseudogene_relation (parent_gene_id, pseudogene_gene_id, gene_name) VALUES (:0, :1, :2);");

		// parse pseudogene file
		QSharedPointer<QFile> pseudogene_fp = Helper::openFileForReading(pseudogene_file_path);

		// stats
		int n_missing_pseudogene_transcript_id = 0;
		int n_missing_parent_in_file = 0;
		int n_missing_parent_transcript_id = 0;
		int n_missing_gene_name = 0;
		int n_unknown_transcript = 0;
		int n_found_gene_gene_relations = 0;
		int n_found_gene_gene_relations_by_name_matching = 0;
		int n_found_gene_name_relations = 0;


		while(!pseudogene_fp->atEnd())
		{
			QByteArray line = pseudogene_fp->readLine().trimmed();
			if (line.isEmpty() || line.startsWith("#") || line.startsWith("Pseudogene_id")) continue;
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

				// execute SQL query
				q_pseudogene.bindValue(0, parent_gene_id);
				q_pseudogene.bindValue(1, pseudogene_gene_id);
				q_pseudogene.bindValue(2, QVariant());
				q_pseudogene.exec();
				n_found_gene_gene_relations++;
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
							// store gene-pseudogene relation
							q_pseudogene.bindValue(0, parent_gene_id);
							q_pseudogene.bindValue(1, pseudogene_gene_id);
							q_pseudogene.bindValue(2, QVariant());
							q_pseudogene.exec();
							n_found_gene_gene_relations_by_name_matching++;
						}
						else // fallback 2: annotate parent with pseudogene name and ensembl id
						{
							// execute SQL query
							q_pseudogene.bindValue(0, parent_gene_id);
							q_pseudogene.bindValue(1, QVariant());
							q_pseudogene.bindValue(2, ensembl_gene_id + ";" + gene_name);
							q_pseudogene.exec();
							n_found_gene_name_relations++;
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
		out << "pseudogene flat file:\n";
		out << "\t missing parent transcript ids in File: " << n_missing_parent_in_file << "\n";
		out << "\t missing pseudogene transcript ids in NGSD: " << n_missing_pseudogene_transcript_id << "\n";
		out << "\t missing parent transcript ids in NGSD: " << n_missing_parent_transcript_id << "\n";
		out << "\n\t found gene-gene relations: " << n_found_gene_gene_relations << "\n";
		out << "\t additional gene-gene relations by name matching: " << n_found_gene_gene_relations_by_name_matching << "\n";
		out << "\t found gene-name relations: " << n_found_gene_name_relations << "\n";
		out << "\t pseudogenes with no gene name: " << n_missing_gene_name << "\n";
		out << "\t pseudogenes with unknown transcript: " << n_unknown_transcript << "\n";

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
		if (!db.tableEmpty("gene_transcript") || !db.tableEmpty("gene_exon"))
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

		//prepare queries
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
		QMap<QByteArray, int> gene_ensemble2ngsd;
		QMap<QByteArray, TranscriptData> transcripts;
		QSet<QByteArray> ccds_transcripts_added;
		auto fp = Helper::openFileForReading(getInfile("in"));
		while(!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty()) continue;

			//section end => commit data
			if (line=="###")
			{
				//import data
				auto it = transcripts.begin();
				while(it!=transcripts.end())
				{
					//add Ensembl transcript
					TranscriptData& t_data = it.value();
					int trans_id = addTranscript(q_trans, t_data.ngsd_gene_id, t_data.name , "ensembl", t_data);
					//add exons
					t_data.exons.merge();
					addExons(q_exon, trans_id, t_data.exons);

					//add CCDS transcript as well (only once)
					if (t_data.name_ccds!="" && !ccds_transcripts_added.contains(t_data.name_ccds))
					{
						int trans_id_ccds = addTranscript(q_trans, t_data.ngsd_gene_id, t_data.name_ccds , "ccds", t_data);
						//add exons (only coding part)
						t_data.exons.intersect(BedFile(t_data.exons[0].chr() ,t_data.start_coding, t_data.end_coding));
						addExons(q_exon, trans_id_ccds, t_data.exons);

						ccds_transcripts_added.insert(t_data.name_ccds);
					}
					++it;
				}

				//clear cache
				gene_ensemble2ngsd.clear();
				transcripts.clear();
				continue;
			}

			//skip header lines
			if (line.startsWith("#")) continue;

			QByteArrayList parts = line.split('\t');
			QByteArray type = parts[2];
			QMap<QByteArray, QByteArray> data = parseAttributes(parts[8]);

			//gene line
			if (data.contains("gene_id"))
			{
				QByteArray gene = data["Name"];

				// store mapping for pseudogene table
				gene_name_relation.insert(data["gene_id"], data["Name"]);

				if (!Chromosome(parts[0]).isNonSpecial())
				{
					out << "Notice: Gene " << data["gene_id"] << "/" << gene << " on special chromosome " << parts[0] << " is skipped." << endl;
					continue;
				}

				//transform gene names to approved gene IDs
				int ngsd_gene_id = db.geneToApprovedID(gene);
				if (ngsd_gene_id==-1) //fallback to HGNC ID
				{
					ngsd_gene_id = geneByHGNC(q_gene, data["description"]);
					if (ngsd_gene_id!=-1)
					{
						out << "Notice: Gene " << data["gene_id"] << "/" << gene << " without HGNC-approved name identified by HGNC identifier." << endl;
					}
				}

				if (ngsd_gene_id==-1)
				{
					out << "Notice: Gene " << data["gene_id"] << "/" << gene << " without HGNC-approved name is skipped." << endl;
					continue;
				}

				gene_ensemble2ngsd[data["ID"]] = ngsd_gene_id;
			}

			//transcript line
			else if (data.contains("transcript_id"))
			{
				// store mapping for pseudogene table
				transcript_gene_relation.insert(data["transcript_id"], data["Parent"].split(':').at(1));

				if (all || data.value("tag")=="basic")
				{
					QByteArray parent_id = data["Parent"];

					//skip transcripts of unhandled genes (e.g. no HGNC gene name)
					if (!gene_ensemble2ngsd.contains(parent_id)) continue;

					TranscriptData tmp;
					tmp.name = data["transcript_id"];
					tmp.name_ccds = data.value("ccdsid", "");
					tmp.chr = parts[0];
					tmp.strand = parts[6];
					tmp.ngsd_gene_id = gene_ensemble2ngsd[parent_id];
					transcripts[data["ID"]] = tmp;
				}
			}

			//exon lines
			else if (type=="CDS" || type=="exon" || type=="three_prime_UTR" || type=="five_prime_UTR" )
			{
				QByteArray parent_id = data["Parent"];

				//skip exons of unhandled transcripts (not GENCODE basic)
				if (!transcripts.contains(parent_id)) continue;
				TranscriptData& t_data = transcripts[parent_id];

				//check chromosome matches
				QByteArray chr = parts[0];
				if (chr!=t_data.chr)
				{
					THROW(FileParseException, "Chromosome mismach between transcript and exon!");
				}

				//update coding start/end
				int start = Helper::toInt(parts[3], "start position");
				int end = Helper::toInt(parts[4], "end position");

				if (type=="CDS")
				{
					t_data.start_coding = (t_data.start_coding==-1) ? start : std::min(start, t_data.start_coding);
					t_data.end_coding = (t_data.end_coding==-1) ? end : std::max(end, t_data.end_coding);
				}

				//add coding exon
				t_data.exons.append(BedLine(chr, start, end));
			}
		}


		// parse Pseudogene file
		importPseudogenes(transcript_gene_relation, gene_name_relation, getInfile("pseudogenes"));

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
