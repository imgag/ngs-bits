#include "../TestFramework.h"
#include "VariantList.h"
#include "QCCollection.h"
#include "Statistics.h"

class Statistics_Test
		: public QObject
{
	Q_OBJECT

private slots:


	void variantList1()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/Statistics_variantList.vcf"));

		QCCollection stats = Statistics::variantList(vl);
		QCOMPARE(stats[0].name(), QString("variant count"));
		QCOMPARE(stats[0].toString(), QString("157"));
		QCOMPARE(stats[0].accession(), QString("QC:2000013"));
		QCOMPARE(stats[1].name(), QString("known variants percentage"));
		QCOMPARE(stats[1].accession(), QString("QC:2000014"));
		QCOMPARE(stats[1].toString(), QString("97.45"));
		QCOMPARE(stats[2].name(), QString("high-impact variants percentage"));
		QCOMPARE(stats[2].accession(), QString("QC:2000015"));
		QCOMPARE(stats[2].toString(), QString("1.27"));
		QCOMPARE(stats[3].name(), QString("homozygous variants percentage"));
		QCOMPARE(stats[3].toString(), QString("39.49"));
		QCOMPARE(stats[4].name(), QString("indel variants percentage"));
		QCOMPARE(stats[4].toString(), QString("4.46"));
		QCOMPARE(stats[5].name(), QString("transition/transversion ratio"));
		QCOMPARE(stats[5].toString(), QString("3.29"));
		QCOMPARE(stats.count(), 6);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			QVERIFY(stats[i].description()!="");
			QVERIFY(stats[i].accession()!="");
		}
	}

	void variantList2()
	{
		VariantList vl;

		QCCollection stats = Statistics::variantList(vl);
		QCOMPARE(stats[0].name(), QString("variant count"));
		QCOMPARE(stats[0].toString(), QString("0"));
		QCOMPARE(stats[1].name(), QString("known variants percentage"));
		QVERIFY(stats[1].toString().startsWith("n/a "));
		QCOMPARE(stats[2].name(), QString("high-impact variants percentage"));
		QVERIFY(stats[2].toString().startsWith("n/a "));
		QCOMPARE(stats[3].name(), QString("homozygous variants percentage"));
		QVERIFY(stats[3].toString().startsWith("n/a "));
		QCOMPARE(stats[4].name(), QString("indel variants percentage"));
		QVERIFY(stats[4].toString().startsWith("n/a "));
		QCOMPARE(stats[5].name(), QString("transition/transversion ratio"));
		QVERIFY(stats[5].toString().startsWith("n/a "));
		QCOMPARE(stats.count(), 6);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			QVERIFY(stats[i].description()!="");
			QVERIFY(stats[i].accession()!="");
		}
	}

	void mapping1()
	{
		BedFile bed_file;
		bed_file.load(QFINDTESTDATA("data_in/panel.bed"));
		bed_file.merge();

		QCCollection stats = Statistics::mapping(bed_file, QFINDTESTDATA("data_in/panel.bam"), 20);
		QCOMPARE(stats[0].name(), QString("trimmed base percentage"));
		QCOMPARE(stats[0].toString(), QString("10.82"));
		QCOMPARE(stats[1].name(), QString("mapped read percentage"));
		QCOMPARE(stats[1].toString(), QString("98.93"));
		QCOMPARE(stats[2].name(), QString("on-target read percentage"));
		QCOMPARE(stats[2].toString(), QString("87.02"));
		QCOMPARE(stats[3].name(), QString("properly-paired read percentage"));
		QCOMPARE(stats[3].toString(), QString("97.86"));
		QCOMPARE(stats[4].name(), QString("insert size"));
		QCOMPARE(stats[4].toString(), QString("180.02"));
		QCOMPARE(stats[5].name(), QString("duplicate read percentage"));
		QCOMPARE(stats[5].toString(), QString("n/a (probably removed from BAM during data analysis)"));
		QCOMPARE(stats[6].name(), QString("target region read depth"));
		QCOMPARE(stats[6].toString(), QString("125.36"));
		QCOMPARE(stats[7].name(), QString("target region 10x percentage"));
		QCOMPARE(stats[7].toString(), QString("97.08"));
		QCOMPARE(stats[8].name(), QString("target region 20x percentage"));
		QCOMPARE(stats[8].toString(), QString("94.05"));
		QCOMPARE(stats[9].name(), QString("target region 30x percentage"));
		QCOMPARE(stats[9].toString(), QString("90.22"));
		QCOMPARE(stats[10].name(), QString("target region 50x percentage"));
		QCOMPARE(stats[10].toString(), QString("80.77"));
		QCOMPARE(stats[11].name(), QString("target region 100x percentage"));
		QCOMPARE(stats[11].toString(), QString("55.38"));
		QCOMPARE(stats[12].name(), QString("target region 200x percentage"));
		QCOMPARE(stats[12].toString(), QString("18.09"));
		QCOMPARE(stats[13].name(), QString("target region 500x percentage"));
		QCOMPARE(stats[13].toString(), QString("0.05"));
		QCOMPARE(stats[14].name(), QString("depth distribution plot"));
		QVERIFY(stats[14].type()==QVariant::ByteArray);
		QCOMPARE(stats[15].name(), QString("insert size distribution plot"));
		QVERIFY(stats[15].type()==QVariant::ByteArray);
		QCOMPARE(stats.count(), 16);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			QVERIFY(stats[i].description()!="");
			QVERIFY(stats[i].accession()!="");
		}
	}

	void mapping2()
	{
		BedFile bed_file;
		bed_file.load(QFINDTESTDATA("data_in/close_exons.bed"));
		bed_file.merge();

		QCCollection stats = Statistics::mapping(bed_file, QFINDTESTDATA("data_in/close_exons.bam"));
		QCOMPARE(stats[0].name(), QString("trimmed base percentage"));
		QCOMPARE(stats[0].toString(), QString("19.10"));
		QCOMPARE(stats[1].name(), QString("mapped read percentage"));
		QCOMPARE(stats[1].toString(), QString("100.00"));
		QCOMPARE(stats[2].name(), QString("on-target read percentage"));
		QCOMPARE(stats[2].toString(), QString("99.94"));
		QCOMPARE(stats[3].name(), QString("properly-paired read percentage"));
		QCOMPARE(stats[3].toString(), QString("100.00"));
		QCOMPARE(stats[4].name(), QString("insert size"));
		QCOMPARE(stats[4].toString(), QString("138.06"));
		QCOMPARE(stats[5].name(), QString("duplicate read percentage"));
		QCOMPARE(stats[5].toString(), QString("n/a (probably removed from BAM during data analysis)"));
		QCOMPARE(stats[6].name(), QString("target region read depth"));
		QCOMPARE(stats[6].toString(), QString("415.05"));
		QCOMPARE(stats[7].name(), QString("target region 10x percentage"));
		QCOMPARE(stats[7].toString(), QString("100.00"));
		QCOMPARE(stats[8].name(), QString("target region 20x percentage"));
		QCOMPARE(stats[8].toString(), QString("100.00"));
		QCOMPARE(stats[9].name(), QString("target region 30x percentage"));
		QCOMPARE(stats[9].toString(), QString("100.00"));
		QCOMPARE(stats[10].name(), QString("target region 50x percentage"));
		QCOMPARE(stats[10].toString(), QString("100.00"));
		QCOMPARE(stats[11].name(), QString("target region 100x percentage"));
		QCOMPARE(stats[11].toString(), QString("93.51"));
		QCOMPARE(stats[12].name(), QString("target region 200x percentage"));
		QCOMPARE(stats[12].toString(), QString("79.87"));
		QCOMPARE(stats[13].name(), QString("target region 500x percentage"));
		QCOMPARE(stats[13].toString(), QString("39.61"));
		QCOMPARE(stats[14].name(), QString("depth distribution plot"));
		QVERIFY(stats[14].type()==QVariant::ByteArray);
		QCOMPARE(stats[15].name(), QString("insert size distribution plot"));
		QVERIFY(stats[15].type()==QVariant::ByteArray);
		QCOMPARE(stats.count(), 16);
	}

	void mapping3()
	{
		QCCollection stats = Statistics::mapping("hg19", QFINDTESTDATA("data_in/close_exons.bam"));
		QCOMPARE(stats[0].name(), QString("trimmed base percentage"));
		QCOMPARE(stats[0].toString(), QString("19.10"));
		QCOMPARE(stats[1].name(), QString("mapped read percentage"));
		QCOMPARE(stats[1].toString(), QString("100.00"));
		QCOMPARE(stats[2].name(), QString("on-target read percentage"));
		QCOMPARE(stats[2].toString(), QString("100.00"));
		QCOMPARE(stats[3].name(), QString("properly-paired read percentage"));
		QCOMPARE(stats[3].toString(), QString("100.00"));
		QCOMPARE(stats[4].name(), QString("insert size"));
		QCOMPARE(stats[4].toString(), QString("138.06"));
		QCOMPARE(stats[5].name(), QString("duplicate read percentage"));
		QCOMPARE(stats[5].toString(), QString("n/a (duplicates not marked or removed during data analysis)"));
		QCOMPARE(stats[6].name(), QString("target region read depth"));
		QCOMPARE(stats[6].toString(8), QString("0.00006538"));
		QCOMPARE(stats[7].name(), QString("insert size distribution plot"));
		QVERIFY(stats[7].type()==QVariant::ByteArray);
		QCOMPARE(stats.count(), 8);
	}

	void mapping3exons()
	{
		QCCollection stats = Statistics::mapping3Exons(QFINDTESTDATA("../tools-TEST/data_in/MappingQC_in2.bam"));
		QCOMPARE(stats[0].name(), QString("error estimation read depth"));
		QCOMPARE(stats[0].toString(), QString("238.84"));
		QCOMPARE(stats[1].name(), QString("error estimation N percentage"));
		QCOMPARE(stats[1].toString(4), QString("0.0693"));
		QCOMPARE(stats[2].name(), QString("error estimation SNV percentage"));
		QCOMPARE(stats[2].toString(4), QString("0.4310"));
		QCOMPARE(stats[3].name(), QString("error estimation indel percentage"));
		QCOMPARE(stats[3].toString(4), QString("0.0038"));
		QCOMPARE(stats.count(), 4);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			QVERIFY(stats[i].description()!="");
			QVERIFY(stats[i].accession()!="");
		}
	}

	void region1()
	{
		BedFile bed_file;
		bed_file.load(QFINDTESTDATA("data_in/demo_unmerged.bed"));

		QCCollection stats = Statistics::region(bed_file, true);
		QCOMPARE(stats[0].name(), QString("roi_bases"));
		QCOMPARE(stats[0].toString(0), QString("92168"));
		QCOMPARE(stats[1].name(), QString("roi_fragments"));
		QCOMPARE(stats[1].toString(), QString("590"));
		QCOMPARE(stats[2].name(), QString("roi_chromosomes"));
		QCOMPARE(stats[2].toString(), QString("13 (1, 2, 3, 4, 6, 7, 8, 10, 12, 16, 17, 18, 22)"));
		QCOMPARE(stats[3].name(), QString("roi_is_sorted"));
		QCOMPARE(stats[3].toString(), QString("yes"));
		QCOMPARE(stats[4].name(), QString("roi_is_merged"));
		QCOMPARE(stats[4].toString(), QString("yes"));
		QCOMPARE(stats[5].name(), QString("roi_fragment_min"));
		QCOMPARE(stats[5].toString(), QString("21"));
		QCOMPARE(stats[6].name(), QString("roi_fragment_max"));
		QCOMPARE(stats[6].toString(), QString("1000"));
		QCOMPARE(stats[7].name(), QString("roi_fragment_mean"));
		QCOMPARE(stats[7].toString(), QString("156.22"));
		QCOMPARE(stats[8].name(), QString("roi_fragment_stdev"));
		QCOMPARE(stats[8].toString(), QString("106.31"));
		QCOMPARE(stats.count(), 9);
		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			QVERIFY(stats[i].description()!="");
		}
	}

	void region2()
	{
		BedFile bed_file;
		bed_file.load(QFINDTESTDATA("data_in/demo_unmerged.bed"));

		QCCollection stats = Statistics::region(bed_file, false);
		QCOMPARE(stats[0].name(), QString("roi_bases"));
		QCOMPARE(stats[0].toString(0), QString("92369"));
		QCOMPARE(stats[1].name(), QString("roi_fragments"));
		QCOMPARE(stats[1].toString(), QString("592"));
		QCOMPARE(stats[2].name(), QString("roi_chromosomes"));
		QCOMPARE(stats[2].toString(), QString("13 (1, 2, 3, 4, 6, 7, 8, 10, 12, 16, 17, 18, 22)"));
		QCOMPARE(stats[3].name(), QString("roi_is_sorted"));
		QCOMPARE(stats[3].toString(), QString("no"));
		QCOMPARE(stats[4].name(), QString("roi_is_merged"));
		QCOMPARE(stats[4].toString(), QString("no"));
		QCOMPARE(stats[5].name(), QString("roi_fragment_min"));
		QCOMPARE(stats[5].toString(), QString("21"));
		QCOMPARE(stats[6].name(), QString("roi_fragment_max"));
		QCOMPARE(stats[6].toString(), QString("965"));
		QCOMPARE(stats[7].name(), QString("roi_fragment_mean"));
		QCOMPARE(stats[7].toString(), QString("156.03"));
		QCOMPARE(stats[8].name(), QString("roi_fragment_stdev"));
		QCOMPARE(stats[8].toString(), QString("102.36"));
		QCOMPARE(stats.count(), 9);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			QVERIFY(stats[i].description()!="");
		}
	}

	void region3()
	{
		BedFile bed_file;
		bed_file.load(QFINDTESTDATA("data_in/WGS_hg19.bed"));

		QCCollection stats = Statistics::region(bed_file, false);
		QCOMPARE(stats[1].name(), QString("roi_fragments"));
		QCOMPARE(stats[1].toString(), QString("25"));
		QCOMPARE(stats[0].name(), QString("roi_bases"));
		QCOMPARE(stats[0].toString(0), QString("3095693958"));
		QCOMPARE(stats[3].name(), QString("roi_is_sorted"));
		QCOMPARE(stats[3].toString(), QString("yes"));
		QCOMPARE(stats[4].name(), QString("roi_is_merged"));
		QCOMPARE(stats[4].toString(), QString("yes"));
		QCOMPARE(stats[5].name(), QString("roi_fragment_min"));
		QCOMPARE(stats[5].toString(), QString("16570"));
		QCOMPARE(stats[6].name(), QString("roi_fragment_max"));
		QCOMPARE(stats[6].toString(), QString("249250620"));
		QCOMPARE(stats[7].name(), QString("roi_fragment_mean"));
		QCOMPARE(stats[7].toString(2), QString("123827758.32"));
		QCOMPARE(stats[8].name(), QString("roi_fragment_stdev"));
		QCOMPARE(stats[8].toString(2), QString("61027437.29"));
		QCOMPARE(stats.count(), 9);

		//check that there is a description for each term
		for (int i=0; i<stats.count(); ++i)
		{
			QVERIFY(stats[i].description()!="");
		}
	}

	void lowCoverage_mapq20()
	{
		BedFile bed_file;
		bed_file.load(QFINDTESTDATA("data_in/panel.bed"));
		bed_file.merge();
		QCOMPARE(bed_file.baseCount(), (long)271536);

		BedFile low_cov =  Statistics::lowCoverage(bed_file, QFINDTESTDATA("data_in/panel.bam"), 20, 20);
		QCOMPARE(low_cov.baseCount(), (long)16165);
	}

	void lowCoverage_mapq20_singletons()
	{
		BedFile bed_file;
		bed_file.load(QFINDTESTDATA("data_in/panel.bed"));
		bed_file.merge();
		QCOMPARE(bed_file.baseCount(), (long)271536);

		BedFile low_cov =  Statistics::lowCoverage(bed_file, QFINDTESTDATA("data_in/panel.bam"), 20, 20, true);
		QCOMPARE(low_cov.baseCount(), (long)16116);
	}

	void lowCoverage_mapq1()
	{
		BedFile bed_file;
		bed_file.load(QFINDTESTDATA("data_in/close_exons.bed"));
		bed_file.merge();
		QCOMPARE(bed_file.baseCount(), (long)154);

		BedFile low_cov =  Statistics::lowCoverage(bed_file, QFINDTESTDATA("data_in/close_exons.bam"), 20, 1);
		QCOMPARE(low_cov.baseCount(), (long)0);
	}

	void avgCoverage1()
	{
		BedFile bed_file;
		bed_file.load(QFINDTESTDATA("data_in/panel.bed"));
		bed_file.merge();

		Statistics::avgCoverage(bed_file, QFINDTESTDATA("data_in/panel.bam"), true, 20);

		QCOMPARE(bed_file.count(), 1532);
		QCOMPARE(bed_file[0].chr(), Chromosome("chr1"));
		QCOMPARE(bed_file[0].start(), 11073775);
		QCOMPARE(bed_file[0].end(), 11074032);
		QCOMPARE(bed_file[0].annotations()[0], QString("105.12"));
	}

	void avgCoverage2()
	{
		BedFile bed_file;
		bed_file.load(QFINDTESTDATA("data_in/close_exons.bed"));
		bed_file.merge();

		Statistics::avgCoverage(bed_file, QFINDTESTDATA("data_in/close_exons.bam"), true, 20);

		QCOMPARE(bed_file.count(), 2);
		QCOMPARE(bed_file[0].chr(), Chromosome("chr1"));
		QCOMPARE(bed_file[0].start(), 45798425);
		QCOMPARE(bed_file[0].end(), 45798516);
		QCOMPARE(bed_file[0].annotations()[0], QString("462.97"));
		QCOMPARE(bed_file[1].chr(), Chromosome("chr1"));
		QCOMPARE(bed_file[1].start(), 45798580);
		QCOMPARE(bed_file[1].end(), 45798641);
		QCOMPARE(bed_file[1].annotations()[0], QString("311.52"));
	}

	void genderXY()
	{
		QStringList debug;
		QString gender = Statistics::genderXY(QFINDTESTDATA("data_in/panel.bam"), debug);
		QCOMPARE(gender, QString("female"));
	}

	void genderHetX()
	{
		QStringList debug;
		QString gender = Statistics::genderHetX(QFINDTESTDATA("data_in/panel.bam"), debug);
		QCOMPARE(gender, QString("unknown (too few SNPs)"));
	}

};

TFW_DECLARE(Statistics_Test)

