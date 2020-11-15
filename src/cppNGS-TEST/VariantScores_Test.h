#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "VariantScores.h"

TEST_CLASS(VariantScores_Test)
{
Q_OBJECT
private slots:

	void algorithms()
	{
		VariantScores ranker;
		IS_TRUE(ranker.algorithms().count()>=1);
	}

	void description()
	{
		VariantScores ranker;
		foreach(QString algorithm, ranker.algorithms())
		{
			IS_FALSE(ranker.description(algorithm).isEmpty());
		}
	}

	void rank_GSvar_v1()
	{
		//construct phenotye ROI
		BedFile roi;
		roi.load(TESTDATA("data_in/VariantScores_HP0003002.bed"));
		Phenotype pheno("HP0003002", "Breast carcinoma");
		QHash<Phenotype, BedFile> pheno_rois;
		pheno_rois[pheno] = roi;

		//load variants
		VariantList variants;
		variants.load(TESTDATA("data_in/VariantScores_in1.GSvar"));

		//rank
		VariantScores ranker;
		VariantScores::Result result = ranker.score("GSvar_v1", variants, pheno_rois);
		I_EQUAL(variants.count(), result.scores.count());
		I_EQUAL(result.warnings.count(), 0);
		for(int i=0; i<variants.count(); ++i)
		{
			QString var_str = variants[i].toString();
			if (var_str=="chr9:116958287-116958287 C>T") F_EQUAL(result.scores[i], 6.5);
			if (var_str=="chr16:3639230-3639230 G>A") F_EQUAL(result.scores[i], 6.0);
			if (var_str=="chr11:6638385-6638385 C>T") F_EQUAL(result.scores[i], 6.0);
			if (var_str=="chr2:178740622-178740622 A>C") F_EQUAL(result.scores[i], 6.5);
			if (var_str=="chr2:29455199-29455199 A>T") F_EQUAL(result.scores[i], 6.0);
			if (var_str=="chr5:131925483-131925483 G>C") F_EQUAL(result.scores[i], 5.5);
			if (var_str=="chr14:106330070-106330070 G>A") F_EQUAL(result.scores[i], 1.0);
			if (var_str=="chr7:100806448-100806448 C>G") F_EQUAL(result.scores[i], 1.0);
			if (var_str=="chr2:234737380-234737380 G>T") F_EQUAL(result.scores[i], 2.0);
			if (var_str=="chr1:866511-866511 ->CCCT") F_EQUAL(result.scores[i], 1.0);
		}
	}

	void rank_GSvar_v1_noNGSD()
	{
		//construct phenotye ROI
		BedFile roi;
		roi.load(TESTDATA("data_in/VariantScores_HP0003002.bed"));
		Phenotype pheno("HP0003002", "Breast carcinoma");
		QHash<Phenotype, BedFile> pheno_rois;
		pheno_rois[pheno] = roi;

		//load variants
		VariantList variants;
		variants.load(TESTDATA("data_in/VariantScores_in1.GSvar"));

		//rank
		VariantScores ranker;
		VariantScores::Result result = ranker.score("GSvar_v1_noNGSD", variants, pheno_rois);
		I_EQUAL(variants.count(), result.scores.count());
		I_EQUAL(result.warnings.count(), 0);
		for(int i=0; i<variants.count(); ++i)
		{
			QString var_str = variants[i].toString();
			if (var_str=="chr9:116958287-116958287 C>T") F_EQUAL(result.scores[i], 6.5);
			if (var_str=="chr16:3639230-3639230 G>A") F_EQUAL(result.scores[i], 6.0);
			if (var_str=="chr11:6638385-6638385 C>T") F_EQUAL(result.scores[i], 6.0);
			if (var_str=="chr2:178740622-178740622 A>C") F_EQUAL(result.scores[i], 6.0); //no class 4 => -0.5 score
			if (var_str=="chr2:29455199-29455199 A>T") F_EQUAL(result.scores[i], 6.0);
			if (var_str=="chr5:131925483-131925483 G>C") F_EQUAL(result.scores[i], 5.5);
			if (var_str=="chr14:106330070-106330070 G>A") F_EQUAL(result.scores[i], 1.0);
			if (var_str=="chr7:100806448-100806448 C>G") F_EQUAL(result.scores[i], 1.0);
			if (var_str=="chr2:234737380-234737380 G>T") F_EQUAL(result.scores[i], 1.0); //no class 5 => -1 score
			if (var_str=="chr1:866511-866511 ->CCCT") F_EQUAL(result.scores[i], 0.0); //filtered out
		}
	}
};
