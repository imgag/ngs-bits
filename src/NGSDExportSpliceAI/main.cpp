#include "ToolBase.h"
#include "NGSD.h"
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
		setDescription("Exports gene transcripts from NGSD in format expected by SpliceAI (parameter -A).");
		setExtendedDescription(QStringList() << "The export is based on the following transcripts: MANE select transcripts, MANE plus clinical transcripts and perferred transcrripts." << "Then the best transcript of all protein-coding and OMIM genes are that are not covered by the initial export are added.");
		addOutfile("out", "The output TXT file.", false);
        addFlag("test", "Uses the test database instead of on the production database.");
	}

    virtual void main()
    {
		//init
		NGSD db(getFlag("test"));

		//extract clinically relevant transcripts
		GeneSet genes_done;
		TranscriptList transcripts;
		foreach(const Transcript& t, db.transcripts())
		{
			if (t.isPreferredTranscript() || t.isManeSelectTranscript() || t.isManePlusClinicalTranscript())
			{
				transcripts << t;
				genes_done << t.gene();
			}
		}

		//add best transcript of protein-coding genes
		SqlQuery query = db.getQuery();
		query.exec("SELECT symbol from gene WHERE type='protein-coding gene'");
		while (query.next())
		{
			QByteArray gene = query.value(0).toByteArray();
			if (genes_done.contains(gene)) continue;

			Transcript t = db.bestTranscript(db.geneId(gene));
			if (!t.isValid()) continue;

			transcripts << t;
			genes_done << t.gene();
		}

		//add best transcript of OMIM genes
		query.exec("SELECT DISTINCT g.gene FROM omim_gene g, omim_phenotype p WHERE g.id=p.omim_gene_id");
		while (query.next())
		{
			QByteArray gene = query.value(0).toByteArray();
			if (genes_done.contains(gene)) continue;

			Transcript t = db.bestTranscript(db.geneId(gene));
			if (!t.isValid()) continue;

			transcripts << t;
			genes_done << t.gene();
		}

		//sort transcripts
		transcripts.sortByPosition();

		//output
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		outfile->write("#NAME\tCHROM\tSTRAND\tTX_START\tTX_END\tEXON_START\tEXON_END\n");
		foreach(const Transcript& t, transcripts)
		{
			QByteArray exon_starts;
			QByteArray exon_ends;
			for(int i=0; i<t.regions().count(); ++i)
			{
				const BedLine& exon = t.regions()[i];
				exon_starts += QByteArray::number(exon.start()-1)+",";
				exon_ends += QByteArray::number(exon.end())+",";
			}
			outfile->write(t.gene() + "_" + t.name() +"\t" + t.chr().strNormalized(true) + "\t" + Transcript::strandToString(t.strand()) + "\t" + QByteArray::number(t.start()-1) + "\t" + QByteArray::number(t.end()) + "\t" + exon_starts + "\t" + exon_ends + "\n");
		}

		//debug output
		QTextStream outstream(stdout);
		outstream << "Exported genes: " << genes_done.count() << Qt::endl;
		outstream << "Exported transcripts: " << transcripts.count() << Qt::endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
