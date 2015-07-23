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
		setDescription("Filters variant lists accordning to a target region.");
		addInfile("in", "Input variant list.", false);
		addInfile("reg", "Input target region in BED format.", false);
		addFlag("invert", "If used, the variants inside the target region are removed.");
		//optional
		addOutfile("out", "Output variant list. If unset, writes to STDOUT.", true);
	}

	virtual void main()
	{
		//load variants
		VariantList variants;
		variants.load(getInfile("in"));

		//filter by region
		BedFile regions;
		regions.load(getInfile("reg"));
		regions.merge();
		variants.filterByRegions(regions, getFlag("invert"));

		//store variants
		variants.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
