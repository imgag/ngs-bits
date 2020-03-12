#ifndef VARIANTTYPE_H
#define VARIANTTYPE_H


//Variant types
enum class VariantType
{
	SNVS_INDELS, //Small variants - germline (SNVs and small InDels)
	CNVS, //CNVs - germline
	SVS, //Structural variants
	INVALID
};

#endif // VARIANTTYPE_H
