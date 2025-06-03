#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "VcfFile.h"
#include "Settings.h"

TEST_CLASS(VcfFile_Test)
{
Q_OBJECT
private slots:

	void removeDuplicates_VCF()
	{
		VcfFile vl,vl2;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl.sort();
		vl2.load(TESTDATA("data_in/variantList_removeDuplicates.vcf"));
		vl2.removeDuplicates(true);
		//after removal of duplicates (and numerical sorting of vl), vl and vl2 should be the same
		I_EQUAL(vl.count(),vl2.count());
		for (int i=0; i<vl.count(); ++i)
		{
            I_EQUAL(vl[i].start(), vl2[i].start());
			I_EQUAL(vl[i].alt().size(), vl2[i].alt().size())
			for(int alt_id = 0; alt_id < vl[i].alt().count(); ++alt_id)
			{
				S_EQUAL(vl[i].alt(alt_id) ,vl2[i].alt(alt_id));
			}
		}
	}

	//check that it works with empty variant lists
	void removeDuplicates_Empty()
	{
		VcfFile vl;
		vl.removeDuplicates(true);
	}

	void loadFromVCF()
	{
		VcfFile vl;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		I_EQUAL(vl.count(), 14);
		//old test expected 3, now two bcs we seperately parse the fileformat
		I_EQUAL(vl.vcfHeader().comments().count(), 2);
		S_EQUAL(vl.sampleIDs().at(0), QString("./Sample_GS120297A3/GS120297A3.bam"));
		//old test checked for annotations().count()==27, with annotations consisting of all formats, informations, id, qual, and filter
		I_EQUAL(vl.vcfHeader().infoLines().count(), 18);
		I_EQUAL(vl.vcfHeader().formatLines().count(), 6);

		InfoFormatLine info_1 = vl.vcfHeader().infoLineByID("INDEL");
		S_EQUAL(info_1.id, QString("INDEL"));
		X_EQUAL(info_1.type, "Flag");
		S_EQUAL(info_1.number, QString("0"));
		S_EQUAL(info_1.description, QString("Indicates that the variant is an INDEL."));

		InfoFormatLine info_2 = vl.vcfHeader().infoLineByID("DP4");
		S_EQUAL(info_2.id, QString("DP4"));
		X_EQUAL(info_2.type, "Integer");
		S_EQUAL(info_2.number, QString("4"));
		S_EQUAL(info_2.description, QString("# high-quality ref-forward bases, ref-reverse, alt-forward and alt-reverse bases"));


		InfoFormatLine format = vl.vcfHeader().formatLineByID("PL");
		S_EQUAL(format.id, QString("PL"));
		S_EQUAL(format.number, QString("G"));
		S_EQUAL(format.description, QString("List of Phred-scaled genotype likelihoods"));
		X_EQUAL(format.type, "Integer");

		I_EQUAL(vl.vcfHeader().filterLines().count(), 3);
		S_EQUAL(vl.vcfHeader().filterLineByID("q10").description, QString("Quality below 10"));
		S_EQUAL(vl.vcfHeader().filterLineByID("s50").description, QString("Less than 50% of samples have data"));

		X_EQUAL(vl[0].chr(), Chromosome("chr17"));
		I_EQUAL(vl[0].start(), 72196817);
		I_EQUAL(vl[0].end(), 72196817);
		S_EQUAL(vl[0].ref(), Sequence("G"));
		S_EQUAL(vl[0].alt(0), Sequence("GA"));
		I_EQUAL(vl[0].alt().count(), 1);
		S_EQUAL(vl[0].info("INDEL"), "TRUE");
		S_EQUAL(vl[0].info("DP4"), "4,3,11,11");
		QByteArray first_sample_name = vl.sampleIDs().at(0);
		QByteArray second_format_name = vl[0].formatKeys().at(1);
		S_EQUAL(vl[0].formatValueFromSample(second_format_name, first_sample_name), QByteArray("255,0,123"));
		I_EQUAL(vl[0].filters().count(), 0);

		I_EQUAL(vl[11].filters().count(), 1);
		S_EQUAL(vl[11].filters().at(0), QByteArray("low_DP"));

		X_EQUAL(vl[12].chr(), Chromosome("chr9"));
		I_EQUAL(vl[12].start(), 130931421);
		I_EQUAL(vl[12].end(), 130931421);
		S_EQUAL(vl[12].ref(), Sequence("G"));
		S_EQUAL(vl[12].alt(0), Sequence("A"));
		I_EQUAL(vl[12].alt().count(), 1);
		S_EQUAL(vl[12].info("DP"), "2512");
		S_EQUAL(vl[12].info("INDEL"), "");
		S_EQUAL(vl[12].info("DP4"), QByteArray("457,473,752,757"));
		first_sample_name = vl.sampleIDs().at(0);
		second_format_name = vl[12].formatKeys().at(1);
		S_EQUAL(vl[12].formatValueFromSample(second_format_name, first_sample_name), QByteArray("255,0,255"));
		I_EQUAL(vl[12].filters().count(), 0);

		//load a second time to check initialization
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.vcfHeader().comments().count(), 2);
		S_EQUAL(vl.sampleIDs()[0], QString("./Sample_GS120297A3/GS120297A3.bam"));
		I_EQUAL(vl.vcfHeader().infoLines().count(), 18);
		I_EQUAL(vl.vcfHeader().formatLines().count(), 6);
	}

	void loadVCFWithNewFilter()
	{
		//test loading and storing with BGZF_NO_COMPRESSION
		VcfFile vcfH;
		vcfH.load(TESTDATA("data_in/VcfFileHandler_in.vcf"), true);
		vcfH.store("out/VcfFileHandler_out.vcf", false, BGZF_NO_COMPRESSION);
		COMPARE_FILES("out/VcfFileHandler_out.vcf", TESTDATA("data_out/VcfFileHandler_out.vcf"));

		//test BGZF_BEST_COMPRESSION
		vcfH.store("out/VcfFileHandler_out.vcf.gz", false, BGZF_BEST_COMPRESSION);
		vcfH.load("out/VcfFileHandler_out.vcf.gz", true);
		vcfH.store("out/VcfFileHandler_out_loaded_from_gzipped.vcf", false, BGZF_NO_COMPRESSION);
		COMPARE_FILES("out/VcfFileHandler_out_loaded_from_gzipped.vcf", TESTDATA("data_out/VcfFileHandler_out.vcf"));

		//test intermediate BGZF COMPRESSION
		vcfH.store("out/VcfFileHandler_out_loaded_from_gzipped_compression5.vcf.gz", false, 5);
		vcfH.load("out/VcfFileHandler_out_loaded_from_gzipped_compression5.vcf.gz", true);
		vcfH.store("out/VcfFileHandler_out_loaded_from_gzipped_2.vcf", false, BGZF_NO_COMPRESSION);
		COMPARE_FILES("out/VcfFileHandler_out_loaded_from_gzipped.vcf", TESTDATA("out/VcfFileHandler_out_loaded_from_gzipped_2.vcf"));

		//test BGZF_GZIP_COMPRESSION
		vcfH.load(TESTDATA("data_in/VcfFileHandler_in.vcf"), true);
		vcfH.store("out/VcfFileHandler_out_gzipped.vcf.gz", false, BGZF_GZIP_COMPRESSION);
		vcfH.load("out/VcfFileHandler_out_gzipped.vcf.gz", true);
		vcfH.store("out/VcfFileHandler_out_loaded_from_gzipped.vcf", false, BGZF_NO_COMPRESSION);
		COMPARE_FILES("out/VcfFileHandler_out_loaded_from_gzipped.vcf", TESTDATA("data_out/VcfFileHandler_out.vcf"));
	}

	void loadFromVCF_withROI()
	{
		BedFile roi;
		roi.append(BedLine("chr17", 72196820, 72196892));
		roi.append(BedLine("chr18", 67904549, 67904670));

		VcfFile vl;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"), roi, false);
		I_EQUAL(vl.count(), 4);
		I_EQUAL(vl.vcfHeader().comments().count(), 2);
		I_EQUAL(vl.sampleIDs().count(), 1);
		S_EQUAL(vl.sampleIDs().at(0), QString("./Sample_GS120297A3/GS120297A3.bam"));
		I_EQUAL(vl.vcfHeader().infoLines().count(), 18);
		I_EQUAL(vl.vcfHeader().formatLines().count(), 6);

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
		VcfFile vl;
		vl.load(TESTDATA("data_in/VariantList_loadFromVCF_noFormatSample.vcf"));
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.vcfHeader().infoLines().count(), 18);
		I_EQUAL(vl.vcfHeader().formatLines().count(), 6);
		I_EQUAL(vl.vcfHeader().comments().count(), 1);
		S_EQUAL(vl.vcfHeader().fileFormat(), QByteArray("VCFv4.1"));
		I_EQUAL(vl.sampleIDs().count(), 0);
	}

	void loadFromVCF_undeclaredAnnotations()
	{
		VcfFile vl;

		//check annotation list
		vl.load(TESTDATA("data_in/VariantList_loadFromVCF_undeclaredAnnotations.vcf"));
		I_EQUAL(vl.count(), 2);
		I_EQUAL(vl.vcfHeader().infoLines().count(), 5);
		I_EQUAL(vl.vcfHeader().formatLines().count(), 10);
		QStringList names;
		foreach(const InfoFormatLine& line, vl.vcfHeader().infoLines())
		{
			names << line.id;
		}
		foreach(const InfoFormatLine&  line, vl.vcfHeader().formatLines())
		{
			names << line.id;
		}
		S_EQUAL(names.join(","), QString("DP,AF,RO,AO,CIGAR,GT,GQ,GL,DP,RO,QR,AO,QA,TRIO,TRIO2"));

		//check variants
		S_EQUAL(vl[0].info("CIGAR"), QByteArray("1X"));
		S_EQUAL(vl[1].info("CIGAR"), QByteArray(""));
		S_EQUAL(vl[0].formatValueFromSample("TRIO2"), QByteArray(""));
		S_EQUAL(vl[1].formatValueFromSample("TRIO2"), QByteArray("HET,9,0.56,WT,17,0.00,HOM,19,1.00"));
	}

	void loadFromVCF_emptyFormatAndInfo()
	{
		QString in = TESTDATA("data_in/VariantList_loadFromVCF_emptyInfoAndFormat.vcf");
		QString out = "out/VariantList_loadFromVCF_emptyInfoAndFormat.vcf";

		VcfFile vl;
		vl.load(in);
		vl.store(out, false, BGZF_NO_COMPRESSION);

		COMPARE_FILES(in,out);
	}

	void storeToVCF()
	{
		//store loaded file
		VcfFile vl;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl.store("out/VariantList_store_01.vcf", false, BGZF_NO_COMPRESSION);
		VCF_IS_VALID_HG19("out/VariantList_store_01.vcf")

		//reload and check that everything stayed the same
		vl.load("out/VariantList_store_01.vcf");
		I_EQUAL(vl.count(), 14);
		//old test expected 3, now two bcs we seperately parse the fileformat
		I_EQUAL(vl.vcfHeader().comments().count(), 2);
		S_EQUAL(vl.sampleIDs().at(0), QString("./Sample_GS120297A3/GS120297A3.bam"));
		//old test checked for annotations().count()==27, with annotations consisting of all formats, informations, id, qual, and filter
		I_EQUAL(vl.vcfHeader().infoLines().count(), 18);
		I_EQUAL(vl.vcfHeader().formatLines().count(), 6);

		InfoFormatLine info_1 = vl.vcfHeader().infoLineByID("INDEL");
		S_EQUAL(info_1.id, QString("INDEL"));
		X_EQUAL(info_1.type, "Flag");
		S_EQUAL(info_1.number, QString("0"));
		S_EQUAL(info_1.description, QString("Indicates that the variant is an INDEL."));

		InfoFormatLine info_2 = vl.vcfHeader().infoLineByID("DP4");
		S_EQUAL(info_2.id, QString("DP4"));
		X_EQUAL(info_2.type, "Integer");
		S_EQUAL(info_2.number, QString("4"));
		S_EQUAL(info_2.description, QString("# high-quality ref-forward bases, ref-reverse, alt-forward and alt-reverse bases"));


		InfoFormatLine format = vl.vcfHeader().formatLineByID("PL");
		S_EQUAL(format.id, QString("PL"));
		S_EQUAL(format.number, QString("G"));
		S_EQUAL(format.description, QString("List of Phred-scaled genotype likelihoods"));
		X_EQUAL(format.type, "Integer");

		I_EQUAL(vl.vcfHeader().filterLines().count(), 3);
		S_EQUAL(vl.vcfHeader().filterLineByID("q10").description, QString("Quality below 10"));
		S_EQUAL(vl.vcfHeader().filterLineByID("s50").description, QString("Less than 50% of samples have data"));

		X_EQUAL(vl[0].chr(), Chromosome("chr17"));
		I_EQUAL(vl[0].start(), 72196817);
		I_EQUAL(vl[0].end(), 72196817);
		S_EQUAL(vl[0].ref(), Sequence("G"));
		S_EQUAL(vl[0].alt(0), Sequence("GA"));
		I_EQUAL(vl[0].alt().count(), 1);
		S_EQUAL(vl[0].info("INDEL"), "TRUE");
		S_EQUAL(vl[0].info("DP4"), "4,3,11,11");
		QByteArray first_sample_name = vl.sampleIDs().at(0);
		QByteArray second_format_name = vl[0].formatKeys().at(1);
		S_EQUAL(vl[0].formatValueFromSample(second_format_name, first_sample_name), QByteArray("255,0,123"));
		I_EQUAL(vl[0].filters().count(), 0);

		I_EQUAL(vl[11].filters().count(), 1);
		S_EQUAL(vl[11].filters().at(0), QByteArray("low_DP"));

		X_EQUAL(vl[12].chr(), Chromosome("chr9"));
		I_EQUAL(vl[12].start(), 130931421);
		I_EQUAL(vl[12].end(), 130931421);
		S_EQUAL(vl[12].ref(), Sequence("G"));
		S_EQUAL(vl[12].alt(0), Sequence("A"));
		I_EQUAL(vl[12].alt().count(), 1);
		S_EQUAL(vl[12].info("DP"), "2512");
		S_EQUAL(vl[12].info("INDEL"), "");
		S_EQUAL(vl[12].info("DP4"), QByteArray("457,473,752,757"));
		first_sample_name = vl.sampleIDs().at(0);
		second_format_name = vl[12].formatKeys().at(1);
		S_EQUAL(vl[12].formatValueFromSample(second_format_name, first_sample_name), QByteArray("255,0,255"));
		I_EQUAL(vl[12].filters().count(), 0);
	}

	void checkThatEmptyVariantAnnotationsAreFilled()
	{
		//store loaded vcf file
		VcfFile vl;
		vl.load(TESTDATA("data_in/VariantList_emptyDescriptions.vcf"));
		vl.store("out/VariantList_emptyDescriptions_fixed.vcf", false, BGZF_NO_COMPRESSION);
		VCF_IS_VALID_HG19("out/VariantList_emptyDescriptions_fixed.vcf")

		VcfFile vl2;
		vl2.load("out/VariantList_emptyDescriptions_fixed.vcf");
		I_EQUAL(vl2.count(), 14);
		I_EQUAL(vl.vcfHeader().infoLines().count(), 18);
		I_EQUAL(vl.vcfHeader().formatLines().count(), 6);
		foreach(InfoFormatLine info, vl2.vcfHeader().infoLines())
		{
			if (info.id=="MQ")
			{
				S_EQUAL(info.description, "no description available");
			}
			else
			{
				IS_FALSE(info.description=="no description available");
			}
		}
		foreach(InfoFormatLine format, vl2.vcfHeader().formatLines())
		{
			if (format.id=="GQ")
			{
				S_EQUAL(format.description, "no description available");
			}
			else
			{
				IS_FALSE(format.description=="no description available");
			}
		}
	}

	void loadFromVCF_GZ()
	{
		VcfFile vl;
		vl.load(TESTDATA("data_in/VariantList_load_zipped.vcf.gz"));
		I_EQUAL(vl.count(), 157);
		I_EQUAL(vl.vcfHeader().infoLines().count(), 64);
		I_EQUAL(vl.vcfHeader().formatLines().count(), 8);
		S_EQUAL(vl.vcfHeader().infoLines().at(0).id, "NS");
		S_EQUAL(vl.vcfHeader().infoLines().at(63).id, "EXAC_AF");

		X_EQUAL(vl[0].chr().str(), "chr1");
		I_EQUAL(vl[0].start(), 27687466);
		I_EQUAL(vl[0].end(), 27687466);
		S_EQUAL(vl[0].ref(), Sequence("G"));
		S_EQUAL(vl[0].alt(0), Sequence("T"));
		I_EQUAL(vl[0].alt().count(), 1);
		S_EQUAL(vl[0].id().at(0), "rs35659744");
		S_EQUAL(QString::number(vl[0].qual()), "11836.9");
		IS_TRUE(vl[0].filters().empty());
		S_EQUAL(vl[0].info("AC"), "1");
		S_EQUAL(vl[0].info("EXAC_AF"), "0.223");

		X_EQUAL(vl[156].chr().str(), "chr20");
		I_EQUAL(vl[156].start(), 48301146);
		I_EQUAL(vl[156].end(), 48301146);
		S_EQUAL(vl[156].ref(), Sequence("G"));
		S_EQUAL(vl[156].alt(0), Sequence("A"));
		I_EQUAL(vl[156].alt().count(), 1);
		S_EQUAL(vl[156].id().at(0), "rs6512586");
		S_EQUAL(QString::number(vl[156].qual()), "39504.2");
		IS_TRUE(vl[156].filters().empty());
		S_EQUAL(vl[156].info("NS"), "1");
		S_EQUAL(vl[156].info("AC"), "2");
		S_EQUAL(vl[156].info("EXAC_AF"), "0.516");
	}

	void vepIndexByName()
	{
		VcfFile vl;
		vl.load(TESTDATA("data_in/panel_vep.vcf"));
		I_EQUAL(vl.vcfHeader().vepIndexByName("Allele", false), 0);
		I_EQUAL(vl.vcfHeader().vepIndexByName("Consequence", false), 1);
		I_EQUAL(vl.vcfHeader().vepIndexByName("IMPACT", false), 2);
		I_EQUAL(vl.vcfHeader().vepIndexByName("HGMD_PHEN", false), 59);
		I_EQUAL(vl.vcfHeader().vepIndexByName("Oranguta-Klaus", false), -1);
	}

	void sort()
	{
		VcfFile vl;
		vl.load(TESTDATA("data_in/sort_in.vcf"));
		vl.sort();
		vl.store("out/sort_out.vcf", false, BGZF_NO_COMPRESSION);
		COMPARE_FILES("out/sort_out.vcf",TESTDATA("data_out/sort_out.vcf"));
		VCF_IS_VALID("out/sort_out.vcf")
	}

	void storeAfterAddingSample()
	{
		VcfFile vl;
		vl.load(TESTDATA("data_in/sort_in.vcf"));
		vl.sort();
		vl.setSampleNames(QByteArrayList() << "Sample_1");
		for (int i=0; i<vl.count(); i++)
		{
			vl[i].setFormatKeys(QByteArrayList() << "CT");
			vl[i].addFormatValues(QByteArrayList() << QByteArray::number(i));
		}
		vl.store("out/sort_out2.vcf", false, BGZF_NO_COMPRESSION);
		COMPARE_FILES("out/sort_out2.vcf",TESTDATA("data_out/sort_out2.vcf"));
		VCF_IS_VALID("out/sort_out2.vcf")
	}

	void storeAfterAddingThreeSamples()
	{
		VcfFile vl;
		vl.load(TESTDATA("data_in/sort_in.vcf"));
		vl.sort();
		vl.setSampleNames(QByteArrayList() << "Sample_1" << "Sample_2" << "Sample_3");
		for (int i=0; i<(vl.count()); i++)
		{
			vl[i].setFormatKeys(QByteArrayList() << "CT");
			vl[i].addFormatValues(QByteArrayList() << QByteArray::number(i));
			vl[i].addFormatValues(QByteArrayList() << QByteArray::number(i));
			vl[i].addFormatValues(QByteArrayList() << QByteArray::number(i));
		}
		vl.store("out/sort_out3.vcf", false, BGZF_NO_COMPRESSION);
		COMPARE_FILES("out/sort_out3.vcf",TESTDATA("data_out/sort_out3.vcf"));
		VCF_IS_VALID("out/sort_out3.vcf")
	}

	//test sort function for VCF files (with quality)
	void sort3()
	{
		VcfFile vl;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
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

	//test sortByFile function for *.vcf-files
	void sortByFile()
	{
		VcfFile vl;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl.sortByFile(TESTDATA("data_in/variantList_sortbyFile.fai"));
		vl.store("out/sortByFile.vcf", false, BGZF_NO_COMPRESSION);
		//entries should be sorted by variantList_sortbyFile.fai, which is reverse-numeric concerning chromosomes
		VCF_IS_VALID_HG19("out/sortByFile.vcf")
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

	void storeAsTsv()
	{
		//store loaded vcf file
		VcfFile vl_vcf;
		vl_vcf.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl_vcf.storeAsTsv("out/VcfFile_storeAsTsv.tsv");


		COMPARE_FILES("out/VcfFile_storeAsTsv.tsv", TESTDATA("data_out/VcfFile_storeAsTsv.tsv"));
	}

	void fromGSvar_single()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		VariantList variant_list;
		variant_list.load(TESTDATA("data_in/VcfFile_fromGSvar_single.GSvar"));

		VcfFile vcf_file = VcfFile::fromGSvar(variant_list, ref_file);
		vcf_file.store("out/VcfFile_fromGSvar_out1.vcf");

		COMPARE_FILES("out/VcfFile_fromGSvar_out1.vcf", TESTDATA("data_out/VcfFile_fromGSvar_out1.vcf"));

		QString out_string;
		QTextStream out_stream(&out_string);
		bool valid_vcf = VcfFile::isValid("out/VcfFile_fromGSvar_out1.vcf", ref_file, out_stream, true);
		IS_TRUE(valid_vcf);
	}


	void fromGSvar_trio()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		VariantList variant_list;
		variant_list.load(TESTDATA("data_in/VcfFile_fromGSvar_trio.GSvar"));

		//tests multisample and leftNormalize from GSvar format to VCF for insertion, deletion, SNP
		VcfFile vcf_file = VcfFile::fromGSvar(variant_list, ref_file);
		vcf_file.store("out/VcfFile_fromGSvar_out2.vcf");

		COMPARE_FILES("out/VcfFile_fromGSvar_out2.vcf", TESTDATA("data_out/VcfFile_fromGSvar_out2.vcf"));

		QString out_string;
		QTextStream out_stream(&out_string);
		bool valid_vcf = VcfFile::isValid("out/VcfFile_fromGSvar_out2.vcf", ref_file, out_stream, true);
		IS_TRUE(valid_vcf);
	}

	void fromGSvar_somatic()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		VariantList variant_list;
		variant_list.load(TESTDATA("data_in/VcfFile_fromGSvar_somatic.GSvar"));

		//tests multisample and leftNormalize from GSvar format to VCF for insertion, deletion, SNP
		VcfFile vcf_file = VcfFile::fromGSvar(variant_list, ref_file);
		vcf_file.store("out/VcfFile_fromGSvar_out3.vcf");

		COMPARE_FILES("out/VcfFile_fromGSvar_out3.vcf", TESTDATA("data_out/VcfFile_fromGSvar_out3.vcf"));

		QString out_string;
		QTextStream out_stream(&out_string);
		bool valid_vcf = VcfFile::isValid("out/VcfFile_fromGSvar_out3.vcf", ref_file, out_stream, true);
		IS_TRUE(valid_vcf);
	}

	void getSampleIds()
	{
		VcfFile vcf_file;

		//load multisample
		vcf_file.load(TESTDATA("data_in/VcfFileHandler_in.vcf"), true);
		QByteArrayList sample_ids = vcf_file.sampleIDs();
		I_EQUAL(sample_ids.count(), 2);
		S_EQUAL(sample_ids.at(0), "normal");
		S_EQUAL(sample_ids.at(1), "tumor");

		//load single sample
		vcf_file.load(TESTDATA("data_in/VcfFileHandler_in.vcf"), false);
		sample_ids = vcf_file.sampleIDs();
		I_EQUAL(sample_ids.count(), 1);
		S_EQUAL(sample_ids.at(0), "normal");
	}

	void getInfoIds()
	{
		VcfFile vcf_file;
		vcf_file.load(TESTDATA("data_in/VcfFileHandler_in.vcf"));

		I_EQUAL(vcf_file.vcfHeader().infoLines().count(), 23);
		S_EQUAL(vcf_file.vcfHeader().infoLines().at(0).id, "CSQ"); //the first INFO ID that was already in the header
		S_EQUAL(vcf_file.vcfHeader().infoLines().at(2).id, "IC"); //the first INFO ID not mentioned in header and parsed from VcfLine
		S_EQUAL(vcf_file.vcfHeader().infoLines().at(16).id, "TQSS_NT"); //the last INFO ID not mentioned in header and parsed from VcfLine
	}

	void getFormatIds()
	{
		VcfFile vcf_file;
		vcf_file.load(TESTDATA("data_in/VcfFileHandler_in.vcf"));

		I_EQUAL(vcf_file.vcfHeader().formatLines().count(), 16);
		S_EQUAL(vcf_file.vcfHeader().formatLines().at(0).id, "GT"); //first one must always be GT if mentioned
		S_EQUAL(vcf_file.vcfHeader().formatLines().at(15).id, "SUBDP");
	}

    void vcf_check_default_params()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

        QString output;
        QTextStream out_stream(&output);
		bool is_valid = VcfFile::isValid(TESTDATA("data_in/panel_vep.vcf"), ref_file, out_stream);

        IS_TRUE(is_valid);
        I_EQUAL(output.length(), 0);
    }

    void vcf_check_with_info()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

        QString output;
        QTextStream out_stream(&output);
		bool is_valid = VcfFile::isValid(TESTDATA("data_in/panel_vep.vcf"), ref_file, out_stream, true);

        IS_TRUE(is_valid);
        QStringList output_lines = output.split("\n");
        I_EQUAL(output_lines.length(), 78);
        S_EQUAL(output_lines[0], "VCF version: 4.2");
    }

	void vcf_check_duplicates()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		QString output;
		QTextStream out_stream(&output);
		bool is_valid = VcfFile::isValid(TESTDATA("data_in/duplicate_variants.vcf"), ref_file, out_stream, true, 10000, true);

		IS_FALSE(is_valid);
	}

    void vcf_url_encoding()
    {
        QString input_string = "Test-String= blabla%, \t; \r\n; \r";
        QString output_string = VcfFile::encodeInfoValue(input_string);
        S_EQUAL(output_string, "Test-String%3D%20blabla%25%2C%20%09%3B%20%0D%0A%3B%20%0D");
    }

    void vcf_url_decoding()
    {
        QString input_string = "Test-String%3D%20blabla%25%2C%20%09%3B%20%0D%0A%3B%20%0D";
        QString output_string = VcfFile::decodeInfoValue(input_string);
        S_EQUAL(output_string, "Test-String= blabla%, \t; \r\n; \r");
    }

	void copyMetaData()
	{
		VcfFile vl;

		FilterLine filter_line;
		filter_line.id = "FILTERID";
		filter_line.description = "FILTERDESCRIPTION";

		InfoFormatLine info_line;
		info_line.id = "INFOID";
		info_line.description = "INFODESCRIPTION";
		info_line.number = ".";

		vl.vcfHeader().addFilterLine(filter_line);
		vl.vcfHeader().addInfoLine(info_line);
		QByteArray comment = "##CommentKey=CommentValue";
		vl.vcfHeader().setCommentLine(comment, 0);

		VcfLine vcf_line(Chromosome("chr1"), 1, "A", QList<Sequence>() << "C");
		vl.append(vcf_line);

		//copy meta data
		VcfFile vl2;
		vl2.copyMetaData(vl);

		//check meta data
		I_EQUAL(vl2.vcfHeader().filterLines().count(), 1);
		IS_TRUE(vl2.vcfHeader().filterIdDefined("FILTERID"));
		I_EQUAL(vl2.vcfHeader().infoLines().count(), 1);
		S_EQUAL(vl2.vcfHeader().infoLines().at(0).id, QString("INFOID"));

		I_EQUAL(vl2.vcfHeader().filterLines().count(), 1);
		I_EQUAL(vl2.vcfHeader().infoLines().count(), 1);
		I_EQUAL(vl2.vcfHeader().formatLines().count(), 0);
		I_EQUAL(vl2.vcfHeader().comments().count(), 1);
		S_EQUAL(vl2.vcfHeader().comments().at(0).key, QString("CommentKey"));

		//check no variants
		I_EQUAL(vl2.count(), 0);
	}

	void copyMetaData_check_sampleInfoFormatPtrs()
	{
		VcfFile vcf_file;
		vcf_file.load(TESTDATA("data_in/panel_snpeff.vcf"));

		I_EQUAL(vcf_file.sampleIDs().count(), 1);
		S_EQUAL(vcf_file.sampleIDs().at(0), QString("./Sample_GS120297A3/GS120297A3.bam"));


		I_EQUAL(vcf_file.count(), 14);
		I_EQUAL(vcf_file[0].formatKeys().count(), 3);
		I_EQUAL(vcf_file[0].infoKeys().count(), 9);
	}

    void convertToStringAndParseFromString()
    {
        VcfFile vcf_file;
		vcf_file.load(TESTDATA("data_in/panel_vep.vcf"));

        // to string
        QByteArray vcf_string = vcf_file.toText();


        // from string
        VcfFile vcf_file_from_text;
        vcf_file_from_text.fromText(vcf_string);

        // write to file
        vcf_file.store("out/panel_vep_fromString.vcf");

		COMPARE_FILES("out/panel_vep_fromString.vcf", TESTDATA("data_in/panel_vep.vcf"));
    }

    void loadStoreComparison()
    {
        VcfFile vcf_file;
		vcf_file.load(TESTDATA("data_in/panel_vep.vcf"));
		vcf_file.store("out/panel_vep_loadStore.vcf");
		COMPARE_FILES("out/panel_vep_loadStore.vcf", TESTDATA("data_in/panel_vep.vcf"));
    }

};
