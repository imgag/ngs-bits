#ifndef VARIANTTYPE_H
#define VARIANTTYPE_H

#include "cppNGS_global.h"


//Variant types
enum class VariantType
{
	SNVS_INDELS, //Small variants - germline (SNVs and small InDels)
	CNVS, //CNVs - germline
	SVS, //Structural variants
	RES, //Repeat expansion
	INVALID
};

//Type to string conversion
QString CPPNGSSHARED_EXPORT variantTypeToString(VariantType type);
VariantType CPPNGSSHARED_EXPORT stringToVariantType(QString str);

#endif // VARIANTTYPE_H
