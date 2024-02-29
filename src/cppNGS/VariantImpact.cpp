#include "VariantImpact.h"
#include "Exceptions.h"

QByteArray variantImpactToString(VariantImpact type)
{
	if (type == VariantImpact::MODIFIER)
	{
		return "MODIFIER";
	}
	else if (type == VariantImpact::LOW)
	{
		return "LOW";
	}
	else if (type == VariantImpact::MODERATE)
	{
		return "MODERATE";
	}
	else if (type == VariantImpact::HIGH)
	{
		return "HIGH";
	}
	else
	{
		THROW(ProgrammingException, "Unhandeled variantImpact type!");
	}
}

VariantImpact stringToVariantImpact(QString str)
{
	str = str.toUpper();

	if (str == "MODIFIER")
	{
		return VariantImpact::MODIFIER;
	}
	else if (str == "LOW")
	{
		return VariantImpact::LOW;
	}
	else if (str == "MODERATE")
	{
		return VariantImpact::MODERATE;
	}
	else if (str == "HIGH")
	{
		return VariantImpact::HIGH;
	}
	else
	{
		THROW(ArgumentException, "Unknown string for variant impact! String:" + str);
	}
}

bool lowerImpactThan(VariantImpact left, VariantImpact right)
{
	return (static_cast<int>(left) < static_cast<int>(right));
}
