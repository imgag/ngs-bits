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
		addFlag("remove_unused_contigs", "Remove comment lines of contigs, i.e. chromosomes, that are not used in the output VCF.");

		changeLog(2022, 12,  8, "Added parameter '-remove_unused_contigs'.");
		changeLog(2020,  8, 12, "Added parameter '-compression_level' for compression level of output VCF files.");
	}

	virtual void main()
	{
		//init
		QString fai = getInfile("fai");
		bool qual = getFlag("qual");
		bool remove_unused_contigs = getFlag("remove_unused_contigs");

		//load
		VcfFile vl;
		vl.load(getInfile("in"));

		//sort
		if (fai=="")
		{
			vl.sort(qual);
		}
		else
		{
			vl.sortByFile(fai);
		}

		//remove unused contig headers
		if (remove_unused_contigs)
		{
			vl.removeUnusedContigHeaders();
		}

		//store
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

