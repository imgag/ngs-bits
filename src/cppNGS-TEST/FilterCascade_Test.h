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

	void FilterAlleleFrequency_apply_without_1000g()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_no1000g_in.GSvar"));

		FilterResult result(vl.count());
		FilterAlleleFrequency filter;
		filter.setDouble("max_af", 1.0);
		filter.apply(vl, result);

		I_EQUAL(result.countPassing(), 29);
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
		FilterVariantIsSNV filter;
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
		filter.setBool("same_genotype", true);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 65);

		//het
		result.reset();
		filter.setStringList("genotypes", QStringList() << "het");
		filter.setBool("same_genotype", true);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 95);

		//het
		result.reset();
		filter.setStringList("genotypes", QStringList() << "wt");
		filter.setBool("same_genotype", true);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 14);

		//wt/het (not hom)
		result.reset();
		filter.setStringList("genotypes", QStringList() << "wt" << "het");
		filter.setBool("same_genotype", true);
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

	void FilterGenotypeAffected_apply_comphet_phased()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in_phased.GSvar"));

		FilterResult result(vl.count());

		//old method
		FilterGenotypeAffected filter1;
		filter1.setStringList("genotypes", QStringList() << "comp-het");
		filter1.apply(vl, result);
		I_EQUAL(result.countPassing(), 187);
		result.reset();

		//only phased
		FilterGenotypeAffected filter2;
		filter2.setStringList("genotypes", QStringList() << "comp-het (phased)");
		filter2.apply(vl, result);
		I_EQUAL(result.countPassing(), 68);
		result.reset();

		//only unphased
		FilterGenotypeAffected filter3;
		filter3.setStringList("genotypes", QStringList() << "comp-het (unphased)");
		filter3.apply(vl, result);
		I_EQUAL(result.countPassing(), 89);
		result.reset();

		//invalid input
		FilterGenotypeAffected filter4;
		filter4.setStringList("genotypes", QStringList() << "comp-het" << "comp-het (phased)");
		IS_THROWN(ArgumentException, filter4.apply(vl, result));
		filter4.setStringList("genotypes", QStringList() << "comp-het" << "comp-het (unphased)");
		IS_THROWN(ArgumentException, filter4.apply(vl, result));
		filter4.setStringList("genotypes", QStringList() << "comp-het (phased)" << "comp-het (unphased)");
		IS_THROWN(ArgumentException, filter4.apply(vl, result));
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
		I_EQUAL(result.countPassing(), 17);

		filter.setInteger("min", 2);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 0);

		//KEEP
		result.reset(false);
		filter.setString("action", "KEEP");
		filter.setInteger("min", 1);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 17);
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
		filter.setInteger("strand_bias", -1);
		filter.setInteger("allele_balance", -1);
		filter.setInteger("min_occurences", 0);
		filter.setDouble("min_af", 0);
		filter.setDouble("max_af", 1);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 138);

		//depth
		result.reset();
		filter.setInteger("qual", 0);
		filter.setInteger("depth", 20);
		filter.setInteger("mapq", 0);
		filter.setInteger("strand_bias", -1);
		filter.setInteger("allele_balance", -1);
		filter.setInteger("min_occurences", 0);
		filter.setDouble("min_af", 0);
		filter.setDouble("max_af", 1);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 136);

		//mapq
		result.reset();
		filter.setInteger("qual", 0);
		filter.setInteger("depth", 0);
		filter.setInteger("mapq", 55);
		filter.setInteger("strand_bias", -1);
		filter.setInteger("allele_balance", -1);
		filter.setInteger("min_occurences", 0);
		filter.setDouble("min_af", 0);
		filter.setDouble("max_af", 1);

		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 131);

		//strand bias
		result.reset();
		filter.setInteger("qual", 0);
		filter.setInteger("depth", 0);
		filter.setInteger("mapq", 0);
		filter.setInteger("strand_bias", 20);
		filter.setInteger("allele_balance", -1);
		filter.setInteger("min_occurences", 0);
		filter.setDouble("min_af", 0);
		filter.setDouble("max_af", 1);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 142);

		//allele bias
		result.reset();
		filter.setInteger("qual", 0);
		filter.setInteger("depth", 0);
		filter.setInteger("mapq", 0);
		filter.setInteger("strand_bias", -1);
		filter.setInteger("allele_balance", 20);
		filter.setInteger("min_occurences", 0);
		filter.setDouble("min_af", 0);
		filter.setDouble("max_af", 1);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 142);

		//combined
		result.reset();
		filter.setInteger("qual", 500);
		filter.setInteger("depth", 20);
		filter.setInteger("mapq", 55);
		filter.setInteger("strand_bias", 20);
		filter.setInteger("allele_balance", 20);
		filter.setDouble("min_af", 0);
		filter.setDouble("max_af", 1);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 113);

		// new test File
		vl.clear();
		vl.load(TESTDATA("data_in/VariantFilter_in1.GSvar"));
		result = FilterResult(vl.count());

		//min occurences per strand
		result.reset();
		filter.setInteger("qual", 0);
		filter.setInteger("depth", 0);
		filter.setInteger("mapq", 0);
		filter.setInteger("strand_bias", -1);
		filter.setInteger("allele_balance", -1);
		filter.setInteger("min_occurences", 3);
		filter.setDouble("min_af", 0);
		filter.setDouble("max_af", 1);

		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 6);


		//Allele frequency
		result.reset();
		filter.setInteger("qual", 0);
		filter.setInteger("depth", 0);
		filter.setInteger("mapq", 0);
		filter.setInteger("strand_bias", -1);
		filter.setInteger("allele_balance", -1);
		filter.setInteger("min_occurences", 0);
		filter.setDouble("min_af", 0.02);
		filter.setDouble("max_af", 0.1);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 6);
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
		I_EQUAL(result.countPassing(), 17);

		//de-novo
		result.reset();
		filter.setStringList("types", QStringList() << "de-novo");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);

		//recessive
		result.reset();
		filter.setStringList("types", QStringList() << "recessive");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);

		//comp-het
		result.reset();
		filter.setStringList("types", QStringList() << "comp-het");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 4);

		//LOH
		result.reset();
		filter.setStringList("types", QStringList() << "LOH");
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 3);

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

	void FilterSplice_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in_newer_Annotation_in.GSvar"));

		FilterResult result(vl.count());


		FilterSpliceEffect filter;
		// MODUS FILTER:
		filter.setString("action", "FILTER");

		// all filters off
		filter.setString("MaxEntScan", "LOW");
		filter.setDouble("SpliceAi", 0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), result.flags().count());

		//only MES MODERATE
		result.reset();
		filter.setString("MaxEntScan", "MODERATE");
		filter.setDouble("SpliceAi", 0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);

		//only MES HIGH
		result.reset();
		filter.setString("MaxEntScan", "HIGH");
		filter.setDouble("SpliceAi", 0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 1);

		//only SpliceAi
		result.reset();
		filter.setString("MaxEntScan", "LOW");
		filter.setDouble("SpliceAi", 0.2);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 5);

		//combined
		result.reset();
		filter.setString("MaxEntScan", "MODERATE");
		filter.setDouble("SpliceAi", 0.4);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 3);

		// MODUS KEEP combined
		filter.setString("action", "KEEP");
		result.reset(false);
		filter.setString("MaxEntScan", "MODERATE");
		filter.setDouble("SpliceAi", 0.4);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 3);

		//only MES: increase
		result.reset(false);
		filter.setString("MaxEntScan", "MODERATE");
		filter.setDouble("SpliceAi", 0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 2);

		//only SpliceAi
		result.reset(false);
		filter.setString("MaxEntScan", "LOW");
		filter.setDouble("SpliceAi", 0.2);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 5);

		// all filters off:
		result.reset(false);
		filter.setString("MaxEntScan", "LOW");;
		filter.setDouble("SpliceAi", 0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 0);
	}

	void FilterVariantRNAAseAlleleFrequency_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterVariantRNAAseAlleleFrequency filter;
		filter.setDouble("min_af", 0.33);
		filter.setDouble("max_af", 0.66);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 41);
	}

	void FilterVariantRNAAseAlt_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterVariantRNAAseAlt filter;
		filter.setInteger("min_ac", 17);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 91);
	}

	void FilterVariantRNAAseDepth_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterVariantRNAAseDepth filter;
		filter.setInteger("min_depth", 26);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 110);
	}

	void FilterVariantRNAAsePval_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterVariantRNAAsePval filter;
		filter.setDouble("max_pval", 0.2);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 13);
	}

	void FilterVariantRNAAberrantSplicing_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterVariantRNAAberrantSplicing filter;
		filter.setDouble("min_asf", 0.05);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 17);
	}

	void FilterVariantRNAExpressionFC_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterVariantRNAExpressionFC filter;
		filter.setDouble("min_fc", 1.5);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 17);
	}

	void FilterVariantRNAExpressionZScore_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterVariantRNAExpressionZScore filter;
		filter.setDouble("min_zscore", 1.25);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 22);
	}

	void FilterVariantRNAGeneExpression_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterVariantRNAGeneExpression filter;
		filter.setDouble("min_tpm", 35);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 18);
	}

	void FilterVariantLrSrOverlap_apply()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterVariantLrSrOverlap filter;
		filter.setBool("invert", false);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 42);
	}


	/********************************************* Filters for small variants (somatic tumor-only) *********************************************/

	void FilterSomaticAlleleFrequency_apply_tumor_only()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in_somatic_tumor_only.GSvar"));

		FilterResult result(vl.count());

		//default
		FilterSomaticAlleleFrequency filter;
		filter.setDouble("min_af_tum", 50.0);
		filter.setDouble("max_af_nor", 100.0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 6);

		//error if normal cutoff is set
		filter.setDouble("max_af_nor", 5.0);
		IS_THROWN(ArgumentException, filter.apply(vl, result));
	}

	/********************************************* Filters for small variants (somatic tumor-only) *********************************************/

	void FilterSomaticAlleleFrequency_apply_tumor_normal()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in_somatic_tumor_normal.GSvar"));

		FilterResult result(vl.count());

		//tumor
		FilterSomaticAlleleFrequency filter;
		filter.setDouble("min_af_tum", 15.0);
		filter.setDouble("max_af_nor", 100.0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 5);

		//error if normal cutoff is set
		filter.setDouble("max_af_nor", 2.0);
		filter.apply(vl, result);
		I_EQUAL(result.countPassing(), 4);
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
		filter.setStringList("cn", QStringList() << "1");
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 37);
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

	void FilterCnvLoglikelihood_apply()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvLoglikelihood filter;
		filter.setDouble("min_ll", 11.0);
		filter.setBool("scale_by_regions", false);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 2);
	}

	void FilterCnvLoglikelihood_apply_scaled()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvLoglikelihood filter;
		filter.setDouble("min_ll", 7.0);
		filter.setBool("scale_by_regions", true);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 7);
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

	void FilterCnvLoglikelihood_apply_trio_with_regions()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline_trio_with_regions.tsv"));

		FilterResult result(cnvs.count());

		//default
		FilterCnvLoglikelihood filter;
		filter.setDouble("min_ll", 20.0);
		filter.setBool("scale_by_regions", true);
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

		FilterCnvCnpOverlap filter;
		filter.setDouble("max_ol", 0.001);
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 65);
	}

	void FilterCnvPathogenicCnvOverlap_apply()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_somatic.tsv"));

		FilterResult result(cnvs.count());

		FilterCnvPathogenicCnvOverlap filter;
		filter.apply(cnvs, result);
		I_EQUAL(result.countPassing(), 3);
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
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline_trio_with_regions.tsv"));

		FilterCascade filters = FilterCascadeFile::load(TESTDATA("data_in/CnvList_filters.ini"), "default filter (ClinCNV)");
		FilterResult result = filters.apply(cnvs, true);
		I_EQUAL(result.countPassing(), 116);
	}

	void default_filters_ClinCNV_somatic()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_somatic.tsv"));

		FilterCascade filters = FilterCascadeFile::load(TESTDATA("data_in/CnvList_filters.ini"), "default filter (ClinCNV)");
		FilterResult result = filters.apply(cnvs, false);
		I_EQUAL(result.countPassing(), 31);
	}


	/********************************************* Filters for SVs *********************************************/

	void FilterSvType_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		FilterSvType filter;
		filter.setStringList("Structural variant type", QStringList("DEL"));
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 29);
	}

	void FilterSvRemoveChromosomeType_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		//default
		FilterSvRemoveChromosomeType filter;
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 74);
	}

	void FilterSvGenotypeAffected_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		FilterSvGenotypeAffected filter;
		filter.setStringList("genotypes", QStringList("hom"));
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 43);
	}

	void FilterSvGenotypeAffected_apply_multi()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline_multi.bedpe"));

		FilterResult result(svs.count());

		FilterSvGenotypeAffected filter;
		filter.setStringList("genotypes", QStringList("hom"));
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 6);
	}

	void FilterSvGenotypeControl_apply_multi()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline_multi.bedpe"));

		FilterResult result(svs.count());

		FilterSvGenotypeControl filter;
		filter.setStringList("genotypes", QStringList() << "het" << "wt");
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 10);
	}

	void FilterSvQuality_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		FilterSvQuality filter;
		filter.setInteger("quality", 365);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 72);
	}

	void FilterSvFilterColumn_remove()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		FilterSvFilterColumn filter;
		filter.setStringList("entries", QStringList("AMBIGUOUS"));
		filter.setString("action", "REMOVE");
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 74);
	}

	void FilterSvFilterColumn_filter()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		FilterSvFilterColumn filter;
		filter.setStringList("entries", QStringList("MaxDepth"));
		filter.setString("action", "FILTER");
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 2);
	}

    void FilterSvFilterColumn_filterMulti()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		FilterSvFilterColumn filter;
		filter.setStringList("entries", QStringList() << "MaxDepth" << "SampleFT");
		filter.setString("action", "FILTER");
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 7);
	}

	void FilterSvFilterColumn_keep()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		//pre-filter SVs
		FilterSvType pre_filter;
		pre_filter.setStringList("Structural variant type", QStringList("DEL"));
		pre_filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 29);


		FilterSvFilterColumn filter;
		filter.setStringList("entries", QStringList("AMBIGUOUS"));
		filter.setString("action", "KEEP");
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 42);
	}

	void FilterSvPairedReadAF_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		FilterSvPairedReadAF filter;
		filter.setDouble("Paired Read AF", 0.4);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 8);
	}

	void FilterSvPairedReadAF_apply_multi()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline_multi.bedpe"));

		FilterResult result(svs.count());

		FilterSvPairedReadAF filter;
		filter.setDouble("Paired Read AF", 0.13);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 2);
	}

    void FilterSvPairedReadAF_apply_multi2()
    {
        BedpeFile svs;
        svs.load(TESTDATA("data_in/SV_Manta_germline_multi.bedpe"));

        FilterResult result(svs.count());

        FilterSvPairedReadAF filter;
        filter.setDouble("Paired Read AF", 0.13);
        filter.setBool("only_affected", true);
        filter.apply(svs, result);
        I_EQUAL(result.countPassing(), 2);
    }

	void FilterSvSplitReadAF_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		FilterSvSplitReadAF filter;
		filter.setDouble("Split Read AF", 0.2);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 11);
	}

	void FilterSvSplitReadAF_apply_multi()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline_multi.bedpe"));

		FilterResult result(svs.count());

		FilterSvSplitReadAF filter;
		filter.setDouble("Split Read AF", 0.55);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 2);
	}

    void FilterSvSplitReadAF_apply_multi2()
    {
        BedpeFile svs;
        svs.load(TESTDATA("data_in/SV_Manta_germline_multi.bedpe"));

        FilterResult result(svs.count());

        FilterSvSplitReadAF filter;
		filter.setDouble("Split Read AF", 0.55);
        filter.setBool("only_affected", true);
        filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 4);
    }

	void FilterSvPeReadDepth_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		FilterSvPeReadDepth filter;
		filter.setInteger("PE Read Depth", 10);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 35);
	}

	void FilterSvPeReadDepth_apply_multi()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline_multi.bedpe"));

		FilterResult result(svs.count());

		FilterSvPeReadDepth filter;
		filter.setInteger("PE Read Depth", 20);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 2);
	}

    void FilterSvPeReadDepth_apply_multi2()
    {
        BedpeFile svs;
        svs.load(TESTDATA("data_in/SV_Manta_germline_multi.bedpe"));

        FilterResult result(svs.count());

        FilterSvPeReadDepth filter;
        filter.setInteger("PE Read Depth", 20);
        filter.setBool("only_affected", true);
        filter.apply(svs, result);
        I_EQUAL(result.countPassing(), 3);
    }

	void FilterSvSomaticscore_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_somatic.bedpe"));

		FilterResult result(svs.count());

		FilterSvSomaticscore filter;
		filter.setInteger("Somaticscore", 60);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 4);
	}

	void FilterSvGeneConstraint_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		// default
		FilterSvGeneConstraint filter;
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 21l);
	}

	void FilterSvSize_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		FilterSvSize filter;
		filter.setInteger("min_size", 100);
		filter.setInteger("max_size", 600);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 33);
	}

	void FilterSvOMIM_filter_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		// default
		FilterSvOMIM filter;
		filter.setString("action", "FILTER");
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 12);
	}

	void FilterSvOMIM_remove_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		// default
		FilterSvOMIM filter;
		filter.setString("action", "REMOVE");
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 75);
	}

	void FilterSvCountNGSD_apply_default()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		// default
		FilterSvCountNGSD filter1;
		filter1.apply(svs, result);
		I_EQUAL(result.countPassing(), 84);
	}

	void FilterSvCountNGSD_apply_ignore_genotype()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		// overlap
		FilterSvCountNGSD filter;
		filter.setBool("ignore_genotype", true);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 76);

	}

	void FilterSvCountNGSD_apply_custom()
	{

		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		// custom maximum
		FilterSvCountNGSD filter;
		filter.setInteger("max_count", 10);
		filter.setBool("ignore_genotype", true);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 71);
	}

	void FilterSvAfNGSD_apply_default()
	{

		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		// default maximum
		FilterSvAfNGSD filter;
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 78);
	}

	void FilterSvAfNGSD_apply_custom()
	{

		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		// custom maximum
		FilterSvAfNGSD filter;
		filter.setDouble("max_af", 50.0);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 83);
	}

	void FilterSvBreakpointDensityNGSD_default()
	{

		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		// default maximum
		FilterSvBreakpointDensityNGSD filter;
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 85);
	}

	void FilterSvBreakpointDensityNGSD_custom()
	{

		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		// custom maximum
		FilterSvBreakpointDensityNGSD filter;
		filter.setInteger("max_density", 4);
		filter.setBool("remove_strict", true);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 82);
	}

	void FilterSvBreakpointDensityNGSD_system_specific()
	{

		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterResult result(svs.count());

		// custom maximum
		FilterSvBreakpointDensityNGSD filter;
		filter.setInteger("max_density", 1);
		filter.setBool("only_system_specific", true);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 84);
	}

	void FilterSvTrio_apply()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline_trio.bedpe"));

		FilterResult result(svs.count());

		// custom maximum
		FilterSvTrio filter;
		filter.setString("gender_child", "female");
		// de-novo
		filter.setStringList("types", QStringList() << "de-novo");
		result.reset();
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 10);
		// recessive
		filter.setStringList("types", QStringList() << "recessive");
		result.reset();
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 10);
		// imprinting
		filter.setStringList("types", QStringList() << "imprinting");
		result.reset();
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 5);
	}

	/********************************************* Filters for lrSVs *********************************************/

	void FilterSvLrAF_apply()
	{

		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Sniffles_germline.bedpe"));

		FilterResult result(svs.count());

		FilterSvLrAF filter;
		filter.setDouble("min_af", 0.2);
		filter.setDouble("max_af", 0.4);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 15);
	}

	void FilterSvLrSupportReads_apply()
	{

		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Sniffles_germline.bedpe"));

		FilterResult result(svs.count());

		// custom maximum
		FilterSvLrSupportReads filter;
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 73);

		filter.setInteger("min_support", 15);
		filter.apply(svs, result);
		I_EQUAL(result.countPassing(), 32);
	}


	/********************************************* Default filters for SVs *********************************************/

	void default_filters_SV_germline()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_germline.bedpe"));

		FilterCascade filters = FilterCascadeFile::load(TESTDATA("data_in/SV_filters.ini"), "germline");
		FilterResult result = filters.apply(svs, true);
		I_EQUAL(result.countPassing(), 74);
	}

	void default_filters_SV_somatic()
	{
		BedpeFile svs;
		svs.load(TESTDATA("data_in/SV_Manta_somatic.bedpe"));

		FilterCascade filters = FilterCascadeFile::load(TESTDATA("data_in/SV_filters.ini"), "somatic");
		FilterResult result = filters.apply(svs, true);
		I_EQUAL(result.countPassing(), 8);
	}


	/********************************************* store/load all filters *********************************************/

	void store_and_load()
	{
		QString tmp_file = Helper::tempFileName(".txt");
		QStringList names = FilterFactory::filterNames();
		foreach(QString name, names)
		{
			//store
			FilterCascade filter_cascade;
			filter_cascade.add(FilterFactory::create(name));
			filter_cascade.store(tmp_file);

			//load
			FilterCascade filter_cascade2;
			filter_cascade2.load(tmp_file);

			//compare
			IS_TRUE(filter_cascade==filter_cascade2);
		}
	}

	void load_bug_empty_enum()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantFilter_in.GSvar"));

		FilterCascade filter;
		filter.load(TESTDATA("data_in/FilterCascade_bug_empty_enum.txt"));
		filter.apply(vl, true);
	}

};
