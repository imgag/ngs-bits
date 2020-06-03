#pragma once

#include "forward.h"
#include "NGSHelper.h"
#include "VariantList.h"

void getVariantInformation(
		VariantInfo& vInfo,
		const VariantList& variant_list,
		int min_depth,
		int min_alt_count);

void countOccurencesOfVariants(
		std::unordered_map<Member, VariantInfo, EnumHash> trio,
		VariantHeritage& variantData
		);
