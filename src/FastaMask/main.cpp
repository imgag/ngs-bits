#include "ToolBase.h"
#include "Helper.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"

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
		setDescription("Mask regions in a FASTA file with N bases.");
		addInfile("in", "Input FASTA file.", false);
		addInfile("reg", "Input BED file with regions to mask.", false);
		addOutfile("out", "Output file.", false);

		//changelog
		changeLog(2024,  8, 17, "Initial version.");
	}

	virtual void main()
	{
		//input checks
		if (getInfile("in")==getOutfile("out")) THROW(Exception, "'in' and 'out' cannot be the same file!");

		//load region and build index
		BedFile reg;
		reg.load(getInfile("reg"));
		reg.merge();
		ChromosomalIndex<BedFile> reg_idx(reg);

		//parse from file
		Chromosome current_chr;
		QSharedPointer<QFile> in = Helper::openFileForReading(getInfile("in"));
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"));
		int pos = 1;
		while(!in->atEnd())
		{
			QByteArray line = in->readLine();
			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

			//skip empty lines
			if(line.isEmpty()) continue;

			if (line.startsWith(">"))
			{
				//determine chromosome name
				QByteArray chr = line.mid(1).trimmed();
				int space_idx = chr.indexOf(' ');
				if (space_idx!=-1) chr = chr.left(space_idx);
				current_chr = chr;

				//reset postion
				pos = 1;

				//write header
				out->write(line);
				out->write("\n");
			}
			else
			{
				//determine if we need to mask a base in this line
				QVector<int> indices = reg_idx.matchingIndices(current_chr, pos, pos+line.length()-1);
				if (indices.size()>0)
				{
					for (int i=0; i<line.length(); ++i)
					{
						foreach(int index, indices)
						{
							if (reg[index].overlapsWith(pos+i))
							{
								line[i] = 'N';
							}
						}
					}
				}

				//update position
				pos += line.length();

				//write header
				out->write(line);
				out->write("\n");
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
