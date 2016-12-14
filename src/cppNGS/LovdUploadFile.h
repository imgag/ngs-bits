#ifndef LOVDUPLOADFILE_H
#define LOVDUPLOADFILE_H

#include "cppNGS_global.h"
#include "VariantList.h"
#include "Phenotype.h"

///Generator for LOVD upload file in JSON format
class CPPNGSSHARED_EXPORT LovdUploadFile
{
	public:
		static QByteArray create(QString sample, QString gender, QString gene, const Phenotype& pheno, const VariantList& vl, const Variant& variant);

	protected:
		static QString getSettings(QString key);
		static QString getAnnotation(const VariantList& vl, const Variant& variant, QString key);

		static QString convertGender(QString gender);
		static QString convertGenotype(QString genotype);
		static QString convertClassification(QString classification);
		static QString chromosomeToAccession(const Chromosome& chr);

		LovdUploadFile(); //declared 'away'
};

#endif // LOVDUPLOADFILE_H
