#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"

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
		setDescription("Extends the regions in a BED file.");
		addInt("n", "The number of bases to extend (on both sides of each region).", false);
		//optional
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addInfile("fai", "Optional FASTA index file that determines the maximum position for each chromosome.", true);
	}

	virtual void main()
	{
		//load and extend file
		BedFile file;
		file.load(getInfile("in"));
		file.extend(getInt("n"));

		//optional: fix position bounds using the FAI file
		if (getInfile("fai")!="")
		{
			//load maxima for each chromosome (store using chromosome number)
			QMap<int, int> max;
			QStringList fai = Helper::loadTextFile(getInfile("fai"), true, '~', true);
			foreach(QString line, fai)
			{
				QStringList parts = line.split("\t");
				if (parts.count()<2) continue;
				bool ok = false;
				int value = parts[1].toInt(&ok);
				if (!ok) continue;
				max[Chromosome(parts[0]).num()] = value;
			}

			//apply maxima
			for (int i=0; i<file.count(); ++i)
			{
				BedLine& line = file[i];
				if (!max.contains(line.chr().num()))
				{
					THROW(ArgumentException, "Chromsome '" + line.chr().str() + "' not contained in FASTA index file '" + getInfile("fai") + "'!");
				}
				line.setEnd(std::min(line.end(), max[line.chr().num()]));
			}
		}

		//store file
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
