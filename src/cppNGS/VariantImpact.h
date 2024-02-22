#ifndef VARIANTIMPACT_H
#define VARIANTIMPACT_H

#include "cppNGS_global.h"
#include "QString"

///The Variant impact
enum class VariantImpact
{
	MODIFIER = 0,
	LOW = 1,
	MODERATE = 2,
	HIGH = 3


};

//Type <-> string conversions
QByteArray CPPNGSSHARED_EXPORT variantImpactToString(VariantImpact type);
VariantImpact CPPNGSSHARED_EXPORT stringToVariantImpact(QString str);

bool CPPNGSSHARED_EXPORT lowerImpactThan(VariantImpact left, VariantImpact right);


#endif // VARIANTIMPACT_H
