#include "Exceptions.h"
#include "ToolBase.h"
#include "NGSD.h"

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
		setDescription("Converts a text file with transcript names to a BED file.");
		addInfile("in", "Input TXT file with one transcript name per line. If unset, reads from STDIN.", true, true);
		QStringList modes;
		modes << "gene" << "exon";
		addEnum("mode", "Mode: gene = start/end of the transcript, exon = start/end of all exons of the transcript.", false, modes);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true, true);
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2023,  5, 25, "First version.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QString mode = getEnum("mode");
		QTextStream messages(stderr);

		//process
		BedFile output;
		QSharedPointer<QFile> file = Helper::openFileForReading(getInfile("in"), true);
		QTextStream stream(file.data());
		while(!stream.atEnd())
		{
			QString transcript = stream.readLine().trimmed();

			if(transcript.isEmpty() || transcript.startsWith('#')) continue;

			try
			{
				output.add(db.transcriptToRegions(transcript.toLatin1(), mode));
			}
			catch (Exception& e)
			{
				messages << e.message() << endl;
			}
		}

		//store
		output.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
