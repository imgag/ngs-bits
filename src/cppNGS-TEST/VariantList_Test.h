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
		if (ref_file=="") SKIP("Test needs the reference genome!");

		VariantList vl;
		vl.load(TESTDATA("data_in/LeftAlign_in1.vcf"));
		vl.leftAlign(ref_file);
		vl.store("out/LeftAlign_out1.vcf");
		COMPARE_FILES("out/LeftAlign_out1.vcf", TESTDATA("data_out/LeftAlign_out1.vcf"));
	}

	void leftAlign02()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");

		VariantList vl;
		vl.load(TESTDATA("data_in/LeftAlign_in2.vcf"));
		vl.checkValid();
		vl.leftAlign(ref_file);
		vl.store("out/LeftAlign_out2.vcf");
		COMPARE_FILES("out/LeftAlign_out2.vcf", TESTDATA("data_out/LeftAlign_out2.vcf"));
	}

	void leftAlign03()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");

		VariantList vl;
		vl.load(TESTDATA("data_in/LeftAlign_in.tsv"));
		vl.checkValid();
		vl.leftAlign(ref_file);
		vl.store("out/LeftAlign_out.tsv");
		COMPARE_FILES("out/LeftAlign_out.tsv", TESTDATA("data_out/LeftAlign_out.tsv"));
	}

	void leftAlign04()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");

		VariantList vl;
		vl.load(TESTDATA("data_in/LeftAlign_in4.vcf"));
		vl.checkValid();
		vl.leftAlign(ref_file);
		vl.store("out/LeftAlign_out4.vcf");
		COMPARE_FILES("out/LeftAlign_out4.vcf", TESTDATA("data_out/LeftAlign_out4.vcf"));
	}

	//check that it works with empty variant lists
	void leftAlign_Empty()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");

		VariantList vl;
		vl.leftAlign(ref_file);
	}

	void removeDuplicates_VCF()
	{
		VariantList vl,vl2;
		vl.load(TESTDATA("data_in/panel.vcf"));
		vl.checkValid();
		vl.sort();
		vl2.load(TESTDATA("data_in/variantList_removeDuplicates.vcf"));
		vl2.checkValid();
		vl2.removeDuplicates(true);
		//after removal of duplicates (and numerical sorting of vl), vl and vl2 should be the same
		I_EQUAL(vl.count(),vl2.count());
		for (int i=0; i<vl.count(); i++)
		{
			S_EQUAL(vl[i].start(),vl2[i].start());
			S_EQUAL(vl[i].obs() ,vl2[i].obs());
		}
	}

	void removeDuplicates_TSV()
	{
		VariantList vl,vl2;
		vl.load(TESTDATA("data_in/variantList_removeDuplicates_in.tsv"));
		vl.checkValid();
		vl.removeDuplicates(true);
		vl2.load(TESTDATA("data_out/variantList_removeDuplicates_out.tsv"));
		vl2.checkValid();
		vl2.sort();
		//after removal of duplicates vl and vl2 should be the same
		I_EQUAL(vl.count(),vl2.count());
		for (int i=0; i<vl2.count(); i++)
		{
			S_EQUAL(vl[i].start(),vl2[i].start());
			S_EQUAL(vl[i].obs() ,vl2[i].obs());
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
		vl.load(TESTDATA("data_in/panel.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.comments().count(), 2);
		S_EQUAL(vl.sampleNames()[0], QString("./Sample_GS120297A3/GS120297A3.bam"));
		I_EQUAL(vl.annotations().count(), 27);

		VariantAnnotationDescription vad = vl.annotationDescriptionByName("ID");
		S_EQUAL(vad.name(), QString("ID"));
		X_EQUAL(vad.type(), VariantAnnotationDescription::STRING);
		S_EQUAL(vad.number(), QString("1"));
		S_EQUAL(vad.description(), QString("ID of the variant, often dbSNP rsnumber"));
		IS_FALSE(vad.sampleSpecific());

		vad = vl.annotationDescriptionByName("INDEL");
		S_EQUAL(vad.name(), QString("INDEL"));
		X_EQUAL(vad.type(), VariantAnnotationDescription::FLAG);
		S_EQUAL(vad.number(), QString("0"));
		S_EQUAL(vad.description(), QString("Indicates that the variant is an INDEL."));
		IS_FALSE(vad.sampleSpecific());

		vad = vl.annotationDescriptionByName("DP4");
		S_EQUAL(vad.name(), QString("DP4"));
		I_EQUAL(vad.type(), VariantAnnotationDescription::INTEGER);
		S_EQUAL(vad.number(), QString("4"));
		S_EQUAL(vad.description(), QString("# high-quality ref-forward bases, ref-reverse, alt-forward and alt-reverse bases"));
		IS_FALSE(vad.sampleSpecific());

		vad = vl.annotationDescriptionByName("PL",true);
		S_EQUAL(vad.name(), QString("PL"));
		S_EQUAL(vad.number(), QString("G"));
		S_EQUAL(vad.description(), QString("List of Phred-scaled genotype likelihoods"));
		IS_TRUE(vad.sampleSpecific());
		I_EQUAL(vad.type(), VariantAnnotationDescription::INTEGER);

		I_EQUAL(vl.filters().count(), 2);
		S_EQUAL(vl.filters()["q10"], QString("Quality below 10"));
		S_EQUAL(vl.filters()["s50"], QString("Less than 50% of samples have data"));

		X_EQUAL(vl[0].chr(), Chromosome("chr17"));
		I_EQUAL(vl[0].start(), 72196817);
		I_EQUAL(vl[0].end(), 72196817);
		S_EQUAL(vl[0].ref(), Sequence("G"));
		S_EQUAL(vl[0].obs(), Sequence("GA"));
		S_EQUAL(vl[0].annotations().at(3), QByteArray("TRUE"));
		S_EQUAL(vl[0].annotations().at(8), QByteArray("4,3,11,11"));
		S_EQUAL(vl[0].annotations().at(26), QByteArray("255,0,123"));
		I_EQUAL(vl[0].filters().count(), 0);

		I_EQUAL(vl[11].filters().count(), 1);
		S_EQUAL(vl[11].filters().at(0), QByteArray("low_DP"));

		X_EQUAL(vl[12].chr(), Chromosome("chr9"));
		I_EQUAL(vl[12].start(), 130931421);
		I_EQUAL(vl[12].end(), 130931421);
		S_EQUAL(vl[12].ref(), Sequence("G"));
		S_EQUAL(vl[12].obs(), Sequence("A"));
		S_EQUAL(vl[12].annotations().at(3), QByteArray(""));
		S_EQUAL(vl[12].annotations().at(8), QByteArray("457,473,752,757"));
		S_EQUAL(vl[12].annotations().at(26), QByteArray("255,0,255"));
		I_EQUAL(vl[12].filters().count(), 0);

		//load a second time to check initialization
		vl.load(TESTDATA("data_in/panel.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.annotations().count(), 27);
		I_EQUAL(vl.comments().count(), 2);
		S_EQUAL(vl.sampleNames()[0], QString("./Sample_GS120297A3/GS120297A3.bam"));
	}

	void loadFromVCF_withROI()
	{
		BedFile roi;
		roi.append(BedLine("chr17", 72196820, 72196892));
		roi.append(BedLine("chr18", 67904549, 67904670));

		VariantList vl;
		vl.load(TESTDATA("data_in/panel.vcf"), VariantList::VCF, &roi);
		vl.checkValid();
		I_EQUAL(vl.count(), 4);
		I_EQUAL(vl.comments().count(), 2);
		S_EQUAL(vl.sampleNames()[0], QString("./Sample_GS120297A3/GS120297A3.bam"));
		I_EQUAL(vl.annotations().count(), 27);

		X_EQUAL(vl[0].chr(), Chromosome("chr17"));
		I_EQUAL(vl[0].start(), 72196887);
		X_EQUAL(vl[1].chr(), Chromosome("chr17"));
		I_EQUAL(vl[1].start(), 72196892);
		X_EQUAL(vl[2].chr(), Chromosome("chr18"));
		I_EQUAL(vl[2].start(), 67904549);
		X_EQUAL(vl[3].chr(), Chromosome("chr18"));
		I_EQUAL(vl[3].start(), 67904586);
	}

	void loadFromVCF_noSampleOrFormatColumn()
	{
		VariantList vl;

		vl.load(TESTDATA("data_in/VariantList_loadFromVCF_noSample.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.annotations().count(), 27);
		I_EQUAL(vl.comments().count(), 2);
		S_EQUAL(vl.sampleNames()[0], QString("Sample"));

		vl.clear();
		vl.load(TESTDATA("data_in/VariantList_loadFromVCF_noFormatSample.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.annotations().count(), 27);
		I_EQUAL(vl.comments().count(), 2);
		S_EQUAL(vl.sampleNames()[0], QString("Sample"));
	}

	void loadFromVCF_undeclaredAnnotations()
	{
		VariantList vl;

		//check annotation list
		vl.load(TESTDATA("data_in/VariantList_loadFromVCF_undeclaredAnnotations.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 2);
		I_EQUAL(vl.annotations().count(), 18);
		QStringList names;
		foreach(VariantAnnotationHeader d, vl.annotations())
		{
			names << d.name();
		}
		S_EQUAL(names.join(","), QString("ID,QUAL,FILTER,DP,AF,RO,AO,GT,GQ,GL,DP,RO,QR,AO,QA,TRIO,CIGAR,TRIO2"));

		//check variants
		S_EQUAL(vl[0].annotations()[16], QByteArray("1X"));
		S_EQUAL(vl[1].annotations()[16], QByteArray(""));
		S_EQUAL(vl[0].annotations()[17], QByteArray(""));
		S_EQUAL(vl[1].annotations()[17], QByteArray("HET,9,0.56,WT,17,0.00,HOM,19,1.00"));
	}

	void loadFromVCF_emptyFormatAndInfo()
	{
		QString in = TESTDATA("data_in/VariantList_loadFromVCF_emptyInfoAndFormat.vcf");
		QString out = "out/VariantList_loadFromVCF_emptyInfoAndFormat.vcf";

		VariantList vl;
		vl.load(in);
		vl.checkValid();
		vl.store(out);

		COMPARE_FILES(in,out);
	}

	void storeToVCF()
	{
		//store loaded file
		VariantList vl;
		vl.load(TESTDATA("data_in/panel.vcf"));
		vl.checkValid();
		vl.store("out/VariantList_store_01.vcf");
		vl.clear();

		//reload and check that everything stayed the same
		vl.load("out/VariantList_store_01.vcf");
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.comments().count(), 2);
		S_EQUAL(vl.sampleNames()[0], QString("./Sample_GS120297A3/GS120297A3.bam"));

		I_EQUAL(vl.annotations().count(), 27);
		S_EQUAL(vl.annotations()[0].name(), QString("ID"));

		VariantAnnotationDescription vad = vl.annotationDescriptionByName("ID");
		I_EQUAL(vad.type(), VariantAnnotationDescription::STRING);
		S_EQUAL(vad.number(), QString("1"));
		S_EQUAL(vad.description(), QString("ID of the variant, often dbSNP rsnumber"));
		IS_FALSE(vad.sampleSpecific());

		vad = vl.annotationDescriptionByName("INDEL");
		S_EQUAL(vad.name(), QString("INDEL"));
		I_EQUAL(vad.type(), VariantAnnotationDescription::FLAG);
		S_EQUAL(vad.number(), QString("0"));
		S_EQUAL(vad.description(), QString("Indicates that the variant is an INDEL."));
		IS_FALSE(vad.sampleSpecific());

		vad = vl.annotationDescriptionByName("DP4");
		S_EQUAL(vad.name(), QString("DP4"));
		I_EQUAL(vad.type(), VariantAnnotationDescription::INTEGER);
		S_EQUAL(vad.number(), QString("4"));
		S_EQUAL(vad.description(), QString("# high-quality ref-forward bases, ref-reverse, alt-forward and alt-reverse bases"));
		IS_FALSE(vad.sampleSpecific());

		vad = vl.annotationDescriptionByName("PL",true);
		S_EQUAL(vad.name(), QString("PL"));
		S_EQUAL(vad.number(), QString("G"));
		S_EQUAL(vad.description(), QString("List of Phred-scaled genotype likelihoods"));
		IS_TRUE(vad.sampleSpecific());
		I_EQUAL(vad.type(), VariantAnnotationDescription::INTEGER);

		I_EQUAL(vl.filters().count(), 2);
		S_EQUAL(vl.filters()["q10"], QString("Quality below 10"));
		S_EQUAL(vl.filters()["s50"], QString("Less than 50% of samples have data"));

		X_EQUAL(vl[0].chr(), Chromosome("chr17"));
		I_EQUAL(vl[0].start(), 72196817);
		I_EQUAL(vl[0].end(), 72196817);
		S_EQUAL(vl[0].ref(), Sequence("G"));
		S_EQUAL(vl[0].obs(), Sequence("GA"));
		S_EQUAL(vl[0].annotations().at(3), QByteArray("TRUE"));
		S_EQUAL(vl[0].annotations().at(8), QByteArray("4,3,11,11"));
		S_EQUAL(vl[0].annotations().at(26), QByteArray("255,0,123"));

		X_EQUAL(vl[12].chr(), Chromosome("chr9"));
		I_EQUAL(vl[12].start(), 130931421);
		I_EQUAL(vl[12].end(), 130931421);
		S_EQUAL(vl[12].ref(), Sequence("G"));
		S_EQUAL(vl[12].obs(), Sequence("A"));
		S_EQUAL(vl[12].annotations().at(3), QByteArray(""));
		S_EQUAL(vl[12].annotations().at(8), QByteArray("457,473,752,757"));
		S_EQUAL(vl[12].annotations().at(26), QByteArray("255,0,255"));
	}

	void loadFromTSV()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel.tsv"));
		vl.checkValid();
		I_EQUAL(vl.count(), 75);
		I_EQUAL(vl.annotations().count(), 27);
		S_EQUAL(vl.annotations()[0].name(), QString("genotype"));
		S_EQUAL(vl.annotations()[26].name(), QString("validated"));
		I_EQUAL(vl.filters().count(), 3);
		S_EQUAL(vl.filters()["low_DP"], QString("Depth less than 20 at variant location."));
		S_EQUAL(vl.filters()["low_MQM"], QString("Mean mapping quality of alternate allele less than Q50."));
		S_EQUAL(vl.filters()["low_QUAL"], QString("Variant quality less than Q30."));

		X_EQUAL(vl[0].chr(), Chromosome("chr1"));
		I_EQUAL(vl[0].start(), 155205047);
		I_EQUAL(vl[0].end(), 155205047);
		S_EQUAL(vl[0].ref(), Sequence("C"));
		S_EQUAL(vl[0].obs(), Sequence("T"));
		S_EQUAL(vl[0].annotations().at(0), QByteArray("het"));
		S_EQUAL(vl[0].annotations().at(5), QByteArray("0.5084"));
		S_EQUAL(vl[0].annotations().at(26), QByteArray(""));
		I_EQUAL(vl[0].filters().count(), 0);

		I_EQUAL(vl[13].filters().count(), 1);
		S_EQUAL(vl[13].filters().at(0), QByteArray("low_QUAL"));

		I_EQUAL(vl[72].filters().count(), 2);
		S_EQUAL(vl[72].filters().at(0), QByteArray("low_QUAL"));
		S_EQUAL(vl[72].filters().at(1), QByteArray("low_MQM"));

		X_EQUAL(vl[74].chr(), Chromosome("chrX"));
		I_EQUAL(vl[74].start(), 153009197);
		I_EQUAL(vl[74].end(), 153009197);
		S_EQUAL(vl[74].ref(), Sequence("G"));
		S_EQUAL(vl[74].obs(), Sequence("C"));
		S_EQUAL(vl[74].annotations().at(0), QByteArray("het"));
		S_EQUAL(vl[74].annotations().at(5), QByteArray("0.5368"));
		S_EQUAL(vl[74].annotations().at(25), QByteArray(""));
		I_EQUAL(vl[74].filters().count(), 0);


		//load a second time to check initialization
		vl.load(TESTDATA("data_in/panel.tsv"));
		vl.checkValid();
		I_EQUAL(vl.count(), 75);
		I_EQUAL(vl.annotations().count(), 27);
	}

	void loadFromTSV_withROI()
	{
		BedFile roi;
		roi.append(BedLine("chr16", 74750405, 74808425));
		roi.append(BedLine("chr19", 7607441, 7607564));

		VariantList vl;
		vl.load(TESTDATA("data_in/panel.tsv"), VariantList::TSV, &roi);
		I_EQUAL(vl.count(), 4);
		I_EQUAL(vl.annotations().count(), 27);
		S_EQUAL(vl.annotations()[0].name(), QString("genotype"));
		S_EQUAL(vl.annotations()[26].name(), QString("validated"));
		I_EQUAL(vl.filters().count(), 3);
		S_EQUAL(vl.filters()["low_DP"], QString("Depth less than 20 at variant location."));
		S_EQUAL(vl.filters()["low_MQM"], QString("Mean mapping quality of alternate allele less than Q50."));
		S_EQUAL(vl.filters()["low_QUAL"], QString("Variant quality less than Q30."));

		X_EQUAL(vl[0].chr(), Chromosome("chr16"));
		I_EQUAL(vl[0].start(), 74750405);
		X_EQUAL(vl[1].chr(), Chromosome("chr16"));
		I_EQUAL(vl[1].start(), 74808425);
		X_EQUAL(vl[2].chr(), Chromosome("chr19"));
		I_EQUAL(vl[2].start(), 7607441);
		X_EQUAL(vl[3].chr(), Chromosome("chr19"));
		I_EQUAL(vl[3].start(), 7607564);
	}

	void storeToTSV()
	{
		//store loaded tsv file
		VariantList vl;
		vl.load(TESTDATA("data_in/panel.tsv"));
		vl.checkValid();
		vl.store("out/VariantList_store_01.tsv");
		vl.clear();

		//reload and check that everything stayed the same
		vl.load("out/VariantList_store_01.tsv");
		vl.checkValid();
		I_EQUAL(vl.count(), 75);

		I_EQUAL(vl.annotations().count(), 27);
		S_EQUAL(vl.annotations()[0].name(), QString("genotype"));
		S_EQUAL(vl.annotations()[26].name(), QString("validated"));

		I_EQUAL(vl.filters().count(), 3);
		S_EQUAL(vl.filters()["low_DP"], QString("Depth less than 20 at variant location."));
		S_EQUAL(vl.filters()["low_MQM"], QString("Mean mapping quality of alternate allele less than Q50."));
		S_EQUAL(vl.filters()["low_QUAL"], QString("Variant quality less than Q30."));

		X_EQUAL(vl[0].chr(), Chromosome("chr1"));
		I_EQUAL(vl[0].start(), 155205047);
		I_EQUAL(vl[0].end(), 155205047);
		S_EQUAL(vl[0].ref(), Sequence("C"));
		S_EQUAL(vl[0].obs(), Sequence("T"));
		S_EQUAL(vl[0].annotations().at(0), QByteArray("het"));
		S_EQUAL(vl[0].annotations().at(5), QByteArray("0.5084"));
		S_EQUAL(vl[0].annotations().at(26), QByteArray(""));

		X_EQUAL(vl[74].chr(), Chromosome("chrX"));
		I_EQUAL(vl[74].start(), 153009197);
		I_EQUAL(vl[74].end(), 153009197);
		S_EQUAL(vl[74].ref(), Sequence("G"));
		S_EQUAL(vl[74].obs(), Sequence("C"));
		S_EQUAL(vl[74].annotations().at(0), QByteArray("het"));
		S_EQUAL(vl[74].annotations().at(5), QByteArray("0.5368"));
		S_EQUAL(vl[74].annotations().at(25), QByteArray(""));

		//load a second time to check initialization
		vl.load(TESTDATA("data_in/panel.tsv"));
		vl.checkValid();
		I_EQUAL(vl.count(), 75);
		I_EQUAL(vl.annotations().count(), 27);
	}

	void convertVCFtoTSV()
	{
		//store loaded vcf file
		VariantList vl;
		vl.load(TESTDATA("data_in/panel.vcf"));
		vl.checkValid();
		vl.store("out/VariantList_convertVCFtoTSV.tsv");
		vl.clear();

		//reload and check that no information became incorrect (vcf-specific things like annotation dimensions and types are still lost)
		vl.load("out/VariantList_convertVCFtoTSV.tsv");
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.annotations().count(), 27);
		I_EQUAL(vl.comments().count(), 1);
		S_EQUAL(vl.annotations()[0].name(), QString("ID"));
		S_EQUAL(vl.annotationDescriptionByName("ID").description(), QString("ID of the variant, often dbSNP rsnumber"));
		S_EQUAL(vl.annotationDescriptionByName("INDEL").name(), QString("INDEL"));
		S_EQUAL(vl.annotationDescriptionByName("INDEL").description(), QString("Indicates that the variant is an INDEL."));
		S_EQUAL(vl.annotationDescriptionByName("DP4").name(), QString("DP4"));
		S_EQUAL(vl.annotationDescriptionByName("DP4").description(), QString("# high-quality ref-forward bases, ref-reverse, alt-forward and alt-reverse bases"));
		S_EQUAL(vl.annotationDescriptionByName("PL_ss").name(), QString("PL_ss"));
		S_EQUAL(vl.annotationDescriptionByName("PL_ss").description(), QString("List of Phred-scaled genotype likelihoods"));


		X_EQUAL(vl[0].chr(), Chromosome("chr17"));
		I_EQUAL(vl[0].start(), 72196817);
		I_EQUAL(vl[0].end(), 72196817);
		S_EQUAL(vl[0].ref(), Sequence("G"));
		S_EQUAL(vl[0].obs(), Sequence("GA"));
		S_EQUAL(vl[0].annotations().at(3), QByteArray("TRUE"));
		S_EQUAL(vl[0].annotations().at(8), QByteArray("4,3,11,11"));
		S_EQUAL(vl[0].annotations().at(26), QByteArray("255,0,123"));

		X_EQUAL(vl[12].chr(), Chromosome("chr9"));
		I_EQUAL(vl[12].start(), 130931421);
		I_EQUAL(vl[12].end(), 130931421);
		S_EQUAL(vl[12].ref(), Sequence("G"));
		S_EQUAL(vl[12].obs(), Sequence("A"));
		S_EQUAL(vl[12].annotations().at(3), QByteArray(""));
		S_EQUAL(vl[12].annotations().at(8), QByteArray("457,473,752,757"));
		S_EQUAL(vl[12].annotations().at(26), QByteArray("255,0,255"));
	}

	void checkThatEmptyVariantAnnotationsAreFilled()
	{
		//store loaded vcf file
		VariantList vl;
		vl.load(TESTDATA("data_in/VariantList_emptyDescriptions.vcf"));
		vl.checkValid();
		vl.store("out/VariantList_emptyDescriptions_fixed.vcf");
		vl.clear();

		VariantList vl2;
		vl2.load("out/VariantList_emptyDescriptions_fixed.vcf");
		vl2.checkValid();
		I_EQUAL(vl2.count(), 14);
		I_EQUAL(vl2.annotations().count(), 27);
		foreach(VariantAnnotationHeader ah, vl2.annotations())
		{
			VariantAnnotationDescription ad = vl2.annotationDescriptionByName(ah.name(),!ah.sampleID().isNull());
			if (ah.name()=="GQ" || ah.name()=="MQ")
			{
				S_EQUAL(ad.description(), "no description available");
			}
			else
			{
				IS_FALSE(ad.description()=="no description available");
			}
		}
	}

	void loadFromVCF_GZ()
	{
		VariantList vl;
		VariantList::Format format = vl.load(TESTDATA("data_in/VariantList_load_zipped.vcf.gz"));
		vl.checkValid();
		I_EQUAL(format, VariantList::VCF_GZ);
		I_EQUAL(vl.count(), 157);
		I_EQUAL(vl.annotations().count(), 75);
		S_EQUAL(vl.annotations()[0].name(), "ID");
		S_EQUAL(vl.annotations()[1].name(), "QUAL");
		S_EQUAL(vl.annotations()[2].name(), "FILTER");
		S_EQUAL(vl.annotations()[3].name(), "NS");
		S_EQUAL(vl.annotations()[74].name(), "EXAC_AF");

		X_EQUAL(vl[0].chr().str(), "chr1");
		I_EQUAL(vl[0].start(), 27687466);
		I_EQUAL(vl[0].end(), 27687466);
		S_EQUAL(vl[0].ref(), Sequence("G"));
		S_EQUAL(vl[0].obs(), Sequence("T"));
		S_EQUAL(vl[0].annotations().at(0), "rs35659744");
		S_EQUAL(vl[0].annotations().at(1), "11836.9");
		S_EQUAL(vl[0].annotations().at(2), ".");
		S_EQUAL(vl[0].annotations().at(3), "1");
		S_EQUAL(vl[0].annotations().at(74), "0.223");

		X_EQUAL(vl[156].chr().str(), "chr20");
		I_EQUAL(vl[156].start(), 48301146);
		I_EQUAL(vl[156].end(), 48301146);
		S_EQUAL(vl[156].ref(), Sequence("G"));
		S_EQUAL(vl[156].obs(), Sequence("A"));
		S_EQUAL(vl[156].annotations().at(0), "rs6512586");
		S_EQUAL(vl[156].annotations().at(1), "39504.2");
		S_EQUAL(vl[156].annotations().at(2), ".");
		S_EQUAL(vl[156].annotations().at(3), "1");
		S_EQUAL(vl[156].annotations().at(74), "0.516");
	}

	void annotationIndexByName()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel.tsv"));
		vl.checkValid();
		I_EQUAL(vl.annotationIndexByName("genotype", true, false), 0);
		I_EQUAL(vl.annotationIndexByName("genotype", false, false), 0);
		I_EQUAL(vl.annotationIndexByName("validated", true, false), 26);
		I_EQUAL(vl.annotationIndexByName("validated", false, false), 26);
		I_EQUAL(vl.annotationIndexByName("1000g_", false, false), 10);
		I_EQUAL(vl.annotationIndexByName("dbSNP_", false, false), 11);
	}

	//test sort function for VCF files
	void sort()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/sort_in.vcf"));
		vl.checkValid();
		vl.sort();
		vl.store("out/sort_out.vcf");
		COMPARE_FILES("out/sort_out.vcf",TESTDATA("data_out/sort_out.vcf"));
	}

	//test sort function for TSV files
	void sort2()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/sort_in.tsv"));
		vl.checkValid();
		vl.sort();
		vl.store("out/sort_out.tsv");
		COMPARE_FILES("out/sort_out.tsv",TESTDATA("data_out/sort_out.tsv"));

	}

	//test sort function for VCF files
	void sort3()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel.vcf"));
		vl.checkValid();
		vl.sort(true);
		//entries should be sorted numerically

		X_EQUAL(vl[0].chr() ,Chromosome("chr1"));
		I_EQUAL(vl[0].start(),11676308);
		I_EQUAL(vl[1].start(),11676377);
		X_EQUAL(vl[2].chr(), Chromosome("chr2"));
		I_EQUAL(vl[2].start(),139498511);
		X_EQUAL(vl[3].chr(), Chromosome("chr4"));
		I_EQUAL(vl[3].start(),68247038);
		I_EQUAL(vl[4].start(),68247113);
		X_EQUAL(vl[5].chr(), Chromosome("chr9"));
		I_EQUAL(vl[5].start(),130931421);
		I_EQUAL(vl[6].start(),130932396);
		X_EQUAL(vl[7].chr(), Chromosome("chr17"));
		I_EQUAL(vl[7].start(),72196817);
		I_EQUAL(vl[8].start(),72196887);
		I_EQUAL(vl[9].start(),72196892);
		X_EQUAL(vl[10].chr(), Chromosome("chr18"));
		I_EQUAL(vl[10].start(),67904549);
		I_EQUAL(vl[11].start(),67904586);
		I_EQUAL(vl[12].start(),67904672);
		X_EQUAL(vl[13].chr(), Chromosome("chr19"));
		I_EQUAL(vl[13].start(),14466629);
	}

	//test sort function for TSV files (with quality)
	void sort4()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/sort_in_qual.tsv"));
		vl.checkValid();
		vl.sort(true);
		vl.store("out/sort_out_qual.tsv");
		COMPARE_FILES("out/sort_out_qual.tsv",TESTDATA("data_out/sort_out_qual.tsv"));
	}

	//test sortByFile function for *.vcf-files
	void sortByFile()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel.vcf"));
		vl.checkValid();
		vl.sortByFile(TESTDATA("data_in/variantList_sortbyFile.fai"));
		vl.store("out/sortByFile.vcf");
		//entries should be sorted by variantList_sortbyFile.fai, which is reverse-numeric concerning chromosomes
		X_EQUAL(vl[0].chr(),Chromosome("chr19"));
		I_EQUAL(vl[0].start(),14466629);
		X_EQUAL(vl[1].chr(),Chromosome("chr18"));
		I_EQUAL(vl[1].start(),67904549);
		I_EQUAL(vl[2].start(),67904586);
		I_EQUAL(vl[3].start(),67904672);
		X_EQUAL(vl[4].chr(),Chromosome("chr17"));
		I_EQUAL(vl[4].start(),72196817);
		I_EQUAL(vl[5].start(),72196887);
		I_EQUAL(vl[6].start(),72196892);
		X_EQUAL(vl[7].chr(),Chromosome("chr9"));
		I_EQUAL(vl[7].start(),130931421);
		I_EQUAL(vl[8].start(),130932396);
		X_EQUAL(vl[9].chr(),Chromosome("chr4"));
		I_EQUAL(vl[9].start(),68247038);
		I_EQUAL(vl[10].start(),68247113);
		X_EQUAL(vl[11].chr(),Chromosome("chr2"));
		I_EQUAL(vl[11].start(),139498511);
		X_EQUAL(vl[12].chr() ,Chromosome("chr1"));
		I_EQUAL(vl[12].start(),11676308);
		I_EQUAL(vl[13].start(),11676377);
	}

	//test sortByFile function for *.tsv-files
	void sortByFile2()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/sort_in.tsv"));
		vl.checkValid();
		vl.sortByFile(TESTDATA("data_in/variantList_sortbyFile.fai"));
		vl.store("out/sortByFile_out.tsv");
		COMPARE_FILES("out/sortByFile_out.tsv",TESTDATA("data_out/sortByFile_out.tsv"));
	}

	void removeAnnotation()
	{
		VariantList vl;
		vl.load(TESTDATA("data_in/panel.tsv"));
		vl.checkValid();
		int index = vl.annotationIndexByName("depth", true, false);

		I_EQUAL(vl.annotations().count(), 27);
		I_EQUAL(vl.count(), 75);
		I_EQUAL(vl[0].annotations().count(), 27);
		S_EQUAL(vl[0].annotations()[index-1], QByteArray("225"));
		S_EQUAL(vl[0].annotations()[index], QByteArray("728"));
		S_EQUAL(vl[0].annotations()[index+1], QByteArray("37"));

		vl.removeAnnotation(index);

		I_EQUAL(vl.annotations().count(), 26);
		I_EQUAL(vl.count(), 75);
		I_EQUAL(vl[0].annotations().count(), 26);
		S_EQUAL(vl[0].annotations()[index-1], QByteArray("225"));
		S_EQUAL(vl[0].annotations()[index], QByteArray("37"));
		S_EQUAL(vl[0].annotations()[index+1], QByteArray("SNV"));
	}

	//bug (number of variants was used to checked if index is out of range)
	void removeAnnotation_bug()
	{
		VariantList vl;
		vl.annotationDescriptions().append(VariantAnnotationDescription("bla", "some desciption"));
		vl.annotations().append(VariantAnnotationHeader("bla"));

		vl.removeAnnotation(0);

		I_EQUAL(vl.annotations().count(), 0)
	}

	void copyMetaData()
	{
		VariantList vl;
		vl.annotationDescriptions().append(VariantAnnotationDescription("bla", "some desciption"));
		vl.annotations().append(VariantAnnotationHeader("bla"));
		vl.filters().insert("MAF", "Minor allele frequency filter");
		vl.addCommentLine("Comment1");
		vl.append(Variant(Chromosome("chr1"), 1, 2, "A", "C"));

		//copy meta data
		VariantList vl2;
		vl2.copyMetaData(vl);

		//check meta data
		I_EQUAL(vl2.annotationDescriptions().count(), 1);
		I_EQUAL(vl2.annotations().count(), 1);
		I_EQUAL(vl2.filters().count(), 1);
		I_EQUAL(vl2.comments().count(), 1);

		//check no variants
		I_EQUAL(vl2.count(), 0);
	}

	void addAnnotation()
	{
		VariantList vl;
		vl.append(Variant(Chromosome("chr1"), 1, 2, "A", "C"));
		vl.append(Variant(Chromosome("chr2"), 1, 2, "A", "C"));

		int index = vl.addAnnotation("name", "desc", "default");

		I_EQUAL(index, 0);
		I_EQUAL(vl.annotations().count(), 1);
		S_EQUAL(vl.annotationDescriptionByName("name", false, true).description(), "desc");
		I_EQUAL(vl[0].annotations().count(), 1);
		S_EQUAL(vl[0].annotations()[index], "default");
		I_EQUAL(vl[1].annotations().count(), 1);
		S_EQUAL(vl[1].annotations()[index], "default");
	}

	void removeAnnotationByName()
	{
		VariantList vl;
		vl.append(Variant(Chromosome("chr1"), 1, 2, "A", "C"));
		vl.append(Variant(Chromosome("chr2"), 1, 2, "A", "C"));
		vl.addAnnotation("name", "desc", "default");

		vl.removeAnnotationByName("name", true, true);

		I_EQUAL(vl.annotations().count(), 0);
		I_EQUAL(vl[0].annotations().count(), 0);
		I_EQUAL(vl[1].annotations().count(), 0);
	}

};
