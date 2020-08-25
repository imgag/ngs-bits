#include "ToolBase.h"
#include "VcfFile.h"

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
        addInfile("in", "Input variant list in VCF format.", false, true);
		addOutfile("out", "Output variant list.", false, true);
		//optional
		addFlag("qual", "Also sort according to variant quality. Ignored if 'fai' file is given.");
		addInfile("fai", "FAI file defining different chromosome order.", true, true);
		addInt("comp", "Compression level for the output vcf file)", true, Z_BEST_SPEED);

		changeLog(2020, 8, 12, "Added parameter '-comp' for compression level of output vcf files.");
	}

	virtual void main()
	{
		//init
		QString fai = getInfile("fai");
		bool qual = getFlag("qual");

		//sort
        VcfFile vl;
        vl.load(getInfile("in"), true);
		if (fai=="")
		{
			vl.sort(qual);
		}
		else
		{
			vl.sortByFile(fai);
		}

		int compression_level = getInt("comp");
		vl.store(getOutfile("out"), false, compression_level);
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

