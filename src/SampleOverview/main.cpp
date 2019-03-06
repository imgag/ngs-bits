#include "ToolBase.h"
#include "NGSHelper.h"

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
		setDescription("Creates a variant overview table from several samples.");
		addInfileList("in", "Input variant lists in GSvar format.", false);
		addOutfile("out", "Output variant list file in GSvar format.", false);
		//optional
		addInt("window", "Window to consider around indel positions to compensate for differing alignments.", true, 100);
		addString("add_cols", "Comma-separated list of input columns that shall be added to the output. By default, all columns that are present in all input files.", true, "[auto]");
	}

	virtual void main()
	{
		//init
		QStringList in = getInfileList("in");
		QString out = getOutfile("out");
		int window = getInt("window");
		QStringList add_cols = getString("add_cols").split(',');
		bool cols_auto = (add_cols.count()==1 && add_cols[0]=="[auto]");
		if (cols_auto) add_cols.clear();

		NGSHelper::createSampleOverview(in, out, window, cols_auto, add_cols);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
