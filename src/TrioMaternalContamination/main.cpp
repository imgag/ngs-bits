#include "BedFile.h"
#include "ToolBase.h"
#include "Exceptions.h"
#include <QFileInfo>
#include "Helper.h"

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

		addEnum("build", "Genome build used to generate the input.", true, QStringList() << "hg19" << "hg38", "hg19");
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);

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
		std::unordered_set<Variant> homozygousVariants;

		//find all variants
		for(auto& trio_it : trio)
		{
			getVariantInformation(trio_it.second, trio_it.first, variant_list, min_depth, min_alt_count, homozygousVariants);
		}

		//delete homozygous variants
		for(auto& trio_it : trio)
		{
			for (auto variant_it = trio_it.second.variants.begin(); variant_it != trio_it.second.variants.end();)
			{
				if(homozygousVariants.find(variant_it->first)!=homozygousVariants.end())
				{
					trio_it.second.variants.erase(variant_it++);
				}
				else
				{
					++variant_it;
				}
			}
		}

		VariantInheritance variantData;
		countOccurencesOfVariants(trio, variantData);

		//open stream
		QSharedPointer<QFile> file = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream stream(file.data());

		stream << "Percentage of mother variants passed to child: " << variantData.percentOfMotherToChild << "\n"
			   << "Percentage of father variants passed to child: " << variantData.percentOfFatherToChild << "\n";


	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

