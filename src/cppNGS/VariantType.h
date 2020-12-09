#ifndef VARIANTTYPE_H
#define VARIANTTYPE_H

#include "cppNGS_global.h"
#include "Exceptions.h"


//Variant types
enum class VariantType
{
	SNVS_INDELS, //Small variants - germline (SNVs and small InDels)
	CNVS, //CNVs - germline
	SVS, //Structural variants
	INVALID
};

//Type to string conversion
QString CPPNGSSHARED_EXPORT variantTypeToString(VariantType type);

#endif // VARIANTTYPE_H
