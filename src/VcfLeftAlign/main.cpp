#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "VariantList.h"
#include "Settings.h"
#include <QFile>
#include <QTextStream>
#include <QList>

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
		setDescription("Shifts indels in a variant list as far to the left as possible. Complex indels and multi-allelic deletions are not shifted!");
		addInfile("in", "Input variant list.", false, true);
		addOutfile("out", "Output variant list.", false, true);
		//optional
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
	}

	virtual void main()
	{
		//determine refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome");

		//left align
		VariantList vl;
		vl.load(getInfile("in"));
        vl.leftAlign(ref_file);
		vl.store(getOutfile("out"));
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

