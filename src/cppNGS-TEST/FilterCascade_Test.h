#include "TestFramework.h"
#include "FilterCascade.h"
#include "Settings.h"

TEST_CLASS(FilterCascade_Test)
{
Q_OBJECT
private slots:

	/********************************************* Filter factory *********************************************/

	void FilterFactory_filterNames()
	{
		//all
		QStringList names = FilterFactory::filterNames();
		IS_TRUE(names.contains("Allele frequency"));
		IS_TRUE(names.contains("CNV size"));
		int count_all = names.count();

		//small variants
		names = FilterFactory::filterNames(VariantType::SNVS_INDELS);
		IS_TRUE(names.contains("Allele frequency"));
		IS_FALSE(names.contains("CNV size"));
		IS_TRUE(names.count()<count_all);

		//CNVs
		names = FilterFactory::filterNames(VariantType::CNVS);
		IS_FALSE(names.contains("Allele frequency"));
		IS_TRUE(names.contains("CNV size"));
		IS_TRUE(names.count()<count_all);
	}

	/********************************************* Filters for small variants *********************************************/

	void FilterAlleleFrequency_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());
		FilterAlleleFrequency filter;
		filter.setDouble("max_af", 1.0);
		filter.apply(vl, result);

		I_EQUAL(result.countPassing(), 22);
		IS_TRUE(result.flags()[70]);
		IS_TRUE(result.flags()[74]);
		IS_TRUE(result.flags()[101]);
		IS_TRUE(result.flags()[120]);
	}

	void FilterSubpopulationAlleleFrequency_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());
		FilterSubpopulationAlleleFrequency filter;
		filter.setDouble("max_af", 1.0);
		filter.apply(vl, result);

		I_EQUAL(result.countPassing(), 9);
	}


	void FilterRegions_apply_specialCaseSingleRegion()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));
		I_EQUAL(vl.annotationIndexByName("filter"), 1);

		FilterResult result(vl.count());
		FilterRegions::apply(vl, BedFile("chr1", 27687465, 27687467), result);

		I_EQUAL(result.countPassing(), 1);
		IS_TRUE(result.flags()[0]);

		result.tagNonPassing(vl, "off-target", "Variants outside target region");
		I_EQUAL(vl.annotationIndexByName("filter"), 1);
		int filter_count = 0;
		for (int i=0; i<vl.count(); ++i)
		{
			if (vl[i].filters().contains("off-target"))
			{
				++filter_count;
			}
		}
		I_EQUAL(filter_count, 142);
	}

	void FilterRegions_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());
		BedFile regions;
		regions.append(BedLine("chr1", 62728830, 62728870));
		regions.append(BedLine("chr17", 41244430, 41245240));
		FilterRegions::apply(vl, regions, result);

		I_EQUAL(result.countPassing(), 5);
		IS_TRUE(result.flags()[5]);
		IS_TRUE(result.flags()[6]);
		IS_TRUE(result.flags()[134]);
		IS_TRUE(result.flags()[135]);
		IS_TRUE(result.flags()[136]);

		result.removeFlagged(vl);
		I_EQUAL(vl.count(), 5);
	}

	void FilterFilterColumnEmpty_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());
		FilterFilterColumnEmpty filter;
		filter.apply(vl, result);

		I_EQUAL(result.countPassing(), 109);
	}

	void FilterVariantIsSNP_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());
		FilterVariantIsSNP filter;
		filter.apply(vl, result);

		I_EQUAL(result.countPassing(), 135);
	}

	void FilterGenes_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//filter one gene
		FilterGenes filter;
		filter.setStringList("genes", QStringList() << "TP53");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 1);

		//filter two genes
		result.reset();
		filter.setStringList("genes", QStringList() << "TP53" << "BRCA1");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 8);

		//filter one gene (wildcard)
		result.reset();
		filter.setStringList("genes", QStringList() << "BRCA*");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 12);

		//filter one gene (wildcard with minus char)
		result.reset();
		filter.setStringList("genes", QStringList() << "*-*");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 8);

		//filter two genes (wildcard)
		result.reset();
		filter.setStringList("genes", QStringList() << "BRCA*" << "TP*");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 19);
	}

	void FilterVariantImpact_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//with one impact string
		FilterVariantImpact filter;
		filter.setStringList("impact", QStringList() << "HIGH");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);
		IS_TRUE(result.flags()[19]);
		IS_TRUE(result.flags()[47]);

		//with two impact strings
		result.reset();
		filter.setStringList("impact", QStringList() << "HIGH" << "MODERATE");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 60);
	}

	void FilterVariantCountNGSD_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//count 5
		FilterVariantCountNGSD filter;
		filter.setInteger("max_count", 5);
		filter.setBool("ignore_genotype", false);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 3);
		IS_TRUE(result.flags()[70]);
		IS_TRUE(result.flags()[92]);
		IS_TRUE(result.flags()[120]);

		//count 50
		result.reset();
		filter.setInteger("max_count", 50);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 9);

		//count 50 (ignore genotype)
		result.reset();
		filter.setBool("ignore_genotype", true);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 8);
	}

	void FilterClassificationNGSD_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//REMOVE
		FilterClassificationNGSD filter;
		filter.setString("action", "REMOVE");
		filter.setStringList("classes", QStringList() << "1" << "2");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 106);

		//FILTER
		result.reset(true);
		filter.setString("action", "FILTER");
		filter.setStringList("classes", QStringList() << "3" << "4" << "5");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 1);

		//KEEP
		result.reset(false);
		filter.setString("action", "KEEP");
		filter.setStringList("classes", QStringList() << "2" << "3");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 6);

	}

	void FilterFilterColumn_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//REMOVE
		FilterFilterColumn filter;
		filter.setString("action", "REMOVE");
		filter.setStringList("entries", QStringList() << "low_MQM");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 132);

		result.reset();
		filter.setString("action", "REMOVE");
		filter.setStringList("entries", QStringList() << "low_MQM" << "low_DP");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 125);

		//KEEP
		result.reset(false);
		filter.setString("action", "KEEP");
		filter.setStringList("entries", QStringList() << "low_DP");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 7);

		//FILTER
		result.reset();
		filter.setString("action", "FILTER");
		filter.setStringList("entries", QStringList() << "low_MQM");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 11);
	}

	void FilterGeneConstraint_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//gnomAD o/e
		FilterGeneConstraint filter;
		filter.setDouble("min_pli", 1.0);
		filter.setDouble("max_oe_lof", 0.1);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 1);

		//ExAC pLI (deprecated, for backward-compatibility)
		result.reset(true);
		filter.setDouble("max_oe_lof", 0.0);
		filter.setDouble("min_pli", 0.0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 143);

		filter.setDouble("min_pli", 0.5);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 12);

		filter.setDouble("min_pli", 0.95);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 4);

		filter.setDouble("min_pli", 1.0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 0);
	}

	void FilterGeneInheritance_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		FilterGeneInheritance filter;
		filter.setStringList("modes", QStringList() << "AD");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 19);

		result.reset();
		filter.setStringList("modes", QStringList() << "AR");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 28);

		result.reset();
		filter.setStringList("modes", QStringList() << "AD" << "AR");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 44);
	}

	void FilterColumnMatchRegexp_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//REMOVE
		FilterColumnMatchRegexp filter;
		filter.setString("action", "REMOVE");
		filter.setString("column", "OMIM");
		filter.setString("pattern", "^$");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 108);

		//KEEP
		result.reset(false);
		filter.setString("action", "KEEP");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 35);

		//FILTER
		result.reset();
		filter.setString("action", "FILTER");
		filter.setString("column", "dbSNP");
		filter.setString("pattern", "^$");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 3);

	}

	void FilterGenotypeControl_apply_multiSample()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in_multi.GSvar"));

		FilterResult result(vl.count());

		//hom
		FilterGenotypeControl filter;
		filter.setStringList("genotypes", QStringList() << "hom");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 103);

		//het
		result.reset();
		filter.setStringList("genotypes", QStringList() << "het");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 53);

		//wt/het (not hom)
		result.reset();
		filter.setStringList("genotypes", QStringList() << "wt" << "het");
		filter.setBool("same_genotype", false);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 153);

		//wt/het (not hom - same genotype)
		result.reset();
		filter.setStringList("genotypes", QStringList() << "wt" << "het");
		filter.setBool("same_genotype", true);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 152);
	}

	void FilterGenotypeAffected_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//hom
		FilterGenotypeAffected filter;
		filter.setStringList("genotypes", QStringList() << "hom");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 57);

		//het
		result.reset();
		filter.setStringList("genotypes", QStringList() << "het");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 86);
	}

	void FilterGenotypeAffected_apply_multiSample()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in_multi.GSvar"));

		FilterResult result(vl.count());

		//hom
		FilterGenotypeAffected filter;
		filter.setStringList("genotypes", QStringList() << "hom");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 65);

		//het
		result.reset();
		filter.setStringList("genotypes", QStringList() << "het");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 95);

		//het
		result.reset();
		filter.setStringList("genotypes", QStringList() << "wt");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 14);

		//wt/het (not hom)
		result.reset();
		filter.setStringList("genotypes", QStringList() << "wt" << "het");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 109);
	}


	void FilterGenotypeAffected_apply_comphet()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//chr1 and high/moderate impact
		FilterRegions::apply(vl, BedFile("chr1", 1, 248956422), result);
		FilterVariantImpact filter2;
		filter2.setStringList("impact", QStringList() << "HIGH" << "MODERATE");
		filter2.apply(vl, result);
		I_EQUAL(result.countPassing(), 9);

		FilterGenotypeAffected filter3;
		filter3.setStringList("genotypes", QStringList() << "comp-het");
		filter3.apply(vl, result);
		I_EQUAL(result.countPassing(), 3);
	}

	void FilterGenotypeAffected_apply_comphet_hom()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//chr1 and high/moderate impact
		FilterRegions::apply(vl, BedFile("chr1", 1, 248956422), result);
		FilterVariantImpact filter2;
		filter2.setStringList("impact", QStringList() << "HIGH" << "MODERATE");
		filter2.apply(vl, result);
		I_EQUAL(result.countPassing(), 9);

		FilterGenotypeAffected filter3;
		filter3.setStringList("genotypes", QStringList() << "comp-het" << "hom");
		filter3.apply(vl, result);
		I_EQUAL(result.countPassing(), 6);
	}

	void FilterGenotypeAffected_apply_multiSample_comphet()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in_multi.GSvar"));

		FilterResult result(vl.count());

		//low af variants
		FilterAlleleFrequency filter1;
		filter1.setDouble("max_af", 1.0);
		filter1.apply(vl, result);
		I_EQUAL(result.countPassing(), 39);

		FilterGenotypeAffected filter2;
		filter2.setStringList("genotypes", QStringList() << "comp-het");
		filter2.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);
	}

	void FilterGenotypeAffected_apply_multiSample_comphet_hom()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in_multi.GSvar"));

		FilterResult result(vl.count());

		//low af variants
		FilterAlleleFrequency filter1;
		filter1.setDouble("max_af", 1.0);
		filter1.apply(vl, result);
		I_EQUAL(result.countPassing(), 39);

		FilterGenotypeAffected filter2;
		filter2.setStringList("genotypes", QStringList() << "comp-het" << "hom");
		filter2.apply(vl, result);
		I_EQUAL(result.countPassing(), 3);
	}

	void FilterAnnotationPathogenic_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//FILTER
		FilterAnnotationPathogenic filter;
		filter.setStringList("sources", QStringList() << "HGMD" << "ClinVar");
		filter.setBool("also_likely_pathogenic", true);
		filter.setString("action", "FILTER");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 5);

		result.reset();
		filter.setStringList("sources", QStringList() << "HGMD" << "ClinVar");
		filter.setBool("also_likely_pathogenic", false);
		filter.setString("action", "FILTER");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);

		//KEEP
		result.reset(false);
		filter.setStringList("sources", QStringList() << "HGMD" << "ClinVar");
		filter.setBool("also_likely_pathogenic", true);
		filter.setString("action", "KEEP");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 5);

		result.reset(false);
		filter.setStringList("sources", QStringList() << "HGMD" << "ClinVar");
		filter.setBool("also_likely_pathogenic", false);
		filter.setString("action", "KEEP");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);

		result.reset(false);
		filter.setStringList("sources", QStringList() << "ClinVar");
		filter.setBool("also_likely_pathogenic", true);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 1);

		result.reset(false);
		filter.setStringList("sources", QStringList() << "ClinVar");
		filter.setBool("also_likely_pathogenic", false);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 1);

		result.reset(false);
		filter.setStringList("sources", QStringList() << "HGMD");
		filter.setBool("also_likely_pathogenic", true);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 4);

		result.reset(false);
		filter.setStringList("sources", QStringList() << "HGMD");
		filter.setBool("also_likely_pathogenic", false);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 1);
	}


	void FilterPredictionPathogenic_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//FILTER
		FilterPredictionPathogenic filter;
		filter.setString("action", "FILTER");
		filter.setInteger("min", 1);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 24);

		filter.setInteger("min", 2);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 6);

		filter.setInteger("min", 3);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 1);

		filter.setInteger("min", 4);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 0);

		//KEEP
		result.reset(false);
		filter.setString("action", "KEEP");
		filter.setInteger("min", 2);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 6);
	}

	void FilterAnnotationText_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//FILTER
		FilterAnnotationText filter;
		filter.setString("action", "FILTER");
		filter.setString("term", "bReast cancer");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 26);

		//KEEP
		result.reset(false);
		filter.setString("action", "KEEP");
		filter.setString("term", "ataXIA");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 4);

		//REMOVE
		result.reset();
		filter.setString("action", "REMOVE");
		filter.setString("term", "ataXIA");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 139);
	}

	void FilterVariantType_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default (HIGH:all MODERATE:all LOW:splice_region)
		FilterVariantType filter;
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 77);
	}

	void FilterVariantQC_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//qual
		FilterVariantQC filter;
		filter.setInteger("qual", 200);
		filter.setInteger("depth", 0);
		filter.setInteger("mapq", 0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 138);

		//depth
		result.reset();
		filter.setInteger("qual", 0);
		filter.setInteger("depth", 20);
		filter.setInteger("mapq", 0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 136);

		//mapq
		result.reset();
		filter.setInteger("qual", 0);
		filter.setInteger("depth", 0);
		filter.setInteger("mapq", 55);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 131);

		//combined
		result.reset();
		filter.setInteger("qual", 500);
		filter.setInteger("depth", 20);
		filter.setInteger("mapq", 55);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 115);
	}

	void FilterVariantQC_apply_multiSample()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in_multi.GSvar"));

		FilterResult result(vl.count());

		FilterVariantQC filter;
		filter.setInteger("qual", 20);
		filter.setInteger("depth", 20);
		filter.setInteger("mapq", 60);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 125);
	}

	void FilterTrio_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in_trio.GSvar")); //pre-filtered: AF:1%, AF(sub):1%, NGSD count:20, QUAL:30, DP:10, MQM:30

		FilterResult result(vl.count());

		//default (all filters)
		FilterTrio filter;
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 13);

		//de-novo
		result.reset();
		filter.setStringList("types", QStringList() << "de-novo");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);

		//recessive
		result.reset();
		filter.setStringList("types", QStringList() << "recessive");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 1);

		//comp-het
		result.reset();
		filter.setStringList("types", QStringList() << "comp-het");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);

		//comp-het
		result.reset();
		filter.setStringList("types", QStringList() << "LOH");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);

		//x-linked
		result.reset();
		filter.setStringList("types", QStringList() << "x-linked");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 6);

		//imprinting
		result.reset();
		filter.setStringList("types", QStringList() << "imprinting");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);
	}

	void FilterOMIM_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterOMIM filter;
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 108);
	}

	void FilterConservedness_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterConservedness filter;
		filter.setDouble("min_score", 2.0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 16);
	}

	void FilterRegulatory_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterRegulatory filter;
		filter.setString("action", "FILTER");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);
	}

	/********************************************* Filters for CNVs *********************************************/

	void FilterCnvSize_apply()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvSize filter;
		filter.setDouble("size", 20.0);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 2);
	}


	void FilterCnvRegions_apply()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvRegions filter;
		filter.setInteger("regions", 4);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 1);
	}

	void FilterCnvCopyNumber_apply_ClinCNV_germline()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvCopyNumber filter;
		filter.setString("cn", "1");
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 37);
	}

	void FilterCnvCopyNumber_apply_CnvHunter_germline()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_CnvHunter_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvCopyNumber filter;
		filter.setString("cn", "4+");
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 1);
	}

	void FilterCnvAlleleFrequency_apply_ClinCNV_germline()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvAlleleFrequency filter;
		filter.setDouble("max_af", 0.02);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 6);
	}

	void FilterCnvAlleleFrequency_apply_CnvHunter_germline()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_CnvHunter_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvAlleleFrequency filter;
		filter.setDouble("max_af", 0.02);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 7);
	}

	void FilterCnvZscore_apply()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_CnvHunter_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvZscore filter;
		filter.setDouble("min_z", 5.5);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 7);
	}


	void FilterCnvLoglikelihood_apply()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvLoglikelihood filter;
		filter.setDouble("min_ll", 11.0);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 2);
	}

	void FilterCnvLoglikelihood_apply_multi()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline_multi.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvLoglikelihood filter;
		filter.setDouble("min_ll", 200.0);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 7);
	}


	void FilterCnvQvalue_apply()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvQvalue filter;
		filter.setDouble("max_q", 0.0001);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 2);
	}

	void FilterCnvQvalue_apply_multi()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline_multi.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvQvalue filter;
		filter.setDouble("max_q", 0.0);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 20);
	}

	void FilterCnvCompHet_apply_cnv_cnv()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_CnvHunter_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvCompHet filter;
		filter.setString("mode", "CNV-CNV");
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 19);
	}

	void FilterCnvCompHet_apply_cnv_snvindel()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvCompHet filter;
		filter.setString("mode", "CNV-SNV/INDEL");
		filter.setHetHitGenes(GeneSet() << "SKI" << "PER3" << "BRCA1" << "BRCA2" << "TP53");
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 2);
	}

	void FilterCnvOMIM_apply()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvOMIM filter;
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 20);
	}


	void FilterCnvCnpOverlap_apply()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvCnpOverlap filter;
		filter.setDouble("max_ol", 0.001);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 65);
	}
	/********************************************* Default filters for CNVs *********************************************/

	void default_filters_ClinCNV_germline_single()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		FilterCascade filters = FilterCascadeFile::load(TESTDATA("data_in/CnvList_filters.ini"), "default filter (ClinCNV)");
		FilterResult result = filters.apply(cnvs, true);
		I_EQUAL(result.countPassing(), 0);
	}

	void default_filters_ClinCNV_germline_multi()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline_multi.tsv"));

		FilterCascade filters = FilterCascadeFile::load(TESTDATA("data_in/CnvList_filters.ini"), "default filter (ClinCNV)");
		FilterResult result = filters.apply(cnvs, true);
		I_EQUAL(result.countPassing(), 32);
	}

	void default_filters_ClinCNV_somatic()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_somatic.tsv"));

		FilterCascade filters = FilterCascadeFile::load(TESTDATA("data_in/CnvList_filters.ini"), "default filter (ClinCNV)");
		FilterResult result = filters.apply(cnvs, false);
		I_EQUAL(result.countPassing(), 31);
	}

	void default_filters_CnvHunter_germline_single()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_CnvHunter_germline.tsv"));

		FilterCascade filters = FilterCascadeFile::load(TESTDATA("data_in/CnvList_filters.ini"), "default filter (CnvHunter)");
		FilterResult result = filters.apply(cnvs, true);
		I_EQUAL(result.countPassing(), 28);
	}

	void default_filters_CnvHunter_germline_multi()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_CnvHunter_germline_multi.tsv"));

		FilterCascade filters = FilterCascadeFile::load(TESTDATA("data_in/CnvList_filters.ini"), "default filter (CnvHunter)");
		FilterResult result = filters.apply(cnvs, true);
		I_EQUAL(result.countPassing(), 28);
	}


};
