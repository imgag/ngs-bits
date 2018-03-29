#include "TestFramework.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"
#include "Settings.h"

TEST_CLASS(NGSHelper_Test)
{
Q_OBJECT
private slots:

	void getKnownVariants()
	{
		VariantList list = NGSHelper::getKnownVariants(false);
		I_EQUAL(list.count(), 102467);

		//only SNPs
		list = NGSHelper::getKnownVariants(true);
		I_EQUAL(list.count(), 97469);

		//only SNPs, AF<80%
		list = NGSHelper::getKnownVariants(true, 0.0, 0.8);
		I_EQUAL(list.count(), 91185);

		//only SNPs, AF>20%
		list = NGSHelper::getKnownVariants(true, 0.2);
		I_EQUAL(list.count(), 36022);

		//only SNPs, AF>20%, AF<80%
		list = NGSHelper::getKnownVariants(true, 0.2, 0.8);
		I_EQUAL(list.count(), 29738);

		//only SNPs on chrX
		BedFile roi_chrx;
		roi_chrx.append(BedLine("chrX", 1, 155270560));
		list = NGSHelper::getKnownVariants(true, 0.0, 1.0, &roi_chrx);
		I_EQUAL(list.count(), 1948);
	}

	void changeSeq()
	{
		S_EQUAL(NGSHelper::changeSeq("", true, true), QByteArray(""));

		S_EQUAL(NGSHelper::changeSeq("ACGT", false, false), QByteArray("ACGT"));
		S_EQUAL(NGSHelper::changeSeq("ACGTN", true, false), QByteArray("NTGCA"));
		S_EQUAL(NGSHelper::changeSeq("ACGTN", false, true), QByteArray("TGCAN"));
		S_EQUAL(NGSHelper::changeSeq("ACGTN", true, true), QByteArray("NACGT"));
	}

	void softClipAlignment()
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al;

		//first soft-clip al
		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}

		NGSHelper::softClipAlignment(al,317587,317596);
		S_EQUAL(al.cigarDataAsString(), "10S123M13I5M");

		//second soft-clip same al
		NGSHelper::softClipAlignment(al,317597,317721);
		S_EQUAL(al.cigarDataAsString(), "148S3M");

		//next alignment
		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}

		//third soft-clip different al1
		NGSHelper::softClipAlignment(al,1048687,1048692);
		NGSHelper::softClipAlignment(al,1048540,1048576);
		S_EQUAL(al.cigarDataAsString(), "36S109M6S");
	}

	void softClipAlignment2()
	{
		BamReader reader(TESTDATA("data_in/bamclipoverlap.bam"));
		BamAlignment al;

		//first soft-clip al
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}

		S_EQUAL(al.name(), "PC0226:55:000000000-A5CV9:1:1101:2110:14905");
		NGSHelper::softClipAlignment(al,33038615,33038624);
		S_EQUAL(al.cigarDataAsString(), "5H10S141M");
		I_EQUAL(al.start(), 33038625);

		NGSHelper::softClipAlignment(al,33038756,33038765);
		S_EQUAL(al.cigarDataAsString(), "5H10S131M10S");

		reader.getNextAlignment(al);
		reader.getNextAlignment(al);

		S_EQUAL(al.name(), "PC0226:55:000000000-A5CV9:1:1101:2110:14905");
		NGSHelper::softClipAlignment(al,33038659,33038668);
		S_EQUAL(al.cigarDataAsString(), "10S141M5H");
		I_EQUAL(al.start(), 33038669);

		NGSHelper::softClipAlignment(al,33038800,33038809);
		S_EQUAL(al.cigarDataAsString(), "10S131M10S5H");
	}

	void getSampleHeader_singlesample_noheader()
	{
		QString input = TESTDATA("data_in/VariantFilter_in.GSvar");
		VariantList vl;
		vl.load(input);
		SampleHeaderInfo info = NGSHelper::getSampleHeader(vl, input);
		I_EQUAL(info.count(), 1);
		I_EQUAL(info.sampleColumns().count(), 1);
		S_EQUAL(info.sampleColumns()[0], "genotype");
		I_EQUAL(info.sampleColumns(true).count(), 1);
		S_EQUAL(info.sampleColumns(true)[0], "genotype");
		I_EQUAL(info.sampleColumns(false).count(), 0);
	}

	void getSampleHeader_multisample()
	{
		QString input = TESTDATA("data_in/VariantFilter_in_multi.GSvar");
		VariantList vl;
		vl.load(input);
		SampleHeaderInfo info = NGSHelper::getSampleHeader(vl, input);
		I_EQUAL(info.count(), 3);
		I_EQUAL(info.sampleColumns().count(), 3);
		S_EQUAL(info.sampleColumns()[0], "Affected1");
		S_EQUAL(info.sampleColumns()[1], "Affected2");
		S_EQUAL(info.sampleColumns()[2], "Control");
		I_EQUAL(info.sampleColumns(true).count(), 2);
		S_EQUAL(info.sampleColumns(true)[0], "Affected1");
		S_EQUAL(info.sampleColumns(true)[1], "Affected2");
		I_EQUAL(info.sampleColumns(false).count(), 1);
		S_EQUAL(info.sampleColumns(false)[0], "Control");
	}

};
