#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
#include "GPD.h"
#include "Log.h"

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
		setDescription("Annotates a variant list with information from the GPD.");
		addInfile("in", "Input variant list.", false, true);
		addOutfile("out", "Output variant list.", false, true);
		//optional
		addString("psname", "Processed sample name. If set this name is used instead of the file name to find the sample in the DB.", true, "");
		addInfile("ref", "Reference genome FASTA file. If unset '/mnt/share/data/dbs/genomes/hg19.fa' is used.", true, false);
	}

	virtual void main()
	{
		//determine refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="")
		{
#ifdef WIN32
			ref_file = "W:\\share\\data\\dbs\\genomes\\hg19_win.fa";
#else
			ref_file = "/mnt/share/data/dbs/genomes/hg19.fa";
#endif
		}

		//load
		VariantList variants;
		variants.load(getInfile("in"));
		QString ps = getString("psname");
		if (ps=="") ps = getInfile("in");

		//annotate
		GPD().annotate(variants);

		//store
		variants.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
