#include "ShallowVariantCaller.h"

void getVariantInformation(
		VariantInfo& vInfo,
		const int& min_depth,
		const int& min_alt_count)
{
	vInfo.variants = NGSHelper::getKnownVariants(vInfo.in_file_name, true);

	BamReader reader(vInfo.in_file_name);
	for (int i=0; i<vInfo.variants.count(); ++i)
	{
		const Variant& v = vInfo.variants[i];

		if (!v.isSNV()) continue;
		if (!v.chr().isAutosome()) continue;

		Pileup pileup_tu = reader.getPileup(v.chr(), v.start());
	}

}
