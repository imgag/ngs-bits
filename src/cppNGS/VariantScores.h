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
		//Score explainations per variant.
		QList<QStringList> score_explainations;
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
	static Result score(QString algorithm, const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const QList<Variant>& blacklist);

	//Annotates a variant list with the scoring result. Returns the number of variants that were scored.
	static int annotate(VariantList& variants, const Result& result, bool add_explainations = false);

	//Returns the variant blackist from the settings file.
	static QList<Variant> blacklist();

private:
	static Result score_GSvar_V1(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const QList<Variant>& blacklist);
	static Result score_GSvar_V1_noNGSD(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const QList<Variant>& blacklist);
};

#endif // VARIANTSCORES_H
