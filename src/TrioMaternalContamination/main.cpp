#include "BedFile.h"
#include "ToolBase.h"
#include "Exceptions.h"
#include <QFileInfo>

#include "forward.h"
#include "ShallowVariantCallerFunctions.h"

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
		setDescription("Detects maternal contamination in a child employing shallow parental sequencing data.");
		addInfile("bam_m", "Input BAM file of mother.", false);
		addInfile("bam_f", "Input BAM file of father.", false);
		addInfile("bam_c", "Input BAM file of child.", false);

		//optional
		addInt("min_depth", "Minimum depth to consider a base.", true, 3);
		addInt("min_alt_count", "Minimum number of alterations to call a variant.", true, 1);

		addEnum("build", "Genome build used to generate the input (needed for contamination only).", true, QStringList() << "hg19" << "hg38", "hg19");

		changeLog(2020,  5,  13, "Initial version of the tool.");
	}

	virtual void main()
    {
		QTextStream out(stdout);

        //init
		QString bam_m = getInfile("bam_m");
		QString bam_f = getInfile("bam_f");
		QString bam_c = getInfile("bam_c");

		int min_depth = getInt("min_depth");
		int min_alt_count = getInt("min_alt_count");

		QString build = getEnum("build");

		//get variants of family
		std::unordered_map<Member, VariantInfo, EnumHash> trio;

		trio.emplace(Member::MOTHER, VariantInfo(bam_m));
		trio.emplace(Member::FATHER, VariantInfo(bam_f));
		trio.emplace(Member::CHILD, VariantInfo(bam_c));

		VariantList variant_list = NGSHelper::getKnownVariants(build, true);

		for(auto& trio_it : trio)
		{
			getVariantInformation(trio_it.second, variant_list, min_depth, min_alt_count);
		}

		VariantInheritance variantData;
		countOccurencesOfVariants(trio, variantData);

		out << "Percentage of mother variants passed to child: " << variantData.percentOfMotherToChild << "\n"
			<< "Percentage of father variants passed to child: " << variantData.percentOfFatherToChild << "\n"
			<< "Percentage of variants seen in both parents passed to child: " << variantData.percentOfBothToChild << "\n\n"

			<< "Percentage of child variants seen in mother: " << variantData.percentageOfInheritedMotherVariants << "\n"
			<< "Percentage of child variants seen in father: " << variantData.percentageOfInheritedFatherVariants << "\n"
			<< "Percentage of child variants seen in both parents: " << variantData.percentageOfInheritedCommonVariants << "\n"
			<< "Percentage of child variants, which are new: " << variantData.percentageOfNewVariants;

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

