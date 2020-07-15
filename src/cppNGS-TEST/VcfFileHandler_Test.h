#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "VcfFileHandler.h"

using namespace VcfFormat;

TEST_CLASS(VcfFileHandler_Test)
{
Q_OBJECT
private slots:

	void loadVCF()
	{
		VcfFileHandler vcfH;
		vcfH.load(TESTDATA("data_in/VcfFileHandler_in.vcf"));
		vcfH.store("out/VcfFileHandler_out.vcf");

		COMPARE_FILES("out/VcfFileHandler_out.vcf", TESTDATA("data_out/VcfFileHandler_out.vcf"));
	}

	void type()
	{
		VcfFileHandler vl;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		I_EQUAL(vl.type(false), VcfFormat::GERMLINE_SINGLESAMPLE);
	}

	void removeDuplicates_VCF()
	{
		VcfFileHandler vl,vl2;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl.checkValid();
		vl.sort();
		vl2.load(TESTDATA("data_in/variantList_removeDuplicates.vcf"));
		vl2.checkValid();
		vl2.removeDuplicates(true);
		//after removal of duplicates (and numerical sorting of vl), vl and vl2 should be the same
		I_EQUAL(vl.count(),vl2.count());
		for (int i=0; i<vl.count(); ++i)
		{
			S_EQUAL(vl.vcfLine(i).pos(),vl2.vcfLine(i).pos());
			I_EQUAL(vl.vcfLine(i).alt().size(), vl2.vcfLine(i).alt().size())
			for(int alt_id = 0; alt_id < vl.vcfLine(i).alt().count(); ++alt_id)
			{
				S_EQUAL(vl.vcfLine(i).alt(alt_id) ,vl2.vcfLine(i).alt(alt_id));
			}
		}
	}

	void removeDuplicates_Empty()
	{
		VcfFileHandler vl;
		vl.removeDuplicates(true);
	}

	void loadFromVCF()
	{
		VcfFileHandler vl;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		//old test expected 3, now two bcs we seperately parse the fileformat
		I_EQUAL(vl.vcfHeader().comments().count(), 2);
		S_EQUAL(vl.sampleIDs().at(0), QString("./Sample_GS120297A3/GS120297A3.bam"));
		//old test checked for annotations().count()==27, with annotations consisting of all formats, informations, id, qual, and filter
		I_EQUAL(vl.informationIDs().count(), 18);
		I_EQUAL(vl.formatIDs().count(), 6);

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

		I_EQUAL(vl.filterIDs().count(), 2);
		S_EQUAL(vl.vcfHeader().filterLineByID("q10").description, QString("Quality below 10"));
		S_EQUAL(vl.vcfHeader().filterLineByID("s50").description, QString("Less than 50% of samples have data"));

		X_EQUAL(vl.vcfLine(0).chr(), Chromosome("chr17"));
		I_EQUAL(vl.vcfLine(0).pos(), 72196817);
		I_EQUAL(vl.vcfLine(0).end(), 72196817);
		S_EQUAL(vl.vcfLine(0).ref(), Sequence("G"));
		S_EQUAL(vl.vcfLine(0).alt(0), Sequence("GA"));
		I_EQUAL(vl.vcfLine(0).alt().count(), 1);
		S_EQUAL(vl.vcfLine(0).infos().at(0).value(), QByteArray("TRUE"));
		S_EQUAL(vl.vcfLine(0).infos().at(5).value(), QByteArray("4,3,11,11"));
		QByteArray first_sample_name = vl.sampleIDs().at(0);
		QByteArray second_format_name = vl.vcfLine(0).format().at(1);
		S_EQUAL(vl.vcfLine(0).samples()[first_sample_name][second_format_name], QByteArray("255,0,123"));
		I_EQUAL(vl.vcfLine(0).filter().count(), 0);

		I_EQUAL(vl.vcfLine(11).filter().count(), 1);
		S_EQUAL(vl.vcfLine(11).filter().at(0), QByteArray("low_DP"));

		X_EQUAL(vl.vcfLine(12).chr(), Chromosome("chr9"));
		I_EQUAL(vl.vcfLine(12).pos(), 130931421);
		I_EQUAL(vl.vcfLine(12).end(), 130931421);
		S_EQUAL(vl.vcfLine(12).ref(), Sequence("G"));
		S_EQUAL(vl.vcfLine(12).alt(0), Sequence("A"));
		I_EQUAL(vl.vcfLine(12).alt().count(), 1);
		S_EQUAL(vl.vcfLine(12).infos().at(0).value(), QByteArray("2512"));
		S_EQUAL(vl.vcfLine(12).info("INDEL"), QByteArray(""));
		S_EQUAL(vl.vcfLine(12).infos().at(4).value(), QByteArray("457,473,752,757"));
		S_EQUAL(vl.vcfLine(12).info("DP4"), QByteArray("457,473,752,757"));
		first_sample_name = vl.sampleIDs().at(0);
		second_format_name = vl.vcfLine(12).format().at(1);
		S_EQUAL(vl.vcfLine(12).samples()[first_sample_name][second_format_name], QByteArray("255,0,255"));
		I_EQUAL(vl.vcfLine(12).filter().count(), 0);

		//load a second time to check initialization
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.vcfHeader().comments().count(), 2);
		S_EQUAL(vl.sampleIDs()[0], QString("./Sample_GS120297A3/GS120297A3.bam"));
		I_EQUAL(vl.informationIDs().count(), 18);
		I_EQUAL(vl.formatIDs().count(), 6);
	}

	void loadFromVCF_withROI()
	{
		BedFile roi;
		roi.append(BedLine("chr17", 72196820, 72196892));
		roi.append(BedLine("chr18", 67904549, 67904670));

		VcfFileHandler vl;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"), &roi);
		vl.checkValid();
		I_EQUAL(vl.count(), 4);
		I_EQUAL(vl.vcfHeader().comments().count(), 2);
		S_EQUAL(vl.sampleIDs().at(0), QString("./Sample_GS120297A3/GS120297A3.bam"));
		I_EQUAL(vl.informationIDs().count(), 18);
		I_EQUAL(vl.formatIDs().count(), 6);

		X_EQUAL(vl.vcfLine(0).chr(), Chromosome("chr17"));
		I_EQUAL(vl.vcfLine(0).pos(), 72196887);
		X_EQUAL(vl.vcfLine(1).chr(), Chromosome("chr17"));
		I_EQUAL(vl.vcfLine(1).pos(), 72196892);
		X_EQUAL(vl.vcfLine(2).chr(), Chromosome("chr18"));
		I_EQUAL(vl.vcfLine(2).pos(), 67904549);
		X_EQUAL(vl.vcfLine(3).chr(), Chromosome("chr18"));
		I_EQUAL(vl.vcfLine(3).pos(), 67904586);
	}

	void loadFromVCF_noSampleOrFormatColumn()
	{
		VcfFileHandler vl;

		vl.load(TESTDATA("data_in/VariantList_loadFromVCF_noSample.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.informationIDs().count(), 18);
		I_EQUAL(vl.formatIDs().count(), 6);
		I_EQUAL(vl.vcfHeader().comments().count(), 1);
		S_EQUAL(vl.vcfHeader().fileFormat(), QByteArray("VCFv4.1"));
		S_EQUAL(vl.sampleIDs().at(0), QString("Sample"));

		vl.load(TESTDATA("data_in/VariantList_loadFromVCF_noFormatSample.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.informationIDs().count(), 18);
		I_EQUAL(vl.formatIDs().count(), 6);
		I_EQUAL(vl.vcfHeader().comments().count(), 1);
		S_EQUAL(vl.vcfHeader().fileFormat(), QByteArray("VCFv4.1"));
		I_EQUAL(vl.sampleIDs().count(), 1);
		S_EQUAL(vl.sampleIDs().at(0), QString("Sample"));
	}

	void loadFromVCF_undeclaredAnnotations()
	{
		VcfFileHandler vl;

		//check annotation list
		vl.load(TESTDATA("data_in/VariantList_loadFromVCF_undeclaredAnnotations.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 2);
		I_EQUAL(vl.informationIDs().count(), 5);
		I_EQUAL(vl.formatIDs().count(), 10);
		QStringList names;
		foreach(QByteArray id, vl.informationIDs())
		{
			names << id;
		}
		foreach(QByteArray id, vl.formatIDs())
		{
			names << id;
		}
		S_EQUAL(names.join(","), QString("DP,AF,RO,AO,CIGAR,GT,GQ,GL,DP,RO,QR,AO,QA,TRIO,TRIO2"));

		//check variants
		S_EQUAL(vl.vcfLine(0).infos().at(4).value(), QByteArray("1X"));
		S_EQUAL(vl.vcfLine(0).info("CIGAR"), QByteArray("1X"));
		S_EQUAL(vl.vcfLine(1).info("CIGAR"), QByteArray(""));
		S_EQUAL(vl.vcfLine(0).sample(0, "TRIO2"), QByteArray(""));
		S_EQUAL(vl.vcfLine(1).sample(0, "TRIO2"), QByteArray("HET,9,0.56,WT,17,0.00,HOM,19,1.00"));
	}

	void loadFromVCF_emptyFormatAndInfo()
	{
		QString in = TESTDATA("data_in/VariantList_loadFromVCF_emptyInfoAndFormat.vcf");
		QString out = "out/VariantList_loadFromVCF_emptyInfoAndFormat.vcf";

		VcfFileHandler vl;
		vl.load(in);
		vl.checkValid();
		vl.store(out);

		COMPARE_FILES(in,out);
	}

	void storeToVCF()
	{
		//store loaded file
		VcfFileHandler vl;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl.checkValid();
		vl.store("out/VariantList_store_01.vcf");
		VCF_IS_VALID("out/VariantList_store_01.vcf")

		//reload and check that everything stayed the same
		vl.load("out/VariantList_store_01.vcf");
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		//old test expected 3, now two bcs we seperately parse the fileformat
		I_EQUAL(vl.vcfHeader().comments().count(), 2);
		S_EQUAL(vl.sampleIDs().at(0), QString("./Sample_GS120297A3/GS120297A3.bam"));
		//old test checked for annotations().count()==27, with annotations consisting of all formats, informations, id, qual, and filter
		I_EQUAL(vl.informationIDs().count(), 18);
		I_EQUAL(vl.formatIDs().count(), 6);

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

		I_EQUAL(vl.filterIDs().count(), 2);
		S_EQUAL(vl.vcfHeader().filterLineByID("q10").description, QString("Quality below 10"));
		S_EQUAL(vl.vcfHeader().filterLineByID("s50").description, QString("Less than 50% of samples have data"));

		X_EQUAL(vl.vcfLine(0).chr(), Chromosome("chr17"));
		I_EQUAL(vl.vcfLine(0).pos(), 72196817);
		I_EQUAL(vl.vcfLine(0).end(), 72196817);
		S_EQUAL(vl.vcfLine(0).ref(), Sequence("G"));
		S_EQUAL(vl.vcfLine(0).alt(0), Sequence("GA"));
		I_EQUAL(vl.vcfLine(0).alt().count(), 1);
		S_EQUAL(vl.vcfLine(0).infos().at(0).value(), QByteArray("TRUE"));
		S_EQUAL(vl.vcfLine(0).infos().at(5).value(), QByteArray("4,3,11,11"));
		QByteArray first_sample_name = vl.sampleIDs().at(0);
		QByteArray second_format_name = vl.vcfLine(0).format().at(1);
		S_EQUAL(vl.vcfLine(0).samples()[first_sample_name][second_format_name], QByteArray("255,0,123"));
		I_EQUAL(vl.vcfLine(0).filter().count(), 0);

		I_EQUAL(vl.vcfLine(11).filter().count(), 1);
		S_EQUAL(vl.vcfLine(11).filter().at(0), QByteArray("low_DP"));

		X_EQUAL(vl.vcfLine(12).chr(), Chromosome("chr9"));
		I_EQUAL(vl.vcfLine(12).pos(), 130931421);
		I_EQUAL(vl.vcfLine(12).end(), 130931421);
		S_EQUAL(vl.vcfLine(12).ref(), Sequence("G"));
		S_EQUAL(vl.vcfLine(12).alt(0), Sequence("A"));
		I_EQUAL(vl.vcfLine(12).alt().count(), 1);
		S_EQUAL(vl.vcfLine(12).infos().at(0).value(), QByteArray("2512"));
		S_EQUAL(vl.vcfLine(12).info("INDEL"), QByteArray(""));
		S_EQUAL(vl.vcfLine(12).infos().at(4).value(), QByteArray("457,473,752,757"));
		S_EQUAL(vl.vcfLine(12).info("DP4"), QByteArray("457,473,752,757"));
		first_sample_name = vl.sampleIDs().at(0);
		second_format_name = vl.vcfLine(12).format().at(1);
		S_EQUAL(vl.vcfLine(12).samples()[first_sample_name][second_format_name], QByteArray("255,0,255"));
		I_EQUAL(vl.vcfLine(12).filter().count(), 0);
	}

	void checkThatEmptyVariantAnnotationsAreFilled()
	{
		//store loaded vcf file
		VcfFileHandler vl;
		vl.load(TESTDATA("data_in/VariantList_emptyDescriptions.vcf"));
		vl.checkValid();
		vl.store("out/VariantList_emptyDescriptions_fixed.vcf");
		VCF_IS_VALID("out/VariantList_emptyDescriptions_fixed.vcf")

		VcfFileHandler vl2;
		vl2.load("out/VariantList_emptyDescriptions_fixed.vcf");
		vl2.checkValid();
		I_EQUAL(vl2.count(), 14);
		I_EQUAL(vl.informationIDs().count(), 18);
		I_EQUAL(vl.formatIDs().count(), 6);
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
		VcfFileHandler vl;
		vl.load(TESTDATA("data_in/VariantList_load_zipped.vcf.gz"));
		vl.checkValid();
		I_EQUAL(vl.count(), 157);
		I_EQUAL(vl.informationIDs().count(), 64);
		I_EQUAL(vl.formatIDs().count(), 8);
		S_EQUAL(vl.informationIDs().at(0), "NS");
		S_EQUAL(vl.informationIDs().at(63), "EXAC_AF");

		X_EQUAL(vl.vcfLine(0).chr().str(), "chr1");
		I_EQUAL(vl.vcfLine(0).pos(), 27687466);
		I_EQUAL(vl.vcfLine(0).end(), 27687466);
		S_EQUAL(vl.vcfLine(0).ref(), Sequence("G"));
		S_EQUAL(vl.vcfLine(0).alt(0), Sequence("T"));
		I_EQUAL(vl.vcfLine(0).alt().count(), 1);
		S_EQUAL(vl.vcfLine(0).id().at(0), "rs35659744");
		S_EQUAL(QString::number(vl.vcfLine(0).qual()), "11836.9");
		IS_TRUE(vl.vcfLine(0).filter().empty());
		S_EQUAL(vl.vcfLine(0).info("AC"), "1");
		S_EQUAL(vl.vcfLine(0).info("EXAC_AF"), "0.223");

		X_EQUAL(vl.vcfLine(156).chr().str(), "chr20");
		I_EQUAL(vl.vcfLine(156).pos(), 48301146);
		I_EQUAL(vl.vcfLine(156).end(), 48301146);
		S_EQUAL(vl.vcfLine(156).ref(), Sequence("G"));
		S_EQUAL(vl.vcfLine(156).alt(0), Sequence("A"));
		I_EQUAL(vl.vcfLine(156).alt().count(), 1);
		S_EQUAL(vl.vcfLine(156).id().at(0), "rs6512586");
		S_EQUAL(QString::number(vl.vcfLine(156).qual()), "39504.2");
		IS_TRUE(vl.vcfLine(156).filter().empty());
		S_EQUAL(vl.vcfLine(156).info("NS"), "1");
		S_EQUAL(vl.vcfLine(156).info("AC"), "2");
		S_EQUAL(vl.vcfLine(156).info("EXAC_AF"), "0.516");
	}

	void vepIndexByName()
	{
		VcfFileHandler vl;
		vl.load(TESTDATA("data_in/panel_vep.vcf"));
		I_EQUAL(vl.vcfHeader().vepIndexByName("Allele", false), 0);
		I_EQUAL(vl.vcfHeader().vepIndexByName("Consequence", false), 1);
		I_EQUAL(vl.vcfHeader().vepIndexByName("IMPACT", false), 2);
		I_EQUAL(vl.vcfHeader().vepIndexByName("HGMD_PHEN", false), 59);
		I_EQUAL(vl.vcfHeader().vepIndexByName("Oranguta-Klaus", false), -1);
	}

	void sort()
	{
		VcfFileHandler vl;
		vl.load(TESTDATA("data_in/sort_in.vcf"));
		vl.checkValid();
		vl.sort();
		vl.store("out/sort_out.vcf");
		COMPARE_FILES("out/sort_out.vcf",TESTDATA("data_out/sort_out.vcf"));
		VCF_IS_VALID("out/sort_out.vcf")
	}

	//test sort function for VCF files (with quality)
	void sort3()
	{
		VcfFileHandler vl;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl.checkValid();
		vl.sort(true);
		//entries should be sorted numerically
		X_EQUAL(vl.vcfLine(0).chr() ,Chromosome("chr1"));
		I_EQUAL(vl.vcfLine(0).pos(),11676308);
		I_EQUAL(vl.vcfLine(1).pos(),11676377);
		X_EQUAL(vl.vcfLine(2).chr(), Chromosome("chr2"));
		I_EQUAL(vl.vcfLine(2).pos(),139498511);
		X_EQUAL(vl.vcfLine(3).chr(), Chromosome("chr4"));
		I_EQUAL(vl.vcfLine(3).pos(),68247038);
		I_EQUAL(vl.vcfLine(4).pos(),68247113);
		X_EQUAL(vl.vcfLine(5).chr(), Chromosome("chr9"));
		I_EQUAL(vl.vcfLine(5).pos(),130931421);
		I_EQUAL(vl.vcfLine(6).pos(),130932396);
		X_EQUAL(vl.vcfLine(7).chr(), Chromosome("chr17"));
		I_EQUAL(vl.vcfLine(7).pos(),72196817);
		I_EQUAL(vl.vcfLine(8).pos(),72196887);
		I_EQUAL(vl.vcfLine(9).pos(),72196892);
		X_EQUAL(vl.vcfLine(10).chr(), Chromosome("chr18"));
		I_EQUAL(vl.vcfLine(10).pos(),67904549);
		I_EQUAL(vl.vcfLine(11).pos(),67904586);
		I_EQUAL(vl.vcfLine(12).pos(),67904672);
		X_EQUAL(vl.vcfLine(13).chr(), Chromosome("chr19"));
		I_EQUAL(vl.vcfLine(13).pos(),14466629);
	}

	//test sortByFile function for *.vcf-files
	void sortByFile()
	{
		VcfFileHandler vl;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl.checkValid();
		vl.sortByFile(TESTDATA("data_in/variantList_sortbyFile.fai"));
		vl.store("out/sortByFile.vcf");
		//entries should be sorted by variantList_sortbyFile.fai, which is reverse-numeric concerning chromosomes
		VCF_IS_VALID("out/sortByFile.vcf")
		X_EQUAL(vl.vcfLine(0).chr(),Chromosome("chr19"));
		I_EQUAL(vl.vcfLine(0).pos(),14466629);
		X_EQUAL(vl.vcfLine(1).chr(),Chromosome("chr18"));
		I_EQUAL(vl.vcfLine(1).pos(),67904549);
		I_EQUAL(vl.vcfLine(2).pos(),67904586);
		I_EQUAL(vl.vcfLine(3).pos(),67904672);
		X_EQUAL(vl.vcfLine(4).chr(),Chromosome("chr17"));
		I_EQUAL(vl.vcfLine(4).pos(),72196817);
		I_EQUAL(vl.vcfLine(5).pos(),72196887);
		I_EQUAL(vl.vcfLine(6).pos(),72196892);
		X_EQUAL(vl.vcfLine(7).chr(),Chromosome("chr9"));
		I_EQUAL(vl.vcfLine(7).pos(),130931421);
		I_EQUAL(vl.vcfLine(8).pos(),130932396);
		X_EQUAL(vl.vcfLine(9).chr(),Chromosome("chr4"));
		I_EQUAL(vl.vcfLine(9).pos(),68247038);
		I_EQUAL(vl.vcfLine(10).pos(),68247113);
		X_EQUAL(vl.vcfLine(11).chr(),Chromosome("chr2"));
		I_EQUAL(vl.vcfLine(11).pos(),139498511);
		X_EQUAL(vl.vcfLine(12).chr() ,Chromosome("chr1"));
		I_EQUAL(vl.vcfLine(12).pos(),11676308);
		I_EQUAL(vl.vcfLine(13).pos(),11676377);
	}


	void sortCustom()
	{
		VcfFileHandler vl;
		vl.checkValid();
		vl.load(TESTDATA("data_in/sort_in.vcf"));
		vl.sortCustom([](const VCFLine& a, const VCFLine& b) {return a.pos() < b.pos(); });

		I_EQUAL(vl.count(), 2344);
		X_EQUAL(vl.vcfLine(0).chr(),Chromosome("chr4"));
		I_EQUAL(vl.vcfLine(0).pos(),85995);
		X_EQUAL(vl.vcfLine(1).chr(),Chromosome("chr4"));
		I_EQUAL(vl.vcfLine(1).pos(),85997);
		X_EQUAL(vl.vcfLine(2).chr(),Chromosome("chr4"));
		I_EQUAL(vl.vcfLine(2).pos(),86101);
		X_EQUAL(vl.vcfLine(3).chr(),Chromosome("chr4"));
		I_EQUAL(vl.vcfLine(3).pos(),86102);
		X_EQUAL(vl.vcfLine(4).chr(),Chromosome("chr4"));
		I_EQUAL(vl.vcfLine(4).pos(),87313);
		X_EQUAL(vl.vcfLine(5).chr(),Chromosome("chr20"));
		I_EQUAL(vl.vcfLine(5).pos(),126309);
		X_EQUAL(vl.vcfLine(6).chr(),Chromosome("chr20"));
		I_EQUAL(vl.vcfLine(6).pos(),126310);
		//...
		X_EQUAL(vl.vcfLine(2343).chr(),Chromosome("chr1"));
		I_EQUAL(vl.vcfLine(2343).pos(),248802249);
	}

	void storeLine()
	{
		VcfFileHandler vl;
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));

		QString first_line = vl.lineToString(0);
		S_EQUAL(first_line, "chr17	72196817	.	G	GA	217	.	INDEL;DP=31;VDB=0.0000;AF1=0.5;AC1=1;DP4=4,3,11,11;MQ=42;FQ=88.5;PV4=1,1,0.17,0.28	GT:PL:GQ	0/1:255,0,123:99");
		QString seventh_line = vl.lineToString(6);
		S_EQUAL(seventh_line, "chr19	14466629	.	A	AA	70.4	.	INDEL;DP=4;VDB=0.0001;AF1=1;AC1=2;DP4=0,0,1,2;MQ=50;FQ=-43.5	GT:PL:GQ	1/1:110,9,0:16");
	}

	void convertVCFtoTSV()
	{
		//store loaded vcf file
		VcfFormat::VcfFileHandler vl_vcf;
		vl_vcf.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl_vcf.checkValid();
		vl_vcf.store("out/VariantList_convertVCFtoTSV.tsv", TSV);

		//reload and check that no information became incorrect (vcf-specific things like annotation dimensions and types are still lost)
		VariantList vl_tsv;
		vl_tsv.load("out/VariantList_convertVCFtoTSV.tsv");
		vl_tsv.checkValid();
		I_EQUAL(vl_tsv.count(), 14);
		I_EQUAL(vl_tsv.annotations().count(), 27);
		I_EQUAL(vl_tsv.comments().count(), 2);
		S_EQUAL(vl_tsv.annotations()[0].name(), QString("ID"));
		S_EQUAL(vl_tsv.annotationDescriptionByName("ID").description(), QString("ID of the variant, often dbSNP rsnumber"));
		S_EQUAL(vl_tsv.annotationDescriptionByName("INDEL").name(), QString("INDEL"));
		S_EQUAL(vl_tsv.annotationDescriptionByName("INDEL").description(), QString("Indicates that the variant is an INDEL."));
		S_EQUAL(vl_tsv.annotationDescriptionByName("DP4").name(), QString("DP4"));
		S_EQUAL(vl_tsv.annotationDescriptionByName("DP4").description(), QString("# high-quality ref-forward bases, ref-reverse, alt-forward and alt-reverse bases"));
		S_EQUAL(vl_tsv.annotationDescriptionByName("PL_ss").name(), QString("PL_ss"));
		S_EQUAL(vl_tsv.annotationDescriptionByName("PL_ss").description(), QString("List of Phred-scaled genotype likelihoods"));


		X_EQUAL(vl_tsv[0].chr(), Chromosome("chr17"));
		I_EQUAL(vl_tsv[0].start(), 72196817);
		I_EQUAL(vl_tsv[0].end(), 72196817);
		S_EQUAL(vl_tsv[0].ref(), Sequence("G"));
		S_EQUAL(vl_tsv[0].obs(), Sequence("GA"));
		S_EQUAL(vl_tsv[0].annotations().at(3), QByteArray("TRUE"));
		S_EQUAL(vl_tsv[0].annotations().at(8), QByteArray("4,3,11,11"));
		S_EQUAL(vl_tsv[0].annotations().at(26), QByteArray("255,0,123"));

		X_EQUAL(vl_tsv[12].chr(), Chromosome("chr9"));
		I_EQUAL(vl_tsv[12].start(), 130931421);
		I_EQUAL(vl_tsv[12].end(), 130931421);
		S_EQUAL(vl_tsv[12].ref(), Sequence("G"));
		S_EQUAL(vl_tsv[12].obs(), Sequence("A"));
		S_EQUAL(vl_tsv[12].annotations().at(3), QByteArray(""));
		S_EQUAL(vl_tsv[12].annotations().at(8), QByteArray("457,473,752,757"));
		S_EQUAL(vl_tsv[12].annotations().at(26), QByteArray("255,0,255"));
	}
};
