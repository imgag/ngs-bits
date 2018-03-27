#include "TestFramework.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"
#include "Settings.h"

int countSequencesContaining(QVector<Sequence> sequences, char c)
{
	int output = 0;
	foreach(const Sequence& sequence, sequences)
	{
		output += sequence.contains(c);
	}
	return output;
}

TEST_CLASS(NGSHelper_Test)
{
Q_OBJECT
private slots:

	void getPileup()
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		Pileup pileup;
		//SNP
		pileup = NGSHelper::getPileup(reader, "chr1", 12062205, 1);
		I_EQUAL(pileup.depth(false), 117);
		F_EQUAL2(pileup.frequency('A', 'G'), 0.4102, 0.001);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = NGSHelper::getPileup(reader, "chr1", 12062181, 1);
		I_EQUAL(pileup.depth(false), 167);
		F_EQUAL2(pileup.frequency('A', 'G'), 0.0, 0.001);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = NGSHelper::getPileup(reader, "1", 12062181, 1);
		I_EQUAL(pileup.depth(false), 167);
		F_EQUAL2(pileup.frequency('G', 'A'), 1.0, 0.001);
		I_EQUAL(pileup.indels().count(), 0);
		//SNP
		pileup = NGSHelper::getPileup(reader, "1", 12062180, 1);
		I_EQUAL(pileup.depth(false), 167);
		IS_TRUE(!BasicStatistics::isValidFloat(pileup.frequency('A', 'T')));
		I_EQUAL(pileup.indels().count(), 0);
		//INSERTATION
		pileup = NGSHelper::getPileup(reader, "chr6", 110053825, 1);
		I_EQUAL(pileup.depth(false), 40);
		I_EQUAL(pileup.t(), 40);
		I_EQUAL(pileup.indels().count(), 29);
		I_EQUAL(countSequencesContaining(pileup.indels(), '+'), 27);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 2);
		//DELATION
		pileup = NGSHelper::getPileup(reader, "chr14", 53513479, 1);
		I_EQUAL(pileup.depth(false), 50);
		I_EQUAL(pileup.a(), 50);
		I_EQUAL(pileup.indels().count(), 14);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 14);
		//INSERTATION -  with window
		pileup = NGSHelper::getPileup(reader, "chr6", 110053825, 20);
		I_EQUAL(pileup.depth(false), 40);
		I_EQUAL(pileup.t(), 40);
		I_EQUAL(pileup.indels().count(), 30);
		I_EQUAL(countSequencesContaining(pileup.indels(), '+'), 28);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 2);
		//DELETION -  with window
		pileup = NGSHelper::getPileup(reader, "chr14", 53513479, 10);
		I_EQUAL(pileup.depth(false), 50);
		I_EQUAL(pileup.a(), 50);
		I_EQUAL(pileup.indels().count(), 14);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 14);
	}

	//special test with RNA because it contains the CIGAR operations S and N
	void getPileup2()
	{
		BamReader reader(TESTDATA("data_in/rna.bam"));
		Pileup pileup;
		//SNP
		pileup = NGSHelper::getPileup(reader, "chr10", 90974727);
		I_EQUAL(pileup.depth(true), 132);
		F_EQUAL2(pileup.frequency('A', 'C'), 0.4621, 0.001);
		I_EQUAL(pileup.indels().count(), 0);
		//DELETION
		pileup = NGSHelper::getPileup(reader, "chr10", 92675287, 10);
		I_EQUAL(pileup.depth(true), 23);
		I_EQUAL(pileup.indels().count(), 23);
		I_EQUAL(countSequencesContaining(pileup.indels(), '+'), 0);
		I_EQUAL(countSequencesContaining(pileup.indels(), '-'), 23);
		//NO COVERAGE
		pileup = NGSHelper::getPileup(reader, "chr11", 92675295, 10);
		I_EQUAL(pileup.depth(true), 0);
		I_EQUAL(pileup.indels().count(), 0);
	}

	void getPileups1()
	{
		//open BAM file
		BamReader reader(TESTDATA("data_in/panel.bam"));
		//get pileups
		QList<Pileup> pileups;
		NGSHelper::getPileups(pileups, reader, "chr6", 110053825, 110053825);
		I_EQUAL(pileups[0].depth(true, true), 42);
		I_EQUAL(pileups[0].t(), 40);
		I_EQUAL(pileups[0].indels().count(), 29);
		I_EQUAL(pileups[0].indels().count("-1"), 2);
		I_EQUAL(pileups[0].indels().count("+T"), 18);
		I_EQUAL(pileups[0].indels().count("+TT"), 9);
		I_EQUAL(pileups.count(), 1);
	}

	void getPileups2()
	{
		//open BAM file
		BamReader reader(TESTDATA("data_in/panel.bam"));
		//get pileups
		QList<Pileup> pileups;
		NGSHelper::getPileups(pileups, reader, "chr6", 110053825-5, 110053825+5);
		I_EQUAL(pileups[0].depth(true, true), 42);
		I_EQUAL(pileups[0].c(), 42);
		I_EQUAL(pileups[0].indels().count(), 0);
		I_EQUAL(pileups[1].depth(true, true), 42);
		I_EQUAL(pileups[1].t(), 42);
		I_EQUAL(pileups[1].indels().count(), 1);
		X_EQUAL(pileups[1].indels()[0], Sequence("+TT"));
		I_EQUAL(pileups[2].depth(true, true), 42);
		I_EQUAL(pileups[2].t(), 42);
		I_EQUAL(pileups[2].indels().count(), 0);
		I_EQUAL(pileups[3].depth(true, true), 42);
		I_EQUAL(pileups[3].t(), 42);
		I_EQUAL(pileups[3].indels().count(), 0);
		I_EQUAL(pileups[4].depth(true, true), 42);
		I_EQUAL(pileups[4].g(), 39);
		I_EQUAL(pileups[4].t(),3);
		I_EQUAL(pileups[4].indels().count(), 0);
		I_EQUAL(pileups[5].depth(true, true), 42);
		I_EQUAL(pileups[5].t(), 40);
		I_EQUAL(pileups[5].indels().count(), 29);
		I_EQUAL(pileups[5].indels().count("-1"), 2);
		I_EQUAL(pileups[5].indels().count("+T"), 18);
		I_EQUAL(pileups[5].indels().count("+TT"), 9);
		I_EQUAL(pileups[6].depth(true, true), 42);
		I_EQUAL(pileups[6].t(), 42);
		I_EQUAL(pileups[6].indels().count(), 0);
		I_EQUAL(pileups[7].depth(true, true), 42);
		I_EQUAL(pileups[7].t(), 42);
		I_EQUAL(pileups[7].indels().count(), 0);
		I_EQUAL(pileups[8].depth(true, true), 42);
		I_EQUAL(pileups[8].t(), 42);
		I_EQUAL(pileups[8].indels().count(), 0);
		I_EQUAL(pileups[9].depth(true, true), 42);
		I_EQUAL(pileups[9].t(), 42);
		I_EQUAL(pileups[9].indels().count(), 0);
		I_EQUAL(pileups[10].depth(true, true), 42);
		I_EQUAL(pileups[10].t(), 42);
		I_EQUAL(pileups[10].indels().count(), 0);
		I_EQUAL(pileups.count(), 11);
	}


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

	void getIndels()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		BamReader reader(TESTDATA("data_in/panel.bam"));
		QVector<Sequence> indels;
		int depth;
		double mapq0_frac;

		//inseration of TT
		NGSHelper::getIndels(reference, reader, "chr6", 110053825-20, 110053825+20, indels, depth, mapq0_frac);
		I_EQUAL(depth, 42);
		I_EQUAL(indels.count(), 30);
		I_EQUAL(indels.count("+TT"), 10);
		I_EQUAL(indels.count("+T"), 18);
		I_EQUAL(indels.count("-T"), 2);
		F_EQUAL2(mapq0_frac, 0.0, 0.001);

		//deletion of AG
		NGSHelper::getIndels(reference, reader, "chr14", 53513479-10, 53513480+10, indels, depth, mapq0_frac);
		I_EQUAL(depth, 64);
		I_EQUAL(indels.count(), 14);
		I_EQUAL(indels.count("-AG"), 14);
		F_EQUAL2(mapq0_frac, 0.0, 0.001);
	}

	void changeSeq()
	{
		S_EQUAL(NGSHelper::changeSeq("", true, true), QByteArray(""));

		S_EQUAL(NGSHelper::changeSeq("ACGT", false, false), QByteArray("ACGT"));
		S_EQUAL(NGSHelper::changeSeq("ACGTN", true, false), QByteArray("NTGCA"));
		S_EQUAL(NGSHelper::changeSeq("ACGTN", false, true), QByteArray("TGCAN"));
		S_EQUAL(NGSHelper::changeSeq("ACGTN", true, true), QByteArray("NACGT"));
	}

	void getDepthFreq()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		BamReader reader(TESTDATA("data_in/panel.bam"));

		//inseration T (left)
		Variant v("chr6", 110053825, 110053825, "-", "T");
		VariantDetails output = NGSHelper::getVariantDetails(reader, reference, v);
		I_EQUAL(output.depth, 42);
		F_EQUAL2(output.frequency, 0.428, 0.001);

		//inseration T (right)
		v = Variant("chr16", 89576894, 89576894, "-", "T");
		output = NGSHelper::getVariantDetails(reader, reference, v);
		I_EQUAL(output.depth, 126);
		F_EQUAL2(output.frequency, 0.126, 0.001);

		//deletion AG
		v = Variant("chr14", 53513479, 53513480, "AG", "-");
		output = NGSHelper::getVariantDetails(reader, reference, v);
		I_EQUAL(output.depth, 64);
		F_EQUAL2(output.frequency, 0.218, 0.001);

		//SNP A>G (het)
		v = Variant("chr4", 108868411, 108868411, "A", "G");
		output = NGSHelper::getVariantDetails(reader, reference, v);
		I_EQUAL(output.depth, 78);
		F_EQUAL2(output.frequency, 0.333, 0.001);

		//SNP C>T (hom)
		v = Variant("chr2", 202625615, 202625615, "C", "T");
		output = NGSHelper::getVariantDetails(reader, reference, v);
		I_EQUAL(output.depth, 166);
		F_EQUAL2(output.frequency, 1.0, 0.001);
	}

	void cigarDataAsString()
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al1;
		reader.getNextAlignment(al1);
		while(al1.isUnmapped())
		{
			reader.getNextAlignment(al1);
		}
		reader.getNextAlignment(al1);
		while(al1.isUnmapped())
		{
			reader.getNextAlignment(al1);
		}
		S_EQUAL(al1.cigarDataAsString(), "36M2D115M");
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
