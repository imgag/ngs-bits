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
		addOutfile("out", "Output variant list in VCF or VCF.GZ format.", false, true);
		//optional
		addFlag("qual", "Also sort according to variant quality. Ignored if 'fai' file is given.");
		addInfile("fai", "FAI file defining different chromosome order.", true, true);
		addInt("compression_level", "Output VCF compression level from 1 (fastest) to 9 (best compression). If unset, an unzipped VCF is written.", true, BGZF_NO_COMPRESSION);

		changeLog(2020, 8, 12, "Added parameter '-compression_level' for compression level of output vcf files.");
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

		int compression_level = getInt("compression_level");
		vl.store(getOutfile("out"), false, compression_level);
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

