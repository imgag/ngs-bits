#include "BedFile.h"
#include "ToolBase.h"
#include "Exceptions.h"
#include "ChainFileReader.h"
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
		setDescription("Lifts the regions in the bed file according to the provided chain file.");
		addInfile("in", "Input BED file with the regions to lift.", false);
		addOutfile("lifted", "The file where the lifted regions will be written to.", false);
		addOutfile("unmapped", "The file where the unmappable regions will be written to.", false);

		//optional
		addString("chain", "Input Chain file or \"hg19_hg38\" / \"hg38_hg19\" to read from settings file.", true, "hg19_hg38");
		addFloat("del", "Allowed percentage of deleted/unmapped bases in each region.", true, 0.05);

		changeLog(2022,  02, 14, "First implementation");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QSharedPointer<QFile> bed = Helper::openFileForReading(in);
		QString chain = getString("chain");

		if (! QFile(chain).exists() && ! chain.contains('\\') && ! chain.contains('/'))
		{
			chain = Settings::string("liftover_" + chain, false);
		}

		double allowed_del = getFloat("del");

		if (allowed_del < 0 || 1 < allowed_del)
		{
			THROW(ArgumentException, "Allowed percentage of deleted/unmapped bases can't be smaller than 0 or larger than 1.")
		}

		ChainFileReader reader(chain, allowed_del);

		//open output
		QString lifted_path = getOutfile("lifted");
		QSharedPointer<QFile> lifted = Helper::openFileForWriting(lifted_path, true);

		QString unmapped_path = getOutfile("unmapped");
		QSharedPointer<QFile> unmapped = Helper::openFileForWriting(unmapped_path, true);


		while(! bed->atEnd())
		{
			BedLine l = BedLine::fromString(bed->readLine());

			try
			{
				// convert to 1-based coordinates for lifting
				BedLine lifted_line = reader.lift(l.chr(), l.start()+1, l.end());
				//convert back to 0-based for bed file.
				lifted_line.setStart(lifted_line.start()-1);

				lifted->write(lifted_line.toString(false).toLatin1() + "\n");
			}
			catch (ArgumentException& e)
			{
				unmapped->write("# " + e.message().toLatin1() + "\n");
				unmapped->write(l.toString(false).toLatin1() + "\n");
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

