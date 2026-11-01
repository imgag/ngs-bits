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
	};

	//Scoring result
	struct Result
	{
		//Algorithm name
		QByteArray algorithm;
		//Scores per variant. Scores below 0 indicate that no score was calculated for the variant. -1 means that the variant did not pass the pre-filtering.
		QList<double> scores;
		//Score explanations per variant.
		QList<QByteArrayList> score_explanations;
		//Ranks per variant. Ranks below 0 indicate that no rank was calculated for the variant.
		QList<int> ranks;
	};

	//Default constructor
	VariantScores();

	//Returns the list of algorithms
	static QByteArrayList algorithms();

	//Returns the algorithm description.
	static QByteArray description(QByteArray algorithm);

	//Returns a variant scores. Throws an error if the input is invalid.
	static Result score(QByteArray algorithm, const VariantList& variants, const QHash<Phenotype, BedFile>& phenotype_rois, const Parameters& parameters);

	//Annotates a variant list with the scoring result. Returns the number of variants that were scored.
	static int annotate(VariantList& variants, const Result& result, bool add_explanations = false);

private:
	//Returns the variant blackist from the settings file.
	static QByteArrayList prefilters(const Parameters& parameters);
	static Result score_GSvar_v3(const VariantList& variants, const QHash<Phenotype, BedFile>& phenotype_rois, const Parameters& parameters);
};

//Helper struct for scoring
class CategorizedScores
  : protected QHash<QByteArray, QHash<QByteArray, double>>
{
public:
	//Adds score (not gene-specific)
	void add(const QByteArray& category, double value);
	//Adds score for a specific gene
	void add(const QByteArray& category, double value, const QByteArray& gene);

	//Returns the maximum score of all genes
	double score(QByteArrayList& best_genes) const;

	//Returns the explanations for the given gene. If an empty gene is given, only the gene-independent scores are returned.
	QByteArrayList explainations(const QByteArray& gene) const;
	//Returns the explanations for the given genes.
	QByteArrayList explainations(const QByteArrayList& best_genes) const;
};

#endif // VARIANTSCORES_H
