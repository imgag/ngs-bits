#include "TestFramework.h"
#include "VariantList.h"
#include "QCCollection.h"
#include "Statistics.h"

TEST_CLASS(Statistics_Test)
{
Q_OBJECT
private slots:

	void somatic()
	{
		QString tumor_bam = TESTDATA("data_in/tumor.bam");
		QString normal_bam = TESTDATA("data_in/normal.bam");
		QString somatic_vcf = TESTDATA("data_in/somatic.vcf");
		QCCollection stats = Statistics::somatic(tumor_bam, normal_bam, somatic_vcf, QString(), QString(), true);

        S_EQUAL(stats[0].name(), QString("sample correlation"));
        S_EQUAL(stats[0].accession(), QString("QC:2000040"));
		S_EQUAL(stats[0].toString(), QString("n/a (too few variants)"));
        S_EQUAL(stats[1].name(), QString("variant count"));
        S_EQUAL(stats[1].accession(), QString("QC:2000013"));
        S_EQUAL(stats[1].toString(), QString("2"));
        S_EQUAL(stats[2].name(), QString("somatic variant count"));
        S_EQUAL(stats[2].accession(), QString("QC:2000041"));
        S_EQUAL(stats[2].toString(), QString("2"));
		S_EQUAL(stats[3].name(), QString("known somatic variants percentage"));
        S_EQUAL(stats[3].accession(), QString("QC:2000045"));
		S_EQUAL(stats[3].toString(), QString("n/a (no EXAC annotation)"));
        S_EQUAL(stats[4].name(), QString("somatic indel percentage"));
        S_EQUAL(stats[4].accession(), QString("QC:2000042"));
        S_EQUAL(stats[4].toString(), QString("50.00"));
        S_EQUAL(stats[5].name(), QString("somatic transition/transversion ratio"));
		S_EQUAL(stats[5].accession(), QString("QC:2000043"));
		S_EQUAL(stats[5].toString(), QString("n/a (no variants or transversions)"));
		S_EQUAL(stats[6].accession(), QString("QC:2000053"));
		S_EQUAL(stats[6].toString(), QString("low (0.00 var/Mb)"));
		S_EQUAL(stats[7].accession(), QString("QC:2000054"));
		S_EQUAL(stats[7].toString(), QString("n/a (too few variants)"));
		I_EQUAL(stats.count(), 8);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			IS_TRUE(stats[i].description()!="");
			IS_TRUE(stats[i].accession()!="");
		}
	}

	void variantList_panel_filter()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/Statistics_variantList.vcf"));

		QCCollection stats = Statistics::variantList(vl, true);
        S_EQUAL(stats[0].name(), QString("variant count"));
		S_EQUAL(stats[0].toString(), QString("151"));
        S_EQUAL(stats[0].accession(), QString("QC:2000013"));
        S_EQUAL(stats[1].name(), QString("known variants percentage"));
        S_EQUAL(stats[1].accession(), QString("QC:2000014"));
		S_EQUAL(stats[1].toString(), QString("97.35"));
		I_EQUAL(stats.count(), 6);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			IS_TRUE(stats[i].description()!="");
			IS_TRUE(stats[i].accession()!="");
		}
	}

	void variantList_panel_nofilter()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/Statistics_variantList.vcf"));

		QCCollection stats = Statistics::variantList(vl, false);
		S_EQUAL(stats[0].name(), QString("variant count"));
		S_EQUAL(stats[0].toString(), QString("157"));
		S_EQUAL(stats[0].accession(), QString("QC:2000013"));
		S_EQUAL(stats[1].name(), QString("known variants percentage"));
		S_EQUAL(stats[1].accession(), QString("QC:2000014"));
		S_EQUAL(stats[1].toString(), QString("97.45"));
		S_EQUAL(stats[2].name(), QString("high-impact variants percentage"));
		S_EQUAL(stats[2].accession(), QString("QC:2000015"));
		S_EQUAL(stats[2].toString(), QString("1.27"));
		S_EQUAL(stats[3].name(), QString("homozygous variants percentage"));
		S_EQUAL(stats[3].toString(), QString("39.49"));
		S_EQUAL(stats[4].name(), QString("indel variants percentage"));
		S_EQUAL(stats[4].toString(), QString("4.46"));
		S_EQUAL(stats[5].name(), QString("transition/transversion ratio"));
		S_EQUAL(stats[5].toString(), QString("3.29"));
		I_EQUAL(stats.count(), 6);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			IS_TRUE(stats[i].description()!="");
			IS_TRUE(stats[i].accession()!="");
		}
	}

    void variantList_empty()
	{
		VariantList vl;

		QCCollection stats = Statistics::variantList(vl, true);
        S_EQUAL(stats[0].name(), QString("variant count"));
        S_EQUAL(stats[0].toString(), QString("0"));
        S_EQUAL(stats[1].name(), QString("known variants percentage"));
        IS_TRUE(stats[1].toString().startsWith("n/a "));
        S_EQUAL(stats[2].name(), QString("high-impact variants percentage"));
        IS_TRUE(stats[2].toString().startsWith("n/a "));
        S_EQUAL(stats[3].name(), QString("homozygous variants percentage"));
        IS_TRUE(stats[3].toString().startsWith("n/a "));
        S_EQUAL(stats[4].name(), QString("indel variants percentage"));
        IS_TRUE(stats[4].toString().startsWith("n/a "));
        S_EQUAL(stats[5].name(), QString("transition/transversion ratio"));
        IS_TRUE(stats[5].toString().startsWith("n/a "));
		I_EQUAL(stats.count(), 6);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			IS_TRUE(stats[i].description()!="");
			IS_TRUE(stats[i].accession()!="");
		}
	}

	void mapping_panel()
	{
		BedFile bed_file;
		bed_file.load(TESTDATA("data_in/panel.bed"));
		bed_file.merge();

		QCCollection stats = Statistics::mapping(bed_file, TESTDATA("data_in/panel.bam"), 20);
        S_EQUAL(stats[0].name(), QString("trimmed base percentage"));
        S_EQUAL(stats[0].toString(), QString("10.82"));
        S_EQUAL(stats[1].name(), QString("clipped base percentage"));
        S_EQUAL(stats[1].toString(), QString("0.00"));
        S_EQUAL(stats[2].name(), QString("mapped read percentage"));
        S_EQUAL(stats[2].toString(), QString("98.93"));
        S_EQUAL(stats[3].name(), QString("on-target read percentage"));
        S_EQUAL(stats[3].toString(), QString("87.02"));
        S_EQUAL(stats[4].name(), QString("properly-paired read percentage"));
        S_EQUAL(stats[4].toString(), QString("97.86"));
        S_EQUAL(stats[5].name(), QString("insert size"));
        S_EQUAL(stats[5].toString(), QString("180.02"));
        S_EQUAL(stats[6].name(), QString("duplicate read percentage"));
        S_EQUAL(stats[6].toString(), QString("n/a (no duplicates marked or duplicates removed during data analysis)"));
        S_EQUAL(stats[7].name(), QString("bases usable (MB)"));
        S_EQUAL(stats[7].toString(), QString("34.11"));
        S_EQUAL(stats[8].name(), QString("target region read depth"));
        S_EQUAL(stats[8].toString(), QString("125.63"));
        S_EQUAL(stats[9].name(), QString("target region 10x percentage"));
        S_EQUAL(stats[9].toString(), QString("97.08"));
        S_EQUAL(stats[10].name(), QString("target region 20x percentage"));
        S_EQUAL(stats[10].toString(), QString("94.06"));
        S_EQUAL(stats[11].name(), QString("target region 30x percentage"));
        S_EQUAL(stats[11].toString(), QString("90.25"));
        S_EQUAL(stats[12].name(), QString("target region 50x percentage"));
        S_EQUAL(stats[12].toString(), QString("80.81"));
        S_EQUAL(stats[13].name(), QString("target region 100x percentage"));
        S_EQUAL(stats[13].toString(), QString("55.51"));
        S_EQUAL(stats[14].name(), QString("target region 200x percentage"));
        S_EQUAL(stats[14].toString(), QString("18.22"));
        S_EQUAL(stats[15].name(), QString("target region 500x percentage"));
        S_EQUAL(stats[15].toString(), QString("0.06"));
        S_EQUAL(stats[16].name(), QString("depth distribution plot"));
        IS_TRUE(stats[16].type()==QVariant::ByteArray);
        S_EQUAL(stats[17].name(), QString("insert size distribution plot"));
        IS_TRUE(stats[17].type()==QVariant::ByteArray);
        I_EQUAL(stats.count(), 18);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			IS_TRUE(stats[i].description()!="");
			IS_TRUE(stats[i].accession()!="");
		}
	}

	void contamination()
	{
		QCCollection stats = Statistics::contamination(TESTDATA("data_in/panel.bam"));
		S_EQUAL(stats[0].name(), QString("SNV allele frequency deviation"));
		S_EQUAL(stats[0].toString(), QString("1.57"));
		I_EQUAL(stats.count(), 1);
	}

	void mapping_close_exons()
	{
		BedFile bed_file;
		bed_file.load(TESTDATA("data_in/close_exons.bed"));
		bed_file.merge();

		QCCollection stats = Statistics::mapping(bed_file, TESTDATA("data_in/close_exons.bam"));
        S_EQUAL(stats[0].name(), QString("trimmed base percentage"));
        S_EQUAL(stats[0].toString(), QString("19.10"));
        S_EQUAL(stats[1].name(), QString("clipped base percentage"));
        S_EQUAL(stats[1].toString(), QString("0.00"));
        S_EQUAL(stats[2].name(), QString("mapped read percentage"));
        S_EQUAL(stats[2].toString(), QString("100.00"));
        S_EQUAL(stats[3].name(), QString("on-target read percentage"));
        S_EQUAL(stats[3].toString(), QString("99.94"));
        S_EQUAL(stats[4].name(), QString("properly-paired read percentage"));
        S_EQUAL(stats[4].toString(), QString("100.00"));
        S_EQUAL(stats[5].name(), QString("insert size"));
        S_EQUAL(stats[5].toString(), QString("138.06"));
        S_EQUAL(stats[6].name(), QString("duplicate read percentage"));
        S_EQUAL(stats[6].toString(), QString("n/a (no duplicates marked or duplicates removed during data analysis)"));
        S_EQUAL(stats[7].name(), QString("bases usable (MB)"));
        S_EQUAL(stats[7].toString(), QString("0.06"));
        S_EQUAL(stats[8].name(), QString("target region read depth"));
        S_EQUAL(stats[8].toString(), QString("415.05"));
        S_EQUAL(stats[9].name(), QString("target region 10x percentage"));
        S_EQUAL(stats[9].toString(), QString("100.00"));
        S_EQUAL(stats[10].name(), QString("target region 20x percentage"));
        S_EQUAL(stats[10].toString(), QString("100.00"));
        S_EQUAL(stats[11].name(), QString("target region 30x percentage"));
        S_EQUAL(stats[11].toString(), QString("100.00"));
        S_EQUAL(stats[12].name(), QString("target region 50x percentage"));
        S_EQUAL(stats[12].toString(), QString("100.00"));
        S_EQUAL(stats[13].name(), QString("target region 100x percentage"));
        S_EQUAL(stats[13].toString(), QString("93.51"));
        S_EQUAL(stats[14].name(), QString("target region 200x percentage"));
        S_EQUAL(stats[14].toString(), QString("79.87"));
        S_EQUAL(stats[15].name(), QString("target region 500x percentage"));
        S_EQUAL(stats[15].toString(), QString("39.61"));
        S_EQUAL(stats[16].name(), QString("depth distribution plot"));
        IS_TRUE(stats[16].type()==QVariant::ByteArray);
        S_EQUAL(stats[17].name(), QString("insert size distribution plot"));
        IS_TRUE(stats[17].type()==QVariant::ByteArray);
        I_EQUAL(stats.count(), 18);
	}

	void mapping_wgs()
	{
        QCCollection stats = Statistics::mapping(TESTDATA("data_in/close_exons.bam"));
        S_EQUAL(stats[0].name(), QString("trimmed base percentage"));
        S_EQUAL(stats[0].toString(), QString("19.10"));
        S_EQUAL(stats[1].name(), QString("clipped base percentage"));
        S_EQUAL(stats[1].toString(), QString("0.00"));
        S_EQUAL(stats[2].name(), QString("mapped read percentage"));
        S_EQUAL(stats[2].toString(), QString("100.00"));
        S_EQUAL(stats[3].name(), QString("on-target read percentage"));
        S_EQUAL(stats[3].toString(), QString("100.00"));
        S_EQUAL(stats[4].name(), QString("properly-paired read percentage"));
        S_EQUAL(stats[4].toString(), QString("100.00"));
        S_EQUAL(stats[5].name(), QString("insert size"));
        S_EQUAL(stats[5].toString(), QString("138.06"));
        S_EQUAL(stats[6].name(), QString("duplicate read percentage"));
        S_EQUAL(stats[6].toString(), QString("n/a (duplicates not marked or removed during data analysis)"));
        S_EQUAL(stats[7].name(), QString("bases usable (MB)"));
        S_EQUAL(stats[7].toString(), QString("0.21"));
        S_EQUAL(stats[8].name(), QString("target region read depth"));
        S_EQUAL(stats[8].toString(8), QString("0.00006626"));
        S_EQUAL(stats[9].name(), QString("insert size distribution plot"));
        IS_TRUE(stats[9].type()==QVariant::ByteArray);
        I_EQUAL(stats.count(), 10);
	}

	void region1()
	{
		BedFile bed_file;
		bed_file.load(TESTDATA("data_in/demo_unmerged.bed"));

		QCCollection stats = Statistics::region(bed_file, true);
        S_EQUAL(stats[0].name(), QString("roi_bases"));
        S_EQUAL(stats[0].toString(0), QString("92168"));
        S_EQUAL(stats[1].name(), QString("roi_fragments"));
        S_EQUAL(stats[1].toString(), QString("590"));
        S_EQUAL(stats[2].name(), QString("roi_chromosomes"));
        S_EQUAL(stats[2].toString(), QString("13 (1, 2, 3, 4, 6, 7, 8, 10, 12, 16, 17, 18, 22)"));
        S_EQUAL(stats[3].name(), QString("roi_is_sorted"));
        S_EQUAL(stats[3].toString(), QString("yes"));
        S_EQUAL(stats[4].name(), QString("roi_is_merged"));
        S_EQUAL(stats[4].toString(), QString("yes"));
        S_EQUAL(stats[5].name(), QString("roi_fragment_min"));
        S_EQUAL(stats[5].toString(), QString("21"));
        S_EQUAL(stats[6].name(), QString("roi_fragment_max"));
        S_EQUAL(stats[6].toString(), QString("1000"));
        S_EQUAL(stats[7].name(), QString("roi_fragment_mean"));
        S_EQUAL(stats[7].toString(), QString("156.22"));
        S_EQUAL(stats[8].name(), QString("roi_fragment_stdev"));
        S_EQUAL(stats[8].toString(), QString("106.31"));
		I_EQUAL(stats.count(), 9);
		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			IS_TRUE(stats[i].description()!="");
		}
	}

	void region2()
	{
		BedFile bed_file;
		bed_file.load(TESTDATA("data_in/demo_unmerged.bed"));

		QCCollection stats = Statistics::region(bed_file, false);
        S_EQUAL(stats[0].name(), QString("roi_bases"));
        S_EQUAL(stats[0].toString(0), QString("92369"));
        S_EQUAL(stats[1].name(), QString("roi_fragments"));
        S_EQUAL(stats[1].toString(), QString("592"));
        S_EQUAL(stats[2].name(), QString("roi_chromosomes"));
        S_EQUAL(stats[2].toString(), QString("13 (1, 2, 3, 4, 6, 7, 8, 10, 12, 16, 17, 18, 22)"));
        S_EQUAL(stats[3].name(), QString("roi_is_sorted"));
        S_EQUAL(stats[3].toString(), QString("no"));
        S_EQUAL(stats[4].name(), QString("roi_is_merged"));
        S_EQUAL(stats[4].toString(), QString("no"));
        S_EQUAL(stats[5].name(), QString("roi_fragment_min"));
        S_EQUAL(stats[5].toString(), QString("21"));
        S_EQUAL(stats[6].name(), QString("roi_fragment_max"));
        S_EQUAL(stats[6].toString(), QString("965"));
        S_EQUAL(stats[7].name(), QString("roi_fragment_mean"));
        S_EQUAL(stats[7].toString(), QString("156.03"));
        S_EQUAL(stats[8].name(), QString("roi_fragment_stdev"));
        S_EQUAL(stats[8].toString(), QString("102.36"));
		I_EQUAL(stats.count(), 9);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			IS_TRUE(stats[i].description()!="");
		}
	}

	void region3()
	{
		BedFile bed_file;
		bed_file.load(TESTDATA("data_in/WGS_hg19.bed"));

		QCCollection stats = Statistics::region(bed_file, false);
        S_EQUAL(stats[1].name(), QString("roi_fragments"));
        S_EQUAL(stats[1].toString(), QString("25"));
        S_EQUAL(stats[0].name(), QString("roi_bases"));
        S_EQUAL(stats[0].toString(0), QString("3095693958"));
        S_EQUAL(stats[3].name(), QString("roi_is_sorted"));
        S_EQUAL(stats[3].toString(), QString("yes"));
        S_EQUAL(stats[4].name(), QString("roi_is_merged"));
        S_EQUAL(stats[4].toString(), QString("yes"));
        S_EQUAL(stats[5].name(), QString("roi_fragment_min"));
        S_EQUAL(stats[5].toString(), QString("16570"));
        S_EQUAL(stats[6].name(), QString("roi_fragment_max"));
        S_EQUAL(stats[6].toString(), QString("249250620"));
        S_EQUAL(stats[7].name(), QString("roi_fragment_mean"));
        S_EQUAL(stats[7].toString(2), QString("123827758.32"));
        S_EQUAL(stats[8].name(), QString("roi_fragment_stdev"));
        S_EQUAL(stats[8].toString(2), QString("61027437.29"));
		I_EQUAL(stats.count(), 9);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			IS_TRUE(stats[i].description()!="");
		}
	}

    void lowCoverage_roi_mapq20()
	{
		BedFile bed_file;
		bed_file.load(TESTDATA("data_in/panel.bed"));
		bed_file.merge();
		I_EQUAL(bed_file.baseCount(), 271536);

        BedFile low_cov =  Statistics::lowCoverage(bed_file, TESTDATA("data_in/panel.bam"), 20, 20);
        I_EQUAL(low_cov.count(), 441);
        I_EQUAL(low_cov.baseCount(), 16116);
	}

    void lowCoverage_roi_closeExons()
	{
		BedFile bed_file;
		bed_file.load(TESTDATA("data_in/close_exons.bed"));
		bed_file.merge();
		I_EQUAL(bed_file.baseCount(), 154);

		BedFile low_cov =  Statistics::lowCoverage(bed_file, TESTDATA("data_in/close_exons.bam"), 20, 1);
		I_EQUAL(low_cov.baseCount(), 0);
	}

	void lowCoverage_roi_bug_case1()
	{
		BedFile bed_file;
		bed_file.append(BedLine("chr13", 32931869, 32931970));

		BedFile low_cov =  Statistics::lowCoverage(bed_file, TESTDATA("data_in/lowcov_bug_case1.bam"), 20, 1);
		I_EQUAL(low_cov.baseCount(), 0);
	}

	void lowCoverage_roi_bug_case2()
	{
		BedFile bed_file;
		bed_file.append(BedLine("chr13", 32931869, 32931970));

		BedFile low_cov =  Statistics::lowCoverage(bed_file, TESTDATA("data_in/lowcov_bug_case2.bam"), 20, 1);
		I_EQUAL(low_cov.baseCount(), 0);
	}

    void lowCoverage_wgs_mapq20()
    {
        BedFile low_cov =  Statistics::lowCoverage(TESTDATA("data_in/panel.bam"), 20, 20);
        I_EQUAL(low_cov.count(), 1806);
        I_EQUAL(low_cov.baseCount(), 3095115275ll);
    }

	void avgCoverage_default()
	{
		BedFile bed_file;
		bed_file.load(TESTDATA("data_in/panel.bed"));
		bed_file.merge();

		Statistics::avgCoverage(bed_file, TESTDATA("data_in/panel.bam"), 20);

		I_EQUAL(bed_file.count(), 1532);
		X_EQUAL(bed_file[0].chr(), Chromosome("chr1"));
		I_EQUAL(bed_file[0].start(), 11073775);
		I_EQUAL(bed_file[0].end(), 11074032);
		S_EQUAL(bed_file[0].annotations()[0], QString("105.12"));
	}

	void avgCoverage_panel_mode()
	{
		BedFile bed_file;
		bed_file.load(TESTDATA("data_in/close_exons.bed"));
		bed_file.merge();

		Statistics::avgCoverage(bed_file, TESTDATA("data_in/close_exons.bam"), 20, false, true);

		I_EQUAL(bed_file.count(), 2);
		X_EQUAL(bed_file[0].chr(), Chromosome("chr1"));
		I_EQUAL(bed_file[0].start(), 45798425);
		I_EQUAL(bed_file[0].end(), 45798516);
		S_EQUAL(bed_file[0].annotations()[0], QString("475.80"));
		X_EQUAL(bed_file[1].chr(), Chromosome("chr1"));
		I_EQUAL(bed_file[1].start(), 45798580);
		I_EQUAL(bed_file[1].end(), 45798641);
		S_EQUAL(bed_file[1].annotations()[0], QString("324.90"));
	}

	void avgCoverage_with_duplicates()
	{
		BedFile bed_file;
		bed_file.load(TESTDATA("data_in/close_exons.bed"));
		bed_file.merge();

		Statistics::avgCoverage(bed_file, TESTDATA("data_in/close_exons.bam"), 20, true, true);

		I_EQUAL(bed_file.count(), 2);
		X_EQUAL(bed_file[0].chr(), Chromosome("chr1"));
		I_EQUAL(bed_file[0].start(), 45798425);
		I_EQUAL(bed_file[0].end(), 45798516);
		S_EQUAL(bed_file[0].annotations()[0], QString("475.80"));
		X_EQUAL(bed_file[1].chr(), Chromosome("chr1"));
		I_EQUAL(bed_file[1].start(), 45798580);
		I_EQUAL(bed_file[1].end(), 45798641);
		S_EQUAL(bed_file[1].annotations()[0], QString("324.90"));
	}

	void genderXY()
	{
		QStringList debug;
		QString gender = Statistics::genderXY(TESTDATA("data_in/panel.bam"), debug);
		S_EQUAL(gender, QString("female"));
	}

	void genderHetX()
	{
		QStringList debug;
		QString gender = Statistics::genderHetX(TESTDATA("data_in/panel.bam"), debug);
		S_EQUAL(gender, QString("unknown (too few SNPs)"));
	}

	void genderSRY()
	{
		QStringList debug;
		QString gender = Statistics::genderSRY(TESTDATA("data_in/panel.bam"), debug);
		S_EQUAL(gender, QString("female"));

		gender = Statistics::genderSRY(TESTDATA("data_in/sry.bam"), debug);
		S_EQUAL(gender, QString("male"));
	}
};

