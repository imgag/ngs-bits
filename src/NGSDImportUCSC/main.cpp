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
		setDescription("Imports transcript information into NGSD (download from http://hgdownload.cse.ucsc.edu/goldenPath/hg19/database/).");
		addInfile("ccds", "UCSC ccdsGene.txt file", false);
		addInfile("ccdsKM", "UCSC ccdsKgMap.txt file", false);
		addInfile("kg", "UCSC knownGene.txt file", false);
		addInfile("kgXR", "UCSC kgXref.txt file", false);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");
	}

	void removeEmptyElements(QList<QByteArray>& list)
	{
		//trim all elements
		for (int i=0; i<list.count(); ++i)
		{
			list[i] = list[i].trimmed();
		}

		//remove empty elements
		auto new_end = std::remove_if(list.begin(), list.end(), [](const QByteArray& string) { return string.isEmpty();} );
		list.erase(new_end, list.end());
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);

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

		//prepare SQL queries
		SqlQuery qi_exon = db.getQuery();
		qi_exon.prepare("INSERT INTO gene_exon (transcript_id, start, end) VALUES (:0, :1, :2);");
		SqlQuery qi_trans = db.getQuery();
		qi_trans.prepare("INSERT INTO gene_transcript (gene_id, name, source, start_coding, end_coding, strand) VALUES (:0, :1, :2, :3, :4, :5);");

		//parse kgXref.txt
		QHash<QByteArray, QByteArray> ucsc2gene;
		QSharedPointer<QFile> fp = Helper::openFileForReading(getInfile("kgXR"));
		while(!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty()) continue;

			QList<QByteArray> parts = line.split('\t');
			if (parts.count()<5) THROW(FileParseException, fp->fileName() + " contains invalid line: " + line);

			QByteArray ucsc_id = parts[0].trimmed();
			if(ucsc2gene.contains(ucsc_id)) THROW(FileParseException, fp->fileName() + " contains duplicate UCSC id: " + ucsc_id);
			ucsc2gene.insert(ucsc_id, parts[4].trimmed());
		}
		out << "Extracted " << ucsc2gene.count() << " UCSC IDs from " << fp->fileName() << endl;

		//parse ccdsKgMap.txt
		QMultiHash<QByteArray, QByteArray> ccds2ucsc;
		fp = Helper::openFileForReading(getInfile("ccdsKM"));
		while(!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty()) continue;

			QList<QByteArray> parts = line.split('\t');
			if (parts.count()<2) THROW(FileParseException, fp->fileName() + " contains invalid line: " + line);

			ccds2ucsc.insert(parts[0].trimmed(), parts[1].trimmed());
		}
		out << "Extracted " << ccds2ucsc.count() << " CCDS IDs from " << fp->fileName() << endl;

		//parse knownGene.txt
		int imported = 0;
		fp = Helper::openFileForReading(getInfile("kg"));
		while(!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty()) continue;

			QList<QByteArray> parts = line.split('\t');
			if (parts.count()<10) THROW(FileParseException, fp->fileName() + " contains invalid line: " + line);

			//get gene name(s)
			QByteArray ucsc_id = parts[0].trimmed();
			QByteArray gene = ucsc2gene.value(ucsc_id);
			if (gene.isEmpty()) THROW(Exception, "ucsc2gene empty for " + ucsc_id);

			//transform gene names to approved gene IDs
			QByteArray chr = parts[1].replace("chr", "");
			int gene_id = db.geneToApprovedID(gene);
			if (gene_id==-1)
			{
				out << ucsc_id << ": no approved gene names found." << endl;
				continue;
			}

			//check that chromosomes of HGNC/UCSC match
			QByteArray hgnc_chr = db.getValue("SELECT chromosome FROM gene WHERE id='" + QString::number(gene_id) + "'").toByteArray();
			if (hgnc_chr!=chr)
			{
				out << ucsc_id << ": chromosome mismatch (ucsc: " + gene + "/chr" + chr + ", hgnc:" + db.geneSymbol(gene_id) + "/chr" + hgnc_chr + ")" << endl;
				continue; //TODO: handle PAR region on chrX/chrY?!
			}

			//get rest of transcript information
			QByteArray strand = parts[2];
			int start_coding = Helper::toInt(parts[5], "coding start") + 1;
			int end_coding = Helper::toInt(parts[6], "coding end");
			QList<QByteArray> exon_starts = parts[8].split(',');
			removeEmptyElements(exon_starts);
			QList<QByteArray> exon_ends = parts[9].split(',');
			removeEmptyElements(exon_ends);
			if (exon_starts.count()!=exon_ends.count()) THROW(FileParseException, fp->fileName() + " contains exon start/end count mismatch: " + line);

			//insert transcript (gene_id, name, sources, start_coding, end_coding, strand)
			++imported;
			qi_trans.bindValue(0, gene_id);
			qi_trans.bindValue(1, ucsc_id);
			qi_trans.bindValue(2, "ucsc");
			if (start_coding<=end_coding)
			{
				qi_trans.bindValue(3, start_coding);
				qi_trans.bindValue(4, end_coding);
			}
			else
			{
				qi_trans.bindValue(3, QVariant());
				qi_trans.bindValue(4, QVariant());
			}
			qi_trans.bindValue(5, strand);
			qi_trans.exec();
			QVariant trans_id = qi_trans.lastInsertId().toInt();

			//insert exons (transcript_id, start, end)
			for(int i=0; i<exon_starts.count(); ++i)
			{
				qi_exon.bindValue(0, trans_id);
				qi_exon.bindValue(1, Helper::toInt(exon_starts[i], "exon start") + 1);
				qi_exon.bindValue(2, Helper::toInt(exon_ends[i], "exon end"));
				qi_exon.exec();
			}
		}
		out << "Imported " << QString::number(imported) + " UCSC transcripts!" << endl;

		//parse ccdsGene.txt
		imported = 0;
		fp = Helper::openFileForReading(getInfile("ccds"));
		while(!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty()) continue;

			QList<QByteArray> parts = line.split('\t');
			if (parts.count()<11) THROW(FileParseException, fp->fileName() + " contains invalid line: " + line);

			//get gene name(s)
			QSet<QByteArray> genes;
			QByteArray ccds_id = parts[1].trimmed();
			QList<QByteArray> ucsc_ids = ccds2ucsc.values(ccds_id);
			if (!ucsc_ids.isEmpty())
			{
				foreach(const QByteArray& ucsc_id, ucsc_ids)
				{
					QByteArray gene = ucsc2gene.value(ucsc_id);
					if (gene.isEmpty()) THROW(Exception, "ucsc2gene empty for " + ucsc_id);
					genes.insert(gene);
				}
			}

			//transform gene names to approved gene IDs
			QByteArray chr = parts[2].replace("chr", "");
			QSet<int> gene_ids;
			foreach(const QByteArray& gene, genes)
			{
				int gene_id = db.geneToApprovedID(gene);
				if (gene_id!=-1)
				{
					//check that chromosomes of HGNC/CCDS match
					QByteArray hgnc_chr = db.getValue("SELECT chromosome FROM gene WHERE id='" + QString::number(gene_id) + "'").toByteArray();
					if (hgnc_chr!=chr)
					{
						QString hgnc_gene = db.getValue("SELECT symbol FROM gene WHERE id='" + QString::number(gene_id) + "'").toString();
						out << ccds_id << ": chromosome mismatch (ccds: " + gene + "/chr" + chr + ", hgnc:" + hgnc_gene + "/chr" + hgnc_chr + ")" << endl;
						continue; //TODO: handle PAR region on chrX/chrY?!
					}

					gene_ids.insert(gene_id);
				}
			}

			if (gene_ids.isEmpty()) //no ccds entry for ORFs etc.
			{
				out << ccds_id << ": no approved gene names found." << endl;
				continue;
			}

			//get rest of transcript information
			QByteArray strand = parts[3];
			int start_coding = Helper::toInt(parts[4], "coding start") + 1;
			int end_coding = Helper::toInt(parts[5], "coding end");
			QList<QByteArray> exon_starts = parts[9].split(',');
			removeEmptyElements(exon_starts);
			QList<QByteArray> exon_ends = parts[10].split(',');
			removeEmptyElements(exon_ends);
			if (exon_starts.count()!=exon_ends.count()) THROW(FileParseException, fp->fileName() + " contains exon start/end count mismatch: " + line);

			//add transcript/exons for gene
			foreach(int gene_id, gene_ids)
			{
				//insert transcript (gene_id, name, sources, start_coding, end_coding, strand)
				++imported;
				qi_trans.bindValue(0, gene_id);
				qi_trans.bindValue(1, ccds_id);
				qi_trans.bindValue(2, "ccds");
				if (start_coding<=end_coding)
				{
					qi_trans.bindValue(3, start_coding);
					qi_trans.bindValue(4, end_coding);
				}
				else
				{
					qi_trans.bindValue(3, QVariant());
					qi_trans.bindValue(4, QVariant());
				}
				qi_trans.bindValue(5, strand);
				qi_trans.exec();
				QVariant trans_id = qi_trans.lastInsertId().toInt();

				//insert exons (transcript_id, start, end)
				for(int i=0; i<exon_starts.count(); ++i)
				{
					qi_exon.bindValue(0, trans_id);
					qi_exon.bindValue(1, Helper::toInt(exon_starts[i], "exon start") + 1);
					qi_exon.bindValue(2, Helper::toInt(exon_ends[i], "exon end"));
					qi_exon.exec();
				}
			}
		}
		out << "Imported " << QString::number(imported) + " CCDS transcripts!" << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
