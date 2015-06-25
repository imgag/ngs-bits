#include "BedFile.h"
#include "ToolBase.h"

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
		setDescription("Merges overlapping regions in a BED file.");
		addInfile("in", "Input BED file.", false, true);
		addOutfile("out", "Output BED file.", false, true);
		//optional
		addFlag("keep_b2b", "Do not merge non-overlapping but adjacent (back-to-back) regions.");
		addFlag("merge_names", "Merge name columns instead of removing all annotations.");
	}

	virtual void main()
	{
		BedFile file;
		file.load(getInfile("in"));
		file.merge(!getFlag("keep_b2b"), getFlag("merge_names"));
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
