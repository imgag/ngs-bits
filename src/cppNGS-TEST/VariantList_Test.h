#include "TestFramework.h"
#include "VariantList.h"
#include "Settings.h"

TEST_CLASS(VariantList_Test)
{
Q_OBJECT
private slots:

	void leftAlign()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");

		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/LeftAlign_in1.vcf"));
		vl.leftAlign(ref_file);
		vl.store("out/LeftAlign_out1.vcf");
		TFW::comareFiles("out/LeftAlign_out1.vcf", QFINDTESTDATA("data_out/LeftAlign_out1.vcf"));
	}

	void leftAlign02()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");

		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/LeftAlign_in2.vcf"));
		vl.leftAlign(ref_file);
		vl.store("out/LeftAlign_out2.vcf");
		TFW::comareFiles("out/LeftAlign_out2.vcf", QFINDTESTDATA("data_out/LeftAlign_out2.vcf"));
	}

	void leftAlign03()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");

		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/LeftAlign_in.tsv"));
		vl.leftAlign(ref_file);
		vl.store("out/LeftAlign_out.tsv");
		TFW::comareFiles("out/LeftAlign_out.tsv", QFINDTESTDATA("data_out/LeftAlign_out.tsv"));
	}

	void leftAlign04()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");

		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/LeftAlign_in4.vcf"));
		vl.leftAlign(ref_file);
		vl.store("out/LeftAlign_out4.vcf");
		TFW::comareFiles("out/LeftAlign_out4.vcf", QFINDTESTDATA("data_out/LeftAlign_out4.vcf"));
	}

	//check that it works with empty variant lists
	void leftAlign_Empty()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");

		VariantList vl;
		vl.leftAlign(ref_file);
	}

	void removeDuplicates_VCF()
	{
		VariantList vl,vl2;
		vl.load(QFINDTESTDATA("data_in/panel.vcf"));
		vl.sort();
		vl2.load(QFINDTESTDATA("data_in/variantList_removeDuplicates.vcf"));
		vl2.removeDuplicates(true);
		//after removal of duplicates (and numerical sorting of vl), vl and vl2 should be the same
		QCOMPARE(vl.count(),vl2.count());
		for (int i=0; i<vl.count(); i++)
		{
			QCOMPARE(vl[i].start(),vl2[i].start());
			QCOMPARE(vl[i].obs() ,vl2[i].obs());
		}
	}

	void removeDuplicates_TSV()
	{
		VariantList vl,vl2;
		vl.load(QFINDTESTDATA("data_in/variantList_removeDuplicates_in.tsv"));
		vl.removeDuplicates(true);
		vl2.load(QFINDTESTDATA("data_out/variantList_removeDuplicates_out.tsv"));
		vl2.sort();
		//after removal of duplicates vl and vl2 should be the same
		QCOMPARE(vl.count(),vl2.count());
		for (int i=0; i<vl2.count(); i++)
		{
			QCOMPARE(vl[i].start(),vl2[i].start());
			QCOMPARE(vl[i].obs() ,vl2[i].obs());
		}
	}

	//check that it works with empty variant lists
	void removeDuplicates_Empty()
	{
		VariantList vl;
		vl.removeDuplicates(true);
	}

	void loadFromVCF()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.vcf"));
		QCOMPARE(vl.count(), 14);
		QCOMPARE(vl.annotations().count(), 27);
		QCOMPARE(vl.comments().count(), 2);
		QCOMPARE(vl.sampleName(), QString("./Sample_GS120297A3/GS120297A3.bam"));

		QCOMPARE(vl.annotations()[0].name(), QString("ID"));
		QCOMPARE(vl.annotations()[0].type(), VariantAnnotationDescription::STRING);
		QCOMPARE(vl.annotations()[0].number(), QString("1"));
		QCOMPARE(vl.annotations()[0].description(), QString("ID of the variant, often dbSNP rsnumber"));
		QCOMPARE(vl.annotations()[0].sampleSpecific(), false);
		QCOMPARE(vl.annotations()[3].name(), QString("INDEL"));
		QCOMPARE(vl.annotations()[3].type(), VariantAnnotationDescription::FLAG);
		QCOMPARE(vl.annotations()[3].number(), QString("0"));
		QCOMPARE(vl.annotations()[3].description(), QString("Indicates that the variant is an INDEL."));
		QCOMPARE(vl.annotations()[3].sampleSpecific(), false);
		QCOMPARE(vl.annotations()[8].name(), QString("DP4"));
		QCOMPARE(vl.annotations()[8].type(), VariantAnnotationDescription::INTEGER);
		QCOMPARE(vl.annotations()[8].number(), QString("4"));
		QCOMPARE(vl.annotations()[8].description(), QString("# high-quality ref-forward bases, ref-reverse, alt-forward and alt-reverse bases"));
		QCOMPARE(vl.annotations()[8].sampleSpecific(), false);
		QCOMPARE(vl.annotations()[26].name(), QString("PL"));
		QCOMPARE(vl.annotations()[26].number(), QString("G"));
		QCOMPARE(vl.annotations()[26].description(), QString("List of Phred-scaled genotype likelihoods"));
		QCOMPARE(vl.annotations()[26].sampleSpecific(), true);
		QCOMPARE(vl.annotations()[26].type(), VariantAnnotationDescription::INTEGER);


		QCOMPARE(vl[0].chr(), Chromosome("chr17"));
		QCOMPARE(vl[0].start(), 72196817);
		QCOMPARE(vl[0].end(), 72196817);
		QCOMPARE(vl[0].ref(), Sequence("G"));
		QCOMPARE(vl[0].obs(), Sequence("GA"));
		QCOMPARE(vl[0].annotations().at(3), QByteArray("TRUE"));
		QCOMPARE(vl[0].annotations().at(8), QByteArray("4,3,11,11"));
		QCOMPARE(vl[0].annotations().at(26), QByteArray("255,0,123"));

		QCOMPARE(vl[12].chr(), Chromosome("chr9"));
		QCOMPARE(vl[12].start(), 130931421);
		QCOMPARE(vl[12].end(), 130931421);
		QCOMPARE(vl[12].ref(), Sequence("G"));
		QCOMPARE(vl[12].obs(), Sequence("A"));
		QCOMPARE(vl[12].annotations().at(3), QByteArray(""));
		QCOMPARE(vl[12].annotations().at(8), QByteArray("457,473,752,757"));
		QCOMPARE(vl[12].annotations().at(26), QByteArray("255,0,255"));

		//load a second time to check initialization
		vl.load(QFINDTESTDATA("data_in/panel.vcf"));
		QCOMPARE(vl.count(), 14);
		QCOMPARE(vl.annotations().count(), 27);
		QCOMPARE(vl.comments().count(), 2);
		QCOMPARE(vl.sampleName(), QString("./Sample_GS120297A3/GS120297A3.bam"));
	}

	void loadFromVCF_noSampleOrFormatColumn()
	{
		VariantList vl;

		vl.load(QFINDTESTDATA("data_in/VariantList_loadFromVCF_noSample.vcf"));
		QCOMPARE(vl.count(), 14);
		QCOMPARE(vl.annotations().count(), 27);
		QCOMPARE(vl.comments().count(), 2);
		QCOMPARE(vl.sampleName(), QString("Sample"));

		vl.clear();
		vl.load(QFINDTESTDATA("data_in/VariantList_loadFromVCF_noFormatSample.vcf"));
		QCOMPARE(vl.count(), 14);
		QCOMPARE(vl.annotations().count(), 27);
		QCOMPARE(vl.comments().count(), 2);
		QCOMPARE(vl.sampleName(), QString("Sample"));
	}

	void loadFromVCF_undeclaredAnnotations()
	{
		VariantList vl;

		//check annotation list
		vl.load(QFINDTESTDATA("data_in/VariantList_loadFromVCF_undeclaredAnnotations.vcf"));
		QCOMPARE(vl.count(), 2);
		QCOMPARE(vl.annotations().count(), 18);
		QStringList names;
		foreach(VariantAnnotationDescription d, vl.annotations())
		{
			names << d.name();
		}
		QCOMPARE(names.join(","), QString("ID,QUAL,FILTER,DP,AF,RO,AO,GT,GQ,GL,DP,RO,QR,AO,QA,TRIO,CIGAR,TRIO2"));

		//check variants
		QCOMPARE(vl[0].annotations()[16], QByteArray("1X"));
		QCOMPARE(vl[1].annotations()[16], QByteArray(""));
		QCOMPARE(vl[0].annotations()[17], QByteArray(""));
		QCOMPARE(vl[1].annotations()[17], QByteArray("HET,9,0.56,WT,17,0.00,HOM,19,1.00"));
	}

	void loadFromVCF_emptyFormatAndInfo()
	{
		QString in = QFINDTESTDATA("data_in/VariantList_loadFromVCF_emptyInfoAndFormat.vcf");
		QString out = "out/VariantList_loadFromVCF_emptyInfoAndFormat.vcf";

		VariantList vl;
		vl.load(in);
		vl.store(out);

		TFW::comareFiles(in,out);
	}

	void storeToVCF()
	{
		//store loaded file
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.vcf"));
		vl.store("out/VariantList_store_01.vcf");
		vl.clear();

		//reload and check that everything stayed the same
		vl.load("out/VariantList_store_01.vcf");
		QCOMPARE(vl.count(), 14);
		QCOMPARE(vl.annotations().count(), 27);
		QCOMPARE(vl.comments().count(), 2);
		QCOMPARE(vl.sampleName(), QString("./Sample_GS120297A3/GS120297A3.bam"));

		QCOMPARE(vl.annotations()[0].name(), QString("ID"));
		QCOMPARE(vl.annotations()[0].type(), VariantAnnotationDescription::STRING);
		QCOMPARE(vl.annotations()[0].number(), QString("1"));
		QCOMPARE(vl.annotations()[0].description(), QString("ID of the variant, often dbSNP rsnumber"));
		QCOMPARE(vl.annotations()[0].sampleSpecific(), false);
		QCOMPARE(vl.annotations()[3].name(), QString("INDEL"));
		QCOMPARE(vl.annotations()[3].type(), VariantAnnotationDescription::FLAG);
		QCOMPARE(vl.annotations()[3].number(), QString("0"));
		QCOMPARE(vl.annotations()[3].description(), QString("Indicates that the variant is an INDEL."));
		QCOMPARE(vl.annotations()[3].sampleSpecific(), false);
		QCOMPARE(vl.annotations()[8].name(), QString("DP4"));
		QCOMPARE(vl.annotations()[8].type(), VariantAnnotationDescription::INTEGER);
		QCOMPARE(vl.annotations()[8].number(), QString("4"));
		QCOMPARE(vl.annotations()[8].description(), QString("# high-quality ref-forward bases, ref-reverse, alt-forward and alt-reverse bases"));
		QCOMPARE(vl.annotations()[8].sampleSpecific(), false);
		QCOMPARE(vl.annotations()[26].name(), QString("PL"));
		QCOMPARE(vl.annotations()[26].number(), QString("G"));
		QCOMPARE(vl.annotations()[26].description(), QString("List of Phred-scaled genotype likelihoods"));
		QCOMPARE(vl.annotations()[26].sampleSpecific(), true);
		QCOMPARE(vl.annotations()[26].type(), VariantAnnotationDescription::INTEGER);


		QCOMPARE(vl[0].chr(), Chromosome("chr17"));
		QCOMPARE(vl[0].start(), 72196817);
		QCOMPARE(vl[0].end(), 72196817);
		QCOMPARE(vl[0].ref(), Sequence("G"));
		QCOMPARE(vl[0].obs(), Sequence("GA"));
		QCOMPARE(vl[0].annotations().at(3), QByteArray("TRUE"));
		QCOMPARE(vl[0].annotations().at(8), QByteArray("4,3,11,11"));
		QCOMPARE(vl[0].annotations().at(26), QByteArray("255,0,123"));

		QCOMPARE(vl[12].chr(), Chromosome("chr9"));
		QCOMPARE(vl[12].start(), 130931421);
		QCOMPARE(vl[12].end(), 130931421);
		QCOMPARE(vl[12].ref(), Sequence("G"));
		QCOMPARE(vl[12].obs(), Sequence("A"));
		QCOMPARE(vl[12].annotations().at(3), QByteArray(""));
		QCOMPARE(vl[12].annotations().at(8), QByteArray("457,473,752,757"));
		QCOMPARE(vl[12].annotations().at(26), QByteArray("255,0,255"));
	}

	void loadFromTSV()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.tsv"));
		QCOMPARE(vl.count(), 75);
		QCOMPARE(vl.annotations().count(), 27);
		QCOMPARE(vl.annotations()[0].name(), QString("genotype"));
		QCOMPARE(vl.annotations()[26].name(), QString("validated"));

		QCOMPARE(vl[0].chr(), Chromosome("chr1"));
		QCOMPARE(vl[0].start(), 155205047);
		QCOMPARE(vl[0].end(), 155205047);
		QCOMPARE(vl[0].ref(), Sequence("C"));
		QCOMPARE(vl[0].obs(), Sequence("T"));
		QCOMPARE(vl[0].annotations().at(0), QByteArray("het"));
		QCOMPARE(vl[0].annotations().at(5), QByteArray("0.5084"));
		QCOMPARE(vl[0].annotations().at(26), QByteArray(""));

		QCOMPARE(vl[74].chr(), Chromosome("chrX"));
		QCOMPARE(vl[74].start(), 153009197);
		QCOMPARE(vl[74].end(), 153009197);
		QCOMPARE(vl[74].ref(), Sequence("G"));
		QCOMPARE(vl[74].obs(), Sequence("C"));
		QCOMPARE(vl[74].annotations().at(0), QByteArray("het"));
		QCOMPARE(vl[74].annotations().at(5), QByteArray("0.5368"));
		QCOMPARE(vl[74].annotations().at(25), QByteArray(""));

		//load a second time to check initialization
		vl.load(QFINDTESTDATA("data_in/panel.tsv"));
		QCOMPARE(vl.count(), 75);
		QCOMPARE(vl.annotations().count(), 27);
	}

	void storeToTSV()
	{
		//store loaded tsv file
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.tsv"));
		vl.store("out/VariantList_store_01.tsv");
		vl.clear();

		//reload and check that everything stayed the same
		vl.load("out/VariantList_store_01.tsv");
		QCOMPARE(vl.count(), 75);
		QCOMPARE(vl.annotations().count(), 27);
		QCOMPARE(vl.annotations()[0].name(), QString("genotype"));
		QCOMPARE(vl.annotations()[26].name(), QString("validated"));

		QCOMPARE(vl[0].chr(), Chromosome("chr1"));
		QCOMPARE(vl[0].start(), 155205047);
		QCOMPARE(vl[0].end(), 155205047);
		QCOMPARE(vl[0].ref(), Sequence("C"));
		QCOMPARE(vl[0].obs(), Sequence("T"));
		QCOMPARE(vl[0].annotations().at(0), QByteArray("het"));
		QCOMPARE(vl[0].annotations().at(5), QByteArray("0.5084"));
		QCOMPARE(vl[0].annotations().at(26), QByteArray(""));

		QCOMPARE(vl[74].chr(), Chromosome("chrX"));
		QCOMPARE(vl[74].start(), 153009197);
		QCOMPARE(vl[74].end(), 153009197);
		QCOMPARE(vl[74].ref(), Sequence("G"));
		QCOMPARE(vl[74].obs(), Sequence("C"));
		QCOMPARE(vl[74].annotations().at(0), QByteArray("het"));
		QCOMPARE(vl[74].annotations().at(5), QByteArray("0.5368"));
		QCOMPARE(vl[74].annotations().at(25), QByteArray(""));

		//load a second time to check initialization
		vl.load(QFINDTESTDATA("data_in/panel.tsv"));
		QCOMPARE(vl.count(), 75);
		QCOMPARE(vl.annotations().count(), 27);
	}

	void convertVCFToTSV()
	{
		//store loaded vcf file
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.vcf"));
		vl.store("out/VariantList_store_02.tsv");
		vl.clear();

		//reload and check that no information became incorrect (vcf-specific things like annotation dimensions and types are still lost)
		vl.load("out/VariantList_store_02.tsv");
		QCOMPARE(vl.count(), 14);
		QCOMPARE(vl.annotations().count(), 27);
		QCOMPARE(vl.comments().count(), 1);
		QCOMPARE(vl.annotations()[0].name(), QString("ID"));
		QCOMPARE(vl.annotations()[0].description(), QString("ID of the variant, often dbSNP rsnumber"));
		QCOMPARE(vl.annotations()[3].name(), QString("INDEL"));
		QCOMPARE(vl.annotations()[3].description(), QString("Indicates that the variant is an INDEL."));
		QCOMPARE(vl.annotations()[8].name(), QString("DP4"));
		QCOMPARE(vl.annotations()[8].description(), QString("# high-quality ref-forward bases, ref-reverse, alt-forward and alt-reverse bases"));
		QCOMPARE(vl.annotations()[26].name(), QString("PL_ss"));
		QCOMPARE(vl.annotations()[26].description(), QString("List of Phred-scaled genotype likelihoods"));


		QCOMPARE(vl[0].chr(), Chromosome("chr17"));
		QCOMPARE(vl[0].start(), 72196817);
		QCOMPARE(vl[0].end(), 72196817);
		QCOMPARE(vl[0].ref(), Sequence("G"));
		QCOMPARE(vl[0].obs(), Sequence("GA"));
		QCOMPARE(vl[0].annotations().at(3), QByteArray("TRUE"));
		QCOMPARE(vl[0].annotations().at(8), QByteArray("4,3,11,11"));
		QCOMPARE(vl[0].annotations().at(26), QByteArray("255,0,123"));

		QCOMPARE(vl[12].chr(), Chromosome("chr9"));
		QCOMPARE(vl[12].start(), 130931421);
		QCOMPARE(vl[12].end(), 130931421);
		QCOMPARE(vl[12].ref(), Sequence("G"));
		QCOMPARE(vl[12].obs(), Sequence("A"));
		QCOMPARE(vl[12].annotations().at(3), QByteArray(""));
		QCOMPARE(vl[12].annotations().at(8), QByteArray("457,473,752,757"));
		QCOMPARE(vl[12].annotations().at(26), QByteArray("255,0,255"));
	}

	void annotationIndexByName()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.tsv"));
		QCOMPARE(vl.annotationIndexByName("genotype", true, false), 0);
		QCOMPARE(vl.annotationIndexByName("genotype", false, false), 0);
		QCOMPARE(vl.annotationIndexByName("validated", true, false), 26);
		QCOMPARE(vl.annotationIndexByName("validated", false, false), 26);
		QCOMPARE(vl.annotationIndexByName("1000g_", false, false), 10);
		QCOMPARE(vl.annotationIndexByName("dbSNP_", false, false), 11);
	}

	void filterByRegions()
	{

		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.tsv"));
		QCOMPARE(vl.count(), 75);

		BedFile regions;
		regions.append(BedLine("chr2", 202575822, 241700675));
		regions.append(BedLine("chrX", 73641252, 153009197));
		vl.filterByRegions(regions);

		QCOMPARE(vl.count(), 6);
		QCOMPARE(vl[0].chr(), Chromosome("chr2"));
		QCOMPARE(vl[0].start(), 202598113);
		QCOMPARE(vl[1].chr(), Chromosome("chr2"));
		QCOMPARE(vl[1].start(), 202625615);
		QCOMPARE(vl[2].chr(), Chromosome("chr2"));
		QCOMPARE(vl[2].start(), 241680802);
		QCOMPARE(vl[3].chr(), Chromosome("chrX"));
		QCOMPARE(vl[3].start(), 73641252);
		QCOMPARE(vl[4].chr(), Chromosome("chrX"));
		QCOMPARE(vl[4].start(), 153005605);
		QCOMPARE(vl[5].chr(), Chromosome("chrX"));
		QCOMPARE(vl[5].start(), 153009197);
	}

	void filterByRegionsInverted()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.tsv"));
		QCOMPARE(vl.count(), 75);

		BedFile regions;
		regions.append(BedLine("chr2", 202575822, 241700675));
		regions.append(BedLine("chrX", 73641252, 153009197));
		vl.filterByRegions(regions);
		QCOMPARE(vl.count(), 6);

		regions.clear();
		regions.append(BedLine("chrX", 73641252, 153009197));
		vl.filterByRegions(regions, true);

		QCOMPARE(vl.count(), 3);
		QCOMPARE(vl[0].chr(), Chromosome("chr2"));
		QCOMPARE(vl[0].start(), 202598113);
		QCOMPARE(vl[1].chr(), Chromosome("chr2"));
		QCOMPARE(vl[1].start(), 202625615);
		QCOMPARE(vl[2].chr(), Chromosome("chr2"));
		QCOMPARE(vl[2].start(), 241680802);
	}

	void filterByRules()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.tsv"));
		QCOMPARE(vl.count(), 75);

		QVector<VariantFilter> filters;
		vl.filterByRules(filters);
		QCOMPARE(vl.count(), 75);

		filters.append(VariantFilter("bla", "chr IS_NOT chrX"));
		vl.filterByRules(filters);
		QCOMPARE(vl.count(), 72);

		filters.append(VariantFilter("bla", "*1000g* IS || *1000g* IS invalid || *1000g* < 0.01 || chr IS chrM"));
		vl.filterByRules(filters);
		QCOMPARE(vl.count(), 8);
	}

	//test sort function for VCF files
	void sort()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/sort_in.vcf"));
		vl.sort();
		vl.store("out/sort_out.vcf");
		TFW::comareFiles("out/sort_out.vcf",QFINDTESTDATA("data_out/sort_out.vcf"));
	}

	//test sort function for TSV files
	void sort2()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/sort_in.tsv"));
		vl.sort();
		vl.store("out/sort_out.tsv");
		TFW::comareFiles("out/sort_out.tsv",QFINDTESTDATA("data_out/sort_out.tsv"));

	}

	//test sort function for VCF files
	void sort3()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.vcf"));
		vl.sort(true);
		//entries should be sorted numerically

		QCOMPARE(vl[0].chr() ,Chromosome("chr1"));
		QCOMPARE(vl[0].start(),11676308);
		QCOMPARE(vl[1].start(),11676377);
		QCOMPARE(vl[2].chr(), Chromosome("chr2"));
		QCOMPARE(vl[2].start(),139498511);
		QCOMPARE(vl[3].chr(), Chromosome("chr4"));
		QCOMPARE(vl[3].start(),68247038);
		QCOMPARE(vl[4].start(),68247113);
		QCOMPARE(vl[5].chr(), Chromosome("chr9"));
		QCOMPARE(vl[5].start(),130931421);
		QCOMPARE(vl[6].start(),130932396);
		QCOMPARE(vl[7].chr(), Chromosome("chr17"));
		QCOMPARE(vl[7].start(),72196817);
		QCOMPARE(vl[8].start(),72196887);
		QCOMPARE(vl[9].start(),72196892);
		QCOMPARE(vl[10].chr(), Chromosome("chr18"));
		QCOMPARE(vl[10].start(),67904549);
		QCOMPARE(vl[11].start(),67904586);
		QCOMPARE(vl[12].start(),67904672);
		QCOMPARE(vl[13].chr(), Chromosome("chr19"));
		QCOMPARE(vl[13].start(),14466629);
	}

	//test sort function for TSV files (with quality)
	void sort4()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/sort_in_qual.tsv"));
		vl.sort(true);
		vl.store("out/sort_out_qual.tsv");
		TFW::comareFiles("out/sort_out_qual.tsv",QFINDTESTDATA("data_out/sort_out_qual.tsv"));
	}

	//test sortByFile function for *.vcf-files
	void sortByFile()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.vcf"));
		vl.sortByFile(QFINDTESTDATA("data_in/variantList_sortbyFile.fai"));
		vl.store("out/sortByFile.vcf");
		//entries should be sorted by variantList_sortbyFile.fai, which is reverse-numeric concerning chromosomes
		QCOMPARE(vl[0].chr(),Chromosome("chr19"));
		QCOMPARE(vl[0].start(),14466629);
		QCOMPARE(vl[1].chr(),Chromosome("chr18"));
		QCOMPARE(vl[1].start(),67904549);
		QCOMPARE(vl[2].start(),67904586);
		QCOMPARE(vl[3].start(),67904672);
		QCOMPARE(vl[4].chr(),Chromosome("chr17"));
		QCOMPARE(vl[4].start(),72196817);
		QCOMPARE(vl[5].start(),72196887);
		QCOMPARE(vl[6].start(),72196892);
		QCOMPARE(vl[7].chr(),Chromosome("chr9"));
		QCOMPARE(vl[7].start(),130931421);
		QCOMPARE(vl[8].start(),130932396);
		QCOMPARE(vl[9].chr(),Chromosome("chr4"));
		QCOMPARE(vl[9].start(),68247038);
		QCOMPARE(vl[10].start(),68247113);
		QCOMPARE(vl[11].chr(),Chromosome("chr2"));
		QCOMPARE(vl[11].start(),139498511);
		QCOMPARE(vl[12].chr() ,Chromosome("chr1"));
		QCOMPARE(vl[12].start(),11676308);
		QCOMPARE(vl[13].start(),11676377);
	}

	//test sortByFile function for *.tsv-files
	void sortByFile2()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/sort_in.tsv"));
		vl.sortByFile(QFINDTESTDATA("data_in/variantList_sortbyFile.fai"));
		vl.store("out/sortByFile_out.tsv");
		TFW::comareFiles("out/sortByFile_out.tsv",QFINDTESTDATA("data_out/sortByFile_out.tsv"));
	}

	void removeAnnotation()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.tsv"));
		int index = vl.annotationIndexByName("depth", true, false);

		QCOMPARE(vl.annotations().count(), 27);
		QCOMPARE(vl.count(), 75);
		QCOMPARE(vl[0].annotations().count(), 27);
		QCOMPARE(vl[0].annotations()[index-1], QByteArray("225"));
		QCOMPARE(vl[0].annotations()[index], QByteArray("728"));
		QCOMPARE(vl[0].annotations()[index+1], QByteArray("37"));

		vl.removeAnnotation(index);

		QCOMPARE(vl.annotations().count(), 26);
		QCOMPARE(vl.count(), 75);
		QCOMPARE(vl[0].annotations().count(), 26);
		QCOMPARE(vl[0].annotations()[index-1], QByteArray("225"));
		QCOMPARE(vl[0].annotations()[index], QByteArray("37"));
		QCOMPARE(vl[0].annotations()[index+1], QByteArray("SNV"));
	}
};
