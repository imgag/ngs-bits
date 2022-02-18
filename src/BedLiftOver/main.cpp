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
		addOutfile("out", "The file where the lifted regions will be written to.", false);

		//optional
		addOutfile("unmapped", "The file where the unmappable regions will be written to.", true, false);
		addString("chain", "Input Chain file or \"hg19_hg38\" / \"hg38_hg19\" to read from settings file.", true, "hg19_hg38");
		addInt("max_deletion", "Allowed percentage of deleted/unmapped bases in each region.", true, 5);
		addInt("max_increase", "Allowed percentage size increase of a region.", true, 10);
		addFlag("remove_special_chr", "Removes regions that are mapped to special chromosomes.");

		changeLog(2022,  02, 14, "First implementation");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QSharedPointer<QFile> bed = Helper::openFileForReading(in);
		QString chain = getString("chain");
		int max_inc = getInt("max_increase");
		int max_del = getInt("max_deletion");
		bool remove_special_chr = getFlag("remove_special_chr");


		if (! QFile(chain).exists() && ! chain.contains('\\') && ! chain.contains('/'))
		{
			chain = Settings::string("liftover_" + chain, false);
		}

		if (max_del < 0 || 100 < max_del)
		{
			THROW(ArgumentException, "Allowed percentage of deleted/unmapped bases can't be smaller than 0 or larger than 100.");
		}

		if (max_inc < 0)
		{
			THROW(ArgumentException, "Allowed maximum size increase of the region can't be negative");
		}

		ChainFileReader reader(chain, (max_del/100.0));

		//open output
		QString lifted_path = getOutfile("out");
		QSharedPointer<QFile> lifted = Helper::openFileForWriting(lifted_path, true);

		QString unmapped_path = getOutfile("unmapped");
		QSharedPointer<QFile> unmapped;
		if (unmapped_path != "")
		{
			unmapped = Helper::openFileForWriting(unmapped_path, true);
		}

		while(! bed->atEnd())
		{
			BedLine l = BedLine::fromString(bed->readLine());

			try
			{
				// convert to 1-based coordinates for lifting
				BedLine lifted_line = reader.lift(l.chr(), l.start()+1, l.end());
				//convert back to 0-based for bed file.
				lifted_line.setStart(lifted_line.start()-1);

				if (lifted_line.length() > l.length()+l.length()*(max_inc/100.0))
				{
					THROW(ArgumentException, "Region increased in size more than " + QString::number(max_inc) + "%.");
				}

				if (! lifted_line.chr().isNonSpecial() && remove_special_chr)
				{
					THROW(ArgumentException, "Region was mapped to a special chromosome.");
				}

				lifted->write(lifted_line.toString(false).toLatin1() + "\n");
			}
			catch (ArgumentException& e)
			{
				if (! unmapped.isNull())
				{
					unmapped->write("# " + e.message().toLatin1() + "\n");
					unmapped->write(l.toString(false).toLatin1() + "\n");
				}
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

