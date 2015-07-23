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
		setDescription("Intersects the regions in two BED files.");
		addInfile("in2", "Second input BED file.", false);
		QStringList output_options;
		output_options << "intersect" << "in" << "in2";
		addEnum("mode", "Output mode: intersect of both files (intersect), original entry of file 1 (in) or original entry of file 2 (in2).", true, output_options, "intersect");
		//optional
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
	}

	virtual void main()
	{
		//input
		BedFile in;
		in.load(getInfile("in"));
		BedFile in2;
		in2.load(getInfile("in2"));
		
		//calculate
		QString mode = getEnum("mode");
		if (mode=="intersect")
		{
			if (!in2.isMergedAndSorted()) in2.merge();
			in.intersect(in2);
			in.store(getOutfile("out"));
		}
		else if (mode=="in")
		{
			if (!in2.isMergedAndSorted()) in2.merge();
			in.overlapping(in2);
			in.store(getOutfile("out"));
		}
		else if (mode=="in2")
		{
			if (!in.isMergedAndSorted()) in.merge();
			in2.overlapping(in);
			in2.store(getOutfile("out"));
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
