#include "BedFile.h"
#include "ToolBase.h"
#include "Exceptions.h"
#include <QFileInfo>

#include "forward.h"
#include "ShallowVariantCaller.h"

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
		addOutfile("out_m", "Output file of common variants in mother.", true);
		addOutfile("out_f", "Output file of common variants in father.", true);
		addOutfile("out_c", "Output file of common variants in child.", true);

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

		trio.emplace(Member::MOTHER, VariantInfo(bam_m, getOutfile("out_m")));
		trio.emplace(Member::FATHER, VariantInfo(bam_f, getOutfile("out_f")));
		trio.emplace(Member::CHILD, VariantInfo(bam_c, getOutfile("out_c")));

		VariantList variant_list = NGSHelper::getKnownVariants(build, true);

		for(auto& trio_it : trio)
		{
			//find variants of family member
			getVariantInformation(trio_it.second, variant_list, min_depth, min_alt_count);
			//write an output file with the variants for family member
			if(!trio_it.second.out_file_name.isEmpty())	trio_it.second.writeData();
		}

		VariantHeritage variantData;
		countOccurencesOfVariants(trio, variantData);

		out << "Common variants: " << variantData.commonVariants
			<< "Father variants: " << variantData.exclusiveVariantsOfFather
			<< "Mother variants: " << variantData.exclusiveVariantsOfMother
			<< "New variants: " << variantData.newVariants;

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

