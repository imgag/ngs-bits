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
		setDescription("Adds the regions in two BED files.");
		addInfile("in2", "Second input BED file.", false);
		//optional
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
	}

	virtual void main()
	{
		//load
		BedFile in;
		in.load(getInfile("in"));

		//merge
		BedFile in2;
		in2.load(getInfile("in2"));
		for (int i=0; i<in2.count(); ++i)
		{
			in.append(in2[i]);
		}

		//store
		in.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
