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
		addInfile("in", "Ensemble transcript file (download and unzip ftp://ftp.ensembl.org/pub/grch37/update/gff3/homo_sapiens/Homo_sapiens.GRCh37.87.gff3.gz).", false);
		//optional
		addFlag("all", "If set, all transcripts are imported (the default is to skip transcripts not labeled as with the 'GENCODE basic' tag).");
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");

		changeLog(2017,  7,  6, "Added first version");
	}

	QMap<QByteArray, QByteArray> parseAttributes(QByteArray attributes)
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

	int addTranscript(SqlQuery& query, int gene_id, QByteArray name, QByteArray source, const TranscriptData& t_data)
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

		//clear tables if not empty
		if (!db.tableEmpty("gene_transcript") || !db.tableEmpty("gene_exon"))
		{
			if (getFlag("force"))
			{
				db.clearTable("gene_exon");
				db.clearTable("gene_transcript");
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
						BedFile coding_part;
						coding_part.append(BedLine(t_data.exons[0].chr() ,t_data.start_coding, t_data.end_coding));
						t_data.exons.intersect(coding_part);
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

				//transform gene names to approved gene IDs
				int ngsd_gene_id = db.geneToApprovedID(gene);
				if (ngsd_gene_id==-1)
				{
					out << "ERROR " << data["gene_id"] << "/" << gene << ": no approved gene names found." << endl;
					continue;
				}
				gene_ensemble2ngsd[data["ID"]] = ngsd_gene_id;
			}

			//transcript line
			else if (data.contains("transcript_id"))
			{
				if (all || data.value("tag")=="basic")
				{
					TranscriptData tmp;
					tmp.name = data["transcript_id"];
					tmp.name_ccds = data.value("ccdsid", "");
					tmp.chr = parts[0];
					tmp.strand = parts[6];
					tmp.ngsd_gene_id = gene_ensemble2ngsd[data["Parent"]];
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
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
