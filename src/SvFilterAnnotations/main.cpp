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
        setDescription("Filter a structural variant list in BEDPE format based on variant annotations.");
        addInfile("in", "Input structural variant list in BEDPE format.", false);
        addOutfile("out", "Output structural variant list in BEDPE format.", false);
		addInfile("filters", "Filter definition file.", false);

		setExtendedDescription(extendedDescription());

        changeLog(2020, 4, 16, "Initial version of the tool. Based on VariantFilterAnnotations.");
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
        QStringList filter_names = FilterFactory::filterNames(VariantType::SVS);
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
        BedpeFile svs;
        svs.load(in);

		//create filter cascade
		FilterCascade filter_cascade;
		QStringList filters_file = Helper::loadTextFile(getInfile("filters"), true, '#', true);
		foreach(QString filter_line, filters_file)
		{
			QStringList parts = filter_line.split("\t");
			QString name = parts[0];
			filter_cascade.add(FilterFactory::create(name, parts.mid(1)));
		}

		//apply filters
        FilterResult result = filter_cascade.apply(svs);
        result.removeFlagged(svs);

		//store result
        svs.toTSV(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
