#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "VariantScores.h"

TEST_CLASS(VariantScores_Test)
{
Q_OBJECT
private slots:

	void algorithms()
	{
		IS_TRUE(VariantScores::algorithms().count()>=1);
	}

	void description()
	{
		foreach(QString algorithm, VariantScores::algorithms())
		{
			IS_FALSE(VariantScores::description(algorithm).isEmpty());
		}
	}

	void rank_GSvar_v1()
	{
		//construct phenotype ROI
		BedFile roi;
		roi.load(TESTDATA("data_in/VariantScores_HP0003002.bed"));
		Phenotype pheno("HP0003002", "Breast carcinoma");
		QHash<Phenotype, BedFile> pheno_rois;
		pheno_rois[pheno] = roi;

		//load variants
		VariantList variants;
		variants.load(TESTDATA("data_in/VariantScores_in1.GSvar"));

		//rank
		VariantScores::Parameters parameters;
		VariantScores::Result result = VariantScores::score("GSvar_v1", variants, pheno_rois, parameters);
		S_EQUAL(result.algorithm, "GSvar_v1");
		I_EQUAL(variants.count(), result.scores.count());
		I_EQUAL(variants.count(), result.score_explainations.count());
		I_EQUAL(variants.count(), result.ranks.count());
		I_EQUAL(result.warnings.count(), 0);
		for(int i=0; i<variants.count(); ++i)
		{
			QString var_str = variants[i].toString();
			if (var_str=="chr2:178740622-178740622 A>C")
			{
				F_EQUAL(result.scores[i], 6.5);
				I_EQUAL(result.ranks[i], 1);
			}
			if (var_str=="chr9:116958287-116958287 C>T")
			{
				F_EQUAL(result.scores[i], 6.5);
				I_EQUAL(result.ranks[i], 2);
			}
			if (var_str=="chr2:29455199-29455199 A>T")
			{
				F_EQUAL(result.scores[i], 6.0);
				I_EQUAL(result.ranks[i], 3);
			}
			if (var_str=="chr11:6638385-6638385 C>T")
			{
				F_EQUAL(result.scores[i], 6.0);
				I_EQUAL(result.ranks[i], 4);
			}
			if (var_str=="chr16:3639230-3639230 G>A")
			{
				F_EQUAL(result.scores[i], 6.0);
				I_EQUAL(result.ranks[i], 5);
			}
			if (var_str=="chr5:131925483-131925483 G>C")
			{
				F_EQUAL(result.scores[i], 5.5);
				I_EQUAL(result.ranks[i], 6);
			}
			if (var_str=="chr2:234737380-234737380 G>T")
			{
				F_EQUAL(result.scores[i], 2.0);
			}
			if (var_str=="chr14:106330070-106330070 G>A")
			{
				 F_EQUAL(result.scores[i], 1.0);
			}
			if (var_str=="chr7:100806448-100806448 C>G")
			{
				F_EQUAL(result.scores[i], 1.0);
			}
			if (var_str=="chr1:866511-866511 ->CCCT")
			{
				F_EQUAL(result.scores[i], -1.0);
			}
		}

		//check that score explainations sum matches score
		for(int i=0; i<result.scores.count(); ++i)
		{
			if (result.scores[i]>=0)
			{
				double score_sum = 0.0;
				QStringList explainations = result.score_explainations[i];
				foreach(QString explaination, explainations)
				{
					QStringList tmp = (explaination+":").split(":");
					score_sum += Helper::toDouble(tmp[1], explaination, variants[i].toString());
				}

				F_EQUAL(score_sum, result.scores[i]);
			}
		}
	}

	void rank_GSvar_v1_noNGSD()
	{
		//construct phenotype ROI
		BedFile roi;
		roi.load(TESTDATA("data_in/VariantScores_HP0003002.bed"));
		Phenotype pheno("HP0003002", "Breast carcinoma");
		QHash<Phenotype, BedFile> pheno_rois;
		pheno_rois[pheno] = roi;

		//load variants
		VariantList variants;
		variants.load(TESTDATA("data_in/VariantScores_in1.GSvar"));

		//rank
		VariantScores::Parameters parameters;
		parameters.use_ngsd_classifications = false;
		VariantScores::Result result = VariantScores::score("GSvar_v1", variants, pheno_rois, parameters);
		I_EQUAL(variants.count(), result.scores.count());
		I_EQUAL(variants.count(), result.score_explainations.count());
		I_EQUAL(variants.count(), result.ranks.count());
		I_EQUAL(result.warnings.count(), 0);
		for(int i=0; i<variants.count(); ++i)
		{
			QString var_str = variants[i].toString();


			if (var_str=="chr9:116958287-116958287 C>T")
			{
				F_EQUAL(result.scores[i], 6.5);
				I_EQUAL(result.ranks[i], 1);
			}
			if (var_str=="chr2:29455199-29455199 A>T")
			{
				F_EQUAL(result.scores[i], 6.0);
				I_EQUAL(result.ranks[i], 2);
			}
			if (var_str=="chr2:178740622-178740622 A>C") //no class 4 => -0.5 score
			{
				F_EQUAL(result.scores[i], 6.0);
				I_EQUAL(result.ranks[i], 3);
			}
			if (var_str=="chr11:6638385-6638385 C>T")
			{
				F_EQUAL(result.scores[i], 6.0);
				I_EQUAL(result.ranks[i], 4);
			}
			if (var_str=="chr16:3639230-3639230 G>A")
			{
				F_EQUAL(result.scores[i], 6.0);
				I_EQUAL(result.ranks[i], 5);
			}
			if (var_str=="chr5:131925483-131925483 G>C")
			{
				F_EQUAL(result.scores[i], 5.5);
				I_EQUAL(result.ranks[i], 6);
			}
			if (var_str=="chr2:234737380-234737380 G>T") //no class 5 => -1 score
			{
				F_EQUAL(result.scores[i], 1.0);
			}
			if (var_str=="chr14:106330070-106330070 G>A")
			{
				 F_EQUAL(result.scores[i], 1.0);
			}
			if (var_str=="chr7:100806448-100806448 C>G")
			{
				F_EQUAL(result.scores[i], 1.0);
			}
			if (var_str=="chr1:866511-866511 ->CCCT") //filtered out
			{
				F_EQUAL(result.scores[i], -1.0);
				I_EQUAL(result.ranks[i], -1);
			}
		}

		//check that score explainations sum matches score
		for(int i=0; i<result.scores.count(); ++i)
		{
			if (result.scores[i]>=0)
			{
				double score_sum = 0.0;
				QStringList explainations = result.score_explainations[i];
				foreach(QString explaination, explainations)
				{
					QStringList tmp = (explaination+":").split(":");
					score_sum += Helper::toDouble(tmp[1], explaination, variants[i].toString());
				}

				F_EQUAL(score_sum, result.scores[i]);
			}
		}
	}
};
