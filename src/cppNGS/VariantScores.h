#ifndef VARIANTSCORES_H
#define VARIANTSCORES_H

#include "cppNGS_global.h"
#include "VariantList.h"
#include "BedFile.h"
#include "Phenotype.h"

//Variant scoring/ranking class.
class CPPNGSSHARED_EXPORT VariantScores
{
public:
	//Result of the scoring
	struct Result
	{
		QList<double> scores;
		QStringList warnings;
	};

	//Default constructor
	VariantScores();

	//Returns the list of algorithms
	QStringList algorithms();

	//Returns the algorithm description.
	QString description(QString algorithm) const;

	//Returns a variant scores. Throws an error if the input is invalid.
	Result score(QString algorithm, const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois) const;

private:
	QStringList algorithms_;

	Result score_GSvar_V1(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois) const;
	Result score_GSvar_V1_noNGSD(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois) const;
};

#endif // VARIANTSCORES_H
