#include "Exceptions.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Log.h"
#include "BedFile.h"
#include "Settings.h"
#include "NGSD.h"
#include <QSet>
#include <QFile>
#include <QTextStream>

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
		setDescription("Converts a text file with gene names to a BED file.");
		addInfile("in", "Input TXT file with one gene symbol per line. If unset, reads from STDIN.", true, true);
		QStringList sources;
		sources << "ccds" << "ucsc";
		addEnum("source", "Source database.", false, sources);
		QStringList modes;
		modes << "gene" << "exon";
		addEnum("mode", "Mode: gene = start/end of gene, exon = start/end of all exons of all splice variants.", false, modes);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true, true);
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		QString source = getEnum("source");
		QString mode = getEnum("mode");
		NGSD db(getFlag("test"));

		BedFile output;
		QTextStream messages(stderr);

		//prepare queries
		SqlQuery q_transcript = db.getQuery();
		q_transcript.prepare("SELECT id, start_coding, end_coding FROM gene_transcript WHERE source='" + source + "' AND gene_id=:1 AND start_coding IS NOT NULL");
		SqlQuery q_exon = db.getQuery();
		q_exon.prepare("SELECT start, end FROM gene_exon WHERE transcript_id=:1");

		//process input data
		QSharedPointer<QFile> infile = Helper::openFileForReading(getInfile("in"), true);
		QStringList genes = Helper::loadTextFile(infile, true, '#', true);
		foreach(QString gene, genes)
		{
			//get approved gene id
			gene = gene.toUpper();
			int id = db.geneToApprovedID(gene.toUtf8());
			if (id==-1)
			{
				messages << "Gene name '" << gene << "' is no HGNC-approved symbol. Skipping it!" << endl;
				continue;
			}

			//get chromosome
			QString chr = "chr" + db.getValue("SELECT chromosome FROM gene WHERE id='" + QString::number(id) + "'").toString();

			//preprare annotations
			QStringList annos;
			annos << gene;

			if (mode=="gene")
			{
				int start_coding = std::numeric_limits<int>::max();
				int end_coding = -std::numeric_limits<int>::max();

				q_transcript.bindValue(0, id);
				q_transcript.exec();
				while(q_transcript.next())
				{
					start_coding = std::min(start_coding, q_transcript.value(1).toInt());
					end_coding = std::max(end_coding, q_transcript.value(2).toInt());
				}

				if (start_coding>end_coding)
				{
					messages << "No coding transcripts found for gene name '" << gene << "'. Skipping it!" << endl;
				}
				else
				{
					output.append(BedLine(chr, start_coding, end_coding, annos));
				}
			}
			else //mode=exon
			{
				int line_count = 0;
				q_transcript.bindValue(0, id);
				q_transcript.exec();
				while(q_transcript.next())
				{
					int trans_id = q_transcript.value(0).toInt();
					int start_coding = q_transcript.value(1).toInt();
					int end_coding = q_transcript.value(2).toInt();
					q_exon.bindValue(0, trans_id);
					q_exon.exec();
					while(q_exon.next())
					{
						int start = std::max(start_coding, q_exon.value(0).toInt());
						int end = std::min(end_coding, q_exon.value(1).toInt());
						if (end<start_coding || start>end_coding) continue;

						output.append(BedLine(chr, start, end, annos));
						++line_count;
					}
				}
				if (line_count==0)
				{
					messages << "No coding exons found for gene name '" << gene << "'. Skipping it!" << endl;
					continue;
				}
			}
		}

		output.sort(true);
		output.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
