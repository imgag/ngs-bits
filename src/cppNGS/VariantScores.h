#ifndef VARIANTSCORES_H
#define VARIANTSCORES_H

#include "cppNGS_global.h"
#include "VariantList.h"
#include "BedFile.h"
#include "Phenotype.h"

class CPPNGSSHARED_EXPORT VariantScores
{
public:
	VariantScores();

	//Returns the list of algorithms
	QStringList algorithms();

	QList<double> score(QString algorithm, const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois);

private:
	QStringList algorithms_;

	QList<double> score_GSvar_V1(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois);
};

#endif // VARIANTSCORES_H
