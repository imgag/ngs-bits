#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "FilterCascade.h"
#include "BedFile.h"
#include "Helper.h"
#include "NGSHelper.h"

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
		setDescription("Filter a variant list in GSvar format based on variant annotations.");
		addInfile("filters", "Filter definition file.", false);
		//optional
		addInfile("in", "Input variant list in GSvar format.", true);
		addOutfile("out", "Output variant list in GSvar format.", true);

		setExtendedDescription(extendedDescription());

		changeLog(2025, 6,  5, "Made input and output files optional.");
		changeLog(2018, 7, 30, "Replaced command-line parameters by INI file and added many new filters.");
		changeLog(2017, 6, 14, "Refactoring of genotype-based filters: now also supports multi-sample filtering of affected and control samples.");
		changeLog(2017, 6, 14, "Added sub-population allele frequency filter.");
		changeLog(2016, 6, 11, "Initial commit.");
	}

	QStringList extendedDescription()
	{
		QStringList output;

		//file format
		output << "The filter definition file lists one filter per line using the following syntax:";
		output << "name[tab]param1=value[tab]param2=value...";
		output << "";
		output << "The order in the filter definition file defines the order in which the filters are applied.";
		output << "";
		output << "Several of the filters offer more than one action:";
		output << "  FILTER - Remove variants if they do not match the filter.";
		output << "  REMOVE - Remove variants if they match the filter.";
		output << "  KEEP - Force variants to be kept, even if filtered out by previous filter steps.";
		output << "";

		//filters
		output << "The following filters are supported:";
		QStringList filter_names = FilterFactory::filterNames(VariantType::SNVS_INDELS);
		//determine maximum filter name length for intentation
		int max_len = 0;
		foreach (QString name, filter_names)
		{
            max_len = std::max(SIZE_TO_INT(max_len), SIZE_TO_INT(name.length()));
		}
		//add filters
		foreach (QString name, filter_names)
		{
			QStringList filter_description = FilterFactory::create(name)->description(true);
			for(int i=0; i<filter_description.count(); ++i)
			{
				if (i==0)
				{
					output << name.leftJustified(max_len) + " " + filter_description[0];
				}
				else
				{
					output << QString(max_len+1, ' ') + filter_description[i];
				}
			}
		}

		return output;
	}

	virtual void main()
	{
		//load variants
		QString in = getInfile("in");
		VariantList variants;
		variants.load(in);

		//create filter cascade
		FilterCascade filter_cascade;
		filter_cascade.load(getInfile("filters"));

		//apply filters
		FilterResult result = filter_cascade.apply(variants);
		result.removeFlagged(variants);

		//store result
		variants.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
