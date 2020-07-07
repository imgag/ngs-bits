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
		S_EQUAL(vl.sampleIDs()[0], QString("./Sample_GS120297A3/GS120297A3.bam"));
		//old test checked for annotations().count()==27, with annotations consisting of all formats, informations, id, qual, and filter
		I_EQUAL(vl.informationIDs().count(), 18);
		I_EQUAL(vl.formatIDs().count(), 6);
/*
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
		vl.load(TESTDATA("data_in/panel_snpeff.vcf"));
		vl.checkValid();
		I_EQUAL(vl.count(), 14);
		I_EQUAL(vl.annotations().count(), 27);
		I_EQUAL(vl.comments().count(), 3);
		S_EQUAL(vl.sampleNames()[0], QString("./Sample_GS120297A3/GS120297A3.bam"));
		*/
	}

};
