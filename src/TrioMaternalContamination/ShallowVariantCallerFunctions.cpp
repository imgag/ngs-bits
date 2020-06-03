#include "ShallowVariantCallerFunctions.h"
#include "QDebug"

void getVariantInformation(
		VariantInfo& vInfo,
		const VariantList& variant_list,
		int min_depth,
		int min_alt_count)
{
	BamReader reader(vInfo.in_file_name);
	for (int i=0; i<variant_list.count(); ++i)
	{
		const Variant& v = variant_list[i];
		if (!v.isSNV()) continue;
		if (!v.chr().isAutosome()) continue;

		Pileup pileup_tu = reader.getPileup(v.chr(), v.start());
		if (pileup_tu.depth(true) < min_depth) continue;

		long long count = pileup_tu.countOf(v.obs()[0]);
		double frequency = pileup_tu.frequency(v.ref()[0], v.obs()[0]);

		if(count >= min_alt_count)
		{
			vInfo.variants[v] = frequency;
		}
	}
}


void countOccurencesOfVariants(
		const std::unordered_map<Member, VariantInfo, EnumHash>& trio,
		VariantInheritance& variantData
		)
{
	std::unordered_map<const Variant, double> variants_mother = trio.at(Member::MOTHER).variants;
	std::unordered_map<const Variant, double> variants_father = trio.at(Member::FATHER).variants;
	std::unordered_map<const Variant, double> variants_child = trio.at(Member::CHILD).variants;

	double common_variants_in_child = 0;
	double total_common_variants = 0;

	double variants_of_mother_in_child = 0;
	double variants_of_father_in_child = 0;

	double new_variants = 0;

	double score_varaints_of_mother_in_child = 0;
	double score_varaints_of_father_in_child = 0;
	for(const std::pair<const Variant,double>& map_element : variants_mother)
	{
		const Variant v = map_element.first;
		if(variants_father.find(v) != variants_father.end())
		{
			 if(variants_child.find(v) != variants_child.end())
			{
				 ++common_variants_in_child;
			}
			++total_common_variants;
		}

		if(variants_child.find(v) != variants_child.end())
		{
			++variants_of_mother_in_child;
			score_varaints_of_mother_in_child += (1 / variants_child[v]);
		}
	}
	variantData.percentOfMotherToChild = variants_of_mother_in_child / variants_mother.size();
	variantData.percentOfBothToChild = common_variants_in_child / total_common_variants;

	for(const std::pair<const Variant,double>& map_element : variants_father)
	{
		const Variant v = map_element.first;
		if(variants_child.find(v) != variants_child.end())
		{
			++variants_of_father_in_child;
			score_varaints_of_father_in_child += (1 / variants_child[v]);
		}
	}
	variantData.percentOfFatherToChild = variants_of_father_in_child / variants_father.size();

	for(const std::pair<const Variant,double>& map_element : variants_child)
	{
		const Variant v = map_element.first;
		if(variants_mother.find(v) == variants_mother.end() && variants_father.find(v) == variants_father.end())
		{
			++new_variants;
		}
	}
	variantData.percentageOfNewVariants = new_variants / variants_child.size();
	variantData.percentageOfInheritedCommonVariants = common_variants_in_child / variants_child.size();
	variantData.percentageOfInheritedMotherVariants = variants_of_mother_in_child / variants_child.size();
	variantData.percentageOfInheritedFatherVariants = variants_of_father_in_child / variants_child.size();

	//Q_ASSERT(variants_child.size() == common_variants_in_child + new_variants + variants_of_mother_in_child + variants_of_father_in_child);
	qDebug() << "DEBUG DATA: " << variants_child.size() << common_variants_in_child << new_variants << variants_of_mother_in_child << variants_of_father_in_child
			 << "sizes: " << variants_child.size() << variants_father.size() << variants_mother.size() << total_common_variants;

	qDebug() << "SCORES " << score_varaints_of_mother_in_child/score_varaints_of_father_in_child
			 << variantData.percentOfMotherToChild/variantData.percentOfFatherToChild;
}
