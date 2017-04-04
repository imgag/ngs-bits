#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
#include "Helper.h"
#include "VariantFilter.h"

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
		addInfile("in", "Input variant list.", false);
		addOutfile("out", "Output variant list.", false);
		//optional
		addInfile("reg", "Input target region in BED format.", true);
		addString("r", "Single target region in the format chr17:41194312-41279500.", true);
		addFlag("mark", "If set, the variants are not removed but marked as 'off-target' in the 'filter' column.");

		changeLog(2017, 1,  4, "Added parameter '-mark' for flagging variants instead of filtering them out.");
		changeLog(2016, 6, 10, "Added single target region parameter '-r'.");
	}

	virtual void main()
	{
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

		//apply filter
		VariantList variants;
		if (getFlag("mark"))
		{
			variants.load(getInfile("in"));
			VariantFilter filter(variants);
			filter.flagByRegions(roi);
			filter.tagFlagged("off-target", "Variant outside the panel/exome target region.");
		}
		else
		{
			variants.load(getInfile("in"), VariantList::AUTO, &roi);
		}
		variants.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
