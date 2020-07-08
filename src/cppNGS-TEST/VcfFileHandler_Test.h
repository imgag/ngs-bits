#include "TestFramework.h"
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
		I_EQUAL(vl.type(false), GERMLINE_SINGLESAMPLE);
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
		I_EQUAL(vl.vcfHeader().file_comments_.count(), 2);
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
		I_EQUAL(vl.vcfLine(0).pos() + vl.vcfLine(0).ref().length() - 1, 72196817);
		S_EQUAL(vl.vcfLine(0).ref(), Sequence("G"));
		S_EQUAL(vl.vcfLine(0).alt(0), Sequence("GA"));
		S_EQUAL(vl.vcfLine(0).infos().at(0).second, QByteArray("TRUE"));
		S_EQUAL(vl.vcfLine(0).infos().at(5).second, QByteArray("4,3,11,11"));
		QByteArray first_sample_name = vl.sampleIDs().at(0);
		QByteArray second_format_name = vl.vcfLine(0).format().at(1);
		S_EQUAL(vl.vcfLine(0).samples()[first_sample_name][second_format_name], QByteArray("255,0,123"));
		I_EQUAL(vl.vcfLine(0).filter().count(), 0);

		I_EQUAL(vl.vcfLine(11).filter().count(), 1);
		S_EQUAL(vl.vcfLine(11).filter().at(0), QByteArray("low_DP"));

		X_EQUAL(vl.vcfLine(12).chr(), Chromosome("chr9"));
		I_EQUAL(vl.vcfLine(12).pos(), 130931421);
		I_EQUAL(vl.vcfLine(12).pos() + vl.vcfLine(0).ref().length() - 1, 130931421);
		S_EQUAL(vl.vcfLine(12).ref(), Sequence("G"));
		S_EQUAL(vl.vcfLine(12).alt(0), Sequence("A"));
		S_EQUAL(vl.vcfLine(12).infos().at(0).second, QByteArray("2512"));
		S_EQUAL(vl.vcfLine(12).info("INDEL"), QByteArray(""));
		S_EQUAL(vl.vcfLine(12).infos().at(4).second, QByteArray("457,473,752,757"));
		S_EQUAL(vl.vcfLine(12).info("DP4"), QByteArray("457,473,752,757"));
		first_sample_name = vl.sampleIDs().at(0);
		second_format_name = vl.vcfLine(12).format().at(1);
		S_EQUAL(vl.vcfLine(12).samples()[first_sample_name][second_format_name], QByteArray("255,0,255"));
		I_EQUAL(vl.vcfLine(12).filter().count(), 0);

		//load a second time to check initialization
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.vcfHeader().file_comments_.count(), 2);
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
		I_EQUAL(vl.vcfHeader().file_comments_.count(), 2);
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
		I_EQUAL(vl.vcfHeader().file_comments_.count(), 1);
		S_EQUAL(vl.vcfHeader().fileformat_, QByteArray("VCFv4.1"));
		S_EQUAL(vl.sampleIDs().at(0), QString("Sample"));

		vl.load(TESTDATA("data_in/VariantList_loadFromVCF_noFormatSample.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.informationIDs().count(), 18);
		I_EQUAL(vl.formatIDs().count(), 6);
		I_EQUAL(vl.vcfHeader().file_comments_.count(), 1);
		S_EQUAL(vl.vcfHeader().fileformat_, QByteArray("VCFv4.1"));
		I_EQUAL(vl.sampleIDs().count(), 0);
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
		S_EQUAL(vl.vcfLine(0).infos().at(4).second, QByteArray("1X"));
		S_EQUAL(vl.vcfLine(0).info("CIGAR"), QByteArray("1X"));
		S_EQUAL(vl.vcfLine(1).info("CIGAR"), QByteArray(""));
		S_EQUAL(vl.vcfLine(0).sample(0, "TRIO2"), QByteArray(""));
		S_EQUAL(vl.vcfLine(1).sample(0, "TRIO2"), QByteArray("HET,9,0.56,WT,17,0.00,HOM,19,1.00"));
	}

};
