#ifndef LOVDUPLOADFILE_H
#define LOVDUPLOADFILE_H

#include "cppNGS_global.h"
#include "VariantList.h"
#include "Phenotype.h"

class CPPNGSSHARED_EXPORT LovdUploadFile
{
	public:
		static QString create(QString sample, QString gender, Phenotype pheno, const VariantList& vl, int variant_index);

	protected:
		static QString getSettings(QString key);
		LovdUploadFile(); //declared 'away'
};

#endif // LOVDUPLOADFILE_H
