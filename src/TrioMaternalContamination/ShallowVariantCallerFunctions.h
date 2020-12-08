#pragma once

#include "Auxilary.h"
#include "NGSHelper.h"
#include "VariantList.h"

void getVariantInformation(
		VariantInfo& vInfo,
		const  VcfFile& variant_list,
		int min_depth,
		int min_alt_count,
		std::unordered_set< VcfLine>& homozygousVariants,
		const QString& ref_file
		);

void countOccurencesOfVariants(
		const std::unordered_map<Member, VariantInfo, EnumHash>& trio,
		VariantInheritance& variantData
		);
