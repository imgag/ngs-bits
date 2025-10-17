#include "BedFile.h"
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
		setDescription("Annotates BED file regions with gene names.");
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addInt("extend", "The number of bases to extend the gene regions before annotation.", true, 0);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("clear", "Clear all annotations present in the input file.");

		changeLog(2017, 11, 28, "Added 'clear' flag.");
		changeLog(2017, 11, 03, "Now appends a column to the BED file instead of always writing it into the 4th column.");
	}

	virtual void main()
	{
		//init
		int extend = getInt("extend");
		NGSD db(getFlag("test"));

		//process
		BedFile file;
		file.load(getInfile("in"));
		if (getFlag("clear"))
		{
			file.clearAnnotations();
		}

		for(int i=0; i<file.count(); ++i)
		{
			BedLine& line = file[i];

			GeneSet genes = db.genesOverlapping(line.chr(), line.start(), line.end(), extend);
			line.annotations() << genes.join(", ");
		}

		//store
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
