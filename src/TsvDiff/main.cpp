#include "ToolBase.h"
#include "TsvFile.h"

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
		setDescription("Compares TSV files.");
		addInfile("in1", "Input TSV file. If unset, reads from STDIN.", false);
		addInfile("in2", "Input TSV file. If unset, reads from STDIN.", false);
		//optional
		addFlag("skip_header", "Do not compare header/comment lines.");
		addString("skip_cols", "Comma-separated list of colums to skip.", true);
	}

	virtual void main()
	{
		TsvFile in1;
		in1.load(getInfile("in1"));
		TsvFile in2;
		in2.load(getInfile("in2"));

		//compare headers
		//TODO

		//remove skipped columns
		//TODO

		//compare columns
		//TODO
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

