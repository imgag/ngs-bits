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
		//Algorithm name
		QString algorithm;
		//Scores per variant. Scores below 0 indicate that no score was calculated for the variant.
		QList<double> scores;
		//Ranks per variant. Ranks below 0 indicate that no rank was calculated for the variant.
		QList<int> ranks;
		//General warnings.
		QStringList warnings;
	};

	//Default constructor
	VariantScores();

	//Returns the list of algorithms
	static QStringList algorithms();

	//Returns the algorithm description.
	static QString description(QString algorithm);

	//Returns a variant scores. Throws an error if the input is invalid.
	static Result score(QString algorithm, const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois);

	//Annotates a variant list with the scoring result.
	static void annotate(VariantList& variants, const Result& result);

private:
	static Result score_GSvar_V1(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois);
	static Result score_GSvar_V1_noNGSD(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois);
};

#endif // VARIANTSCORES_H
