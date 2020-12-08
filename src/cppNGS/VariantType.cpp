#include "VariantType.h"

QString variantTypeToString(VariantType type)
{
	switch(type)
	{
		case VariantType::SNVS_INDELS:
			return "small variant";
		case VariantType::CNVS:
			return "CNV";
		case VariantType::SVS:
			return "SV";
		default:
			THROW(ProgrammingException, "Unhandled variant type!");
	}
}
