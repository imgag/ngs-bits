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
		addInfile("in1", "Input BED file one.", false, true);
		addInfile("in2", "Input BED file two.", false, true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true, true);
		QStringList output_options;
		output_options << "intersect" << "in1" << "in2";
		addEnum("mode", "Output mode: intersect of both files (intersect), original entry of file 1 (in1) or original entry of file 2 (in2).", true, output_options, "intersect");
	}

	virtual void main()
	{
		//input
		BedFile in1;
		in1.load(getInfile("in1"));
		BedFile in2;
		in2.load(getInfile("in2"));
		
		//calculate
		QString mode = getEnum("mode");
		if (mode=="intersect")
		{
			if (!in2.isMergedAndSorted()) in2.merge();
			in1.intersect(in2);
			in1.store(getOutfile("out"));
		}
		else if (mode=="in1")
		{
			if (!in2.isMergedAndSorted()) in2.merge();
			in1.overlapping(in2);
			in1.store(getOutfile("out"));
		}
		else if (mode=="in2")
		{
			if (!in1.isMergedAndSorted()) in1.merge();
			in2.overlapping(in1);
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
