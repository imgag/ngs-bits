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
		case VariantType::RES:
			return "RE";
		default:
			THROW(ProgrammingException, "Unhandled variant type!");
	}
}

VariantType stringToVariantType(QString str)
{
	if (str=="small variant") return VariantType::SNVS_INDELS;
	if (str=="CNV") return VariantType::CNVS;
	if (str=="SV") return VariantType::SVS;
	if (str=="RE") return VariantType::RES;
	THROW(ProgrammingException, "Unhandled variant type string '" + str + "'!");
}
