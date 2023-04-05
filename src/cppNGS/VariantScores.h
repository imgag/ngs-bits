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

	//Scoring parameters
	struct Parameters
	{
		bool use_ngsd_classifications = true;
		bool use_blacklist = false;
	};

	//Scoring result
	struct Result
	{
		//Algorithm name
		QString algorithm;
		//Scores per variant. Scores below 0 indicate that no score was calculated for the variant. -1 means that the variant did not pass the pre-filtering. -2 means that the variant was blacklisted.
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
	static Result score(QString algorithm, const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const Parameters& parameters);

	//Annotates a variant list with the scoring result. Returns the number of variants that were scored.
	static int annotate(VariantList& variants, const Result& result, bool add_explainations = false);

private:
	//Returns the variant blackist from the settings file.
	static QList<Variant> loadBlacklist();

	static Result score_GSvar_v1(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const Parameters& parameters);
	static Result score_GSvar_v2_dominant(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const Parameters& parameters);
	static Result score_GSvar_v2_recessive(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const Parameters& parameters);
};

#endif // VARIANTSCORES_H
