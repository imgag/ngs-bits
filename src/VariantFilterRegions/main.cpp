#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
#include "Helper.h"
#include "FilterCascade.h"

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
		setDescription("Filter a variant list based on a target region.");
		addInfile("in", "Input variant list. In VCF (default) or GSvar format.", false);
		//optional
		addOutfile("out", "Output variant list (same format as 'in'). If unset, writes to STDOUT.", true);
		addInfile("reg", "Input target region in BED format.", true);
		addString("r", "Single target region in the format chr17:41194312-41279500.", true);
		addString("mark", "If set, instead of removing variants, they are marked with the given flag in the 'filter' column.", true);
		addFlag("inv", "Inverts the filter, i.e. variants inside the region are removed/marked.");
		addEnum("mode", "Mode (input format).", true, QStringList() << "vcf" << "gsvar", "vcf");
		addInt("compression_level", "Output VCF compression level from 1 (fastest) to 9 (best compression). If unset, an unzipped VCF is written.", true, BGZF_NO_COMPRESSION);

		changeLog(2020, 8, 12, "Added parameter '-compression_level' for compression level of output vcf files.");
		changeLog(2018, 1, 23, "Added parameter '-inv' and made parameter '-mark' a string parameter to allow custom annotations names.");
		changeLog(2017, 1,  4, "Added parameter '-mark' for flagging variants instead of filtering them out.");
		changeLog(2016, 6, 10, "Added single target region parameter '-r'.");
	}

	virtual void main()
	{
		//init
		bool inv = getFlag("inv");
		QByteArray mark = getString("mark").toUtf8();

		//load target region
		BedFile roi;
		if (getInfile("reg")!="")
		{
			roi.load(getInfile("reg"));
			roi.merge();
		}
		else if (getString("r")!="")
		{
			roi.append(BedLine::fromString(getString("r")));
		}
		else
		{
			THROW(ArgumentException, "You have to provide either the 'reg' or the 'r' parameter!");
		}

		QString mode = getEnum("mode");

		//apply filter
		if(mode=="vcf")
		{
			VcfFile variants;
			if (mark!="")
			{
                variants.load(getInfile("in"), true);
				FilterResult filter_result(variants.count());
				FilterRegions::apply(variants, roi, filter_result);
				if (inv) filter_result.invert();
				filter_result.tagNonPassing(variants, mark, "Variant marked as '" + mark + "'.");
			}
			else
			{
				variants.load(getInfile("in"), roi, true, inv);
			}
			int compression_level = getInt("compression_level");
			variants.store(getOutfile("out"), true, compression_level);
		}
		else if(mode=="gsvar")
		{
			VariantList variants;
			if (mark!="")
			{
				variants.load(getInfile("in"));
				FilterResult filter_result(variants.count());
				FilterRegions::apply(variants, roi, filter_result);
				if (inv) filter_result.invert();
				filter_result.tagNonPassing(variants, mark, "Variant marked as '" + mark + "'.");
			}
			else
			{
				variants.load(getInfile("in"), roi, inv);
			}
			variants.store(getOutfile("out"));
		}
		else
		{
			THROW(ProgrammingException, "Invalid mode " + mode + "!");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
