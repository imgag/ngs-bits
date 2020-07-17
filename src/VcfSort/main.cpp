#include "ToolBase.h"
#include "VcfFileHandler.h"

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
		setDescription("Sorts variant lists according to chromosomal position.");
		addInfile("in", "Input variant list.", false, true);
		addOutfile("out", "Output variant list.", false, true);
		//optional
		addFlag("qual", "Also sort according to variant quality. Ignored if 'fai' file is given.");
		addInfile("fai", "FAI file defining different chromosome order.", true, true);
	}

	virtual void main()
	{
		//init
		QString fai = getInfile("fai");
		bool qual = getFlag("qual");

		//sort
		 VcfFileHandler vl;
		vl.load(getInfile("in"), true);
		if (fai=="")
		{
			vl.sort(qual);
		}
		else
		{
			vl.sortByFile(fai);
		}
		vl.store(getOutfile("out"));
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

