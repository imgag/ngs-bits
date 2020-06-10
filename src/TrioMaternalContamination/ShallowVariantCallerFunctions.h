#pragma once

#include "forward.h"
#include "NGSHelper.h"
#include "VariantList.h"

void getVariantInformation(
		VariantInfo& vInfo,
		const Member mem,
		const VariantList& variant_list,
		int min_depth,
		int min_alt_count,
		std::unordered_set<Variant>& homozygousVariants);

void countOccurencesOfVariants(
		const std::unordered_map<Member, VariantInfo, EnumHash>& trio,
		VariantInheritance& variantData
		);
