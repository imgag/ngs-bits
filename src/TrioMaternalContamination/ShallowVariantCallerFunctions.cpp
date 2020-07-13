#include "ShallowVariantCallerFunctions.h"

void getVariantInformation(
		VariantInfo& vInfo,
		const VcfFormat::VcfFileHandler& variant_list,
		int min_depth,
		int min_alt_count,
		std::unordered_set<VcfFormat::VCFLine>& homozygousVariants)
{
	BamReader reader(vInfo.in_file_name);
	for (int i=0; i<variant_list.count(); ++i)
	{
		const VcfFormat::VCFLine& v = variant_list[i];
		if (!v.isSNV()) continue;
		if (!v.chr().isAutosome()) continue;

		Pileup pileup_tu = reader.getPileup(v.chr(), v.start());

		//keep only variants of minimum depth
		if (pileup_tu.depth(true) < min_depth) continue;

		long long count = pileup_tu.countOf(v.altString()[0]);
		double frequency = pileup_tu.frequency(v.ref()[0], v.altString()[0]);

		//do not keep homozygous variants
		if(frequency==1)
		{
			homozygousVariants.insert(v);
			continue;
		}
		if(homozygousVariants.find(v)!=homozygousVariants.end())
		{
			continue;
		}

		//only keep variants with a minimum base count
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
	const std::unordered_map<const VcfFormat::VCFLine, double>& variants_mother = trio.at(Member::MOTHER).variants;
	const std::unordered_map<const VcfFormat::VCFLine, double>& variants_father = trio.at(Member::FATHER).variants;
	const std::unordered_map<const VcfFormat::VCFLine, double>& variants_child = trio.at(Member::CHILD).variants;

	double variants_of_mother_in_child = 0;
	double variants_of_father_in_child = 0;

	double mother_variants = 0;
	double father_variants = 0;

	//count mother variants
	for(const std::pair<const VcfFormat::VCFLine, double>& map_element : variants_mother)
	{
		const VcfFormat::VCFLine v = map_element.first;
		if(variants_father.find(v) == variants_father.end())
		{
			if(variants_child.find(v) != variants_child.end())
			{
				++variants_of_mother_in_child;
			}
			++mother_variants;
		}
	}
	variantData.percentOfMotherToChild = variants_of_mother_in_child / mother_variants;

	//count father variants
	for(const std::pair<const VcfFormat::VCFLine, double>& map_element : variants_father)
	{
		const VcfFormat::VCFLine v = map_element.first;
		if(variants_mother.find(v) == variants_mother.end())
		{
			if(variants_child.find(v) != variants_child.end())
			{
				++variants_of_father_in_child;
			}
			++father_variants;
		}
	}
	variantData.percentOfFatherToChild = variants_of_father_in_child / father_variants;

}
