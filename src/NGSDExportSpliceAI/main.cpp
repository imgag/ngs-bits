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
		addOutfile("out", "The output TXT file.", false);
        addFlag("test", "Uses the test database instead of on the production database.");
	}

    virtual void main()
    {
		//init
		NGSD db(getFlag("test"));
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		outfile->write("#NAME\tCHROM\tSTRAND\tTX_START\tTX_END\tEXON_START\tEXON_END\n");

		//extract clinically relevant transcripts
		TranscriptList transcripts;
		foreach(const Transcript& t, db.transcripts())
		{
			if (t.isGencodePrimaryTranscript() || t.isManeSelectTranscript() || t.isManePlusClinicalTranscript())
			{
				transcripts << t;
			}
		}

		//sort transcripts
		transcripts.sortByPosition();

		//output
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

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
