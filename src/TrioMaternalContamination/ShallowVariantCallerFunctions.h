#pragma once

#include "Auxilary.h"
#include "NGSHelper.h"
#include "VariantList.h"

void getVariantInformation(
		VariantInfo& vInfo,
		const VcfFormat::VcfFileHandler& variant_list,
		int min_depth,
		int min_alt_count,
		std::unordered_set<VcfFormat::VCFLine>& homozygousVariants);

void countOccurencesOfVariants(
		const std::unordered_map<Member, VariantInfo, EnumHash>& trio,
		VariantInheritance& variantData
		);
