#include "ShallowVariantCaller.h"
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
		std::unordered_map<Member, VariantInfo, EnumHash> trio,
		VariantHeritage& variantData
		)
{
	std::unordered_map<const Variant, double> variants_mother = trio.at(Member::MOTHER).variants;
	std::unordered_map<const Variant, double> variants_father = trio.at(Member::FATHER).variants;
	std::unordered_map<const Variant, double> variants_child = trio.at(Member::CHILD).variants;

	double frequ_score = 0;
	int homozygous_mother = 0;

	for(const std::pair<const Variant,double>& map_element : variants_child)
	{
		const Variant v = map_element.first;
		if(variants_mother.find(v) != variants_mother.end() && variants_father.find(v) != variants_father.end())
		{
			++variantData.commonVariants;
			if(variantData.commonVariants == 1)	qDebug() <<"common"<< v.chr().num() << v.start();

		}
		else if(variants_mother.find(v) != variants_mother.end() )
		{
			++variantData.exclusiveVariantsOfMother;
			frequ_score += variants_mother[v]/variants_child[v];
			if(variantData.exclusiveVariantsOfMother == 1)	qDebug() << "mother only" << v.chr().num() << v.start();
			if(variants_mother[v] == 1)	++homozygous_mother;

		}
		else if(variants_father.find(v) != variants_father.end() )
		{
			++variantData.exclusiveVariantsOfFather;
			if(variantData.exclusiveVariantsOfFather <= 50)	qDebug() << "FATHER only" << v.chr().num() << v.start();

		}
		else
		{
			++variantData.newVariants;
			if(variantData.newVariants <= 150)	qDebug() << "novo"<< v.chr().num() << v.start();
		}
	}
}
