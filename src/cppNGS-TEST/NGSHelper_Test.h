#include "TestFramework.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"
#include "Settings.h"

#include "api/BamReader.h"
using namespace BamTools;

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
		BamReader reader;
		NGSHelper::openBAM(reader, QFINDTESTDATA("data_in/panel.bam"));
		Pileup pileup;
		//SNP
		pileup = NGSHelper::getPileup(reader, "chr1", 12062205, 1);
		QCOMPARE(pileup.depth(false), 117);
		TFW::compareDouble(pileup.frequency('A', 'G'), 0.4102, 0.001);
		QCOMPARE(pileup.indels().count(), 0);
		//SNP
		pileup = NGSHelper::getPileup(reader, "chr1", 12062181, 1);
		QCOMPARE(pileup.depth(false), 167);
		TFW::compareDouble(pileup.frequency('A', 'G'), 0.0, 0.001);
		QCOMPARE(pileup.indels().count(), 0);
		//SNP
		pileup = NGSHelper::getPileup(reader, "1", 12062181, 1);
		QCOMPARE(pileup.depth(false), 167);
		TFW::compareDouble(pileup.frequency('G', 'A'), 1.0, 0.001);
		QCOMPARE(pileup.indels().count(), 0);
		//SNP
		pileup = NGSHelper::getPileup(reader, "1", 12062180, 1);
		QCOMPARE(pileup.depth(false), 167);
		QVERIFY(!BasicStatistics::isValidFloat(pileup.frequency('A', 'T')));
		QCOMPARE(pileup.indels().count(), 0);
		//INSERTATION
		pileup = NGSHelper::getPileup(reader, "chr6", 110053825, 1);
		QCOMPARE(pileup.depth(false), 40);
		QCOMPARE(pileup.t(), 40);
		QCOMPARE(pileup.indels().count(), 29);
		QCOMPARE(countSequencesContaining(pileup.indels(), '+'), 27);
		QCOMPARE(countSequencesContaining(pileup.indels(), '-'), 2);
		//DELATION
		pileup = NGSHelper::getPileup(reader, "chr14", 53513479, 1);
		QCOMPARE(pileup.depth(false), 50);
		QCOMPARE(pileup.a(), 50);
		QCOMPARE(pileup.indels().count(), 14);
		QCOMPARE(countSequencesContaining(pileup.indels(), '-'), 14);
		//INSERTATION -  with window
		pileup = NGSHelper::getPileup(reader, "chr6", 110053825, 20);
		QCOMPARE(pileup.depth(false), 40);
		QCOMPARE(pileup.t(), 40);
		QCOMPARE(pileup.indels().count(), 30);
		QCOMPARE(countSequencesContaining(pileup.indels(), '+'), 28);
		QCOMPARE(countSequencesContaining(pileup.indels(), '-'), 2);
		//DELETION -  with window
		pileup = NGSHelper::getPileup(reader, "chr14", 53513479, 10);
		QCOMPARE(pileup.depth(false), 50);
		QCOMPARE(pileup.a(), 50);
		QCOMPARE(pileup.indels().count(), 14);
		QCOMPARE(countSequencesContaining(pileup.indels(), '-'), 14);
	}

	//special test with RNA because it contains the CIGAR operations S and N
	void getPileup2()
	{
		BamReader reader;
		NGSHelper::openBAM(reader, QFINDTESTDATA("data_in/rna.bam"));
		Pileup pileup;
		//SNP
		pileup = NGSHelper::getPileup(reader, "chr10", 90974727);
		QCOMPARE(pileup.depth(true), 132);
		TFW::compareDouble(pileup.frequency('A', 'C'), 0.4621, 0.001);
		QCOMPARE(pileup.indels().count(), 0);
		//DELETION
		pileup = NGSHelper::getPileup(reader, "chr10", 92675287, 10);
		QCOMPARE(pileup.depth(true), 23);
		QCOMPARE(pileup.indels().count(), 23);
		QCOMPARE(countSequencesContaining(pileup.indels(), '+'), 0);
		QCOMPARE(countSequencesContaining(pileup.indels(), '-'), 23);
		//NO COVERAGE
		pileup = NGSHelper::getPileup(reader, "chr11", 92675295, 10);
		QCOMPARE(pileup.depth(true), 0);
		QCOMPARE(pileup.indels().count(), 0);
	}

	void getPileups1()
	{
		//open BAM file
		BamReader reader;
		NGSHelper::openBAM(reader, QFINDTESTDATA("data_in/panel.bam"));
		//get pileups
		QList<Pileup> pileups;
		NGSHelper::getPileups(pileups, reader, "chr6", 110053825, 110053825);
		QCOMPARE(pileups[0].depth(true, true), 42);
		QCOMPARE(pileups[0].t(), 40);
		QCOMPARE(pileups[0].indels().count(), 29);
		QCOMPARE(pileups[0].indels().count("-1"), 2);
		QCOMPARE(pileups[0].indels().count("+T"), 18);
		QCOMPARE(pileups[0].indels().count("+TT"), 9);
		QCOMPARE(pileups.count(), 1);
	}

	void getPileups2()
	{
		//open BAM file
		BamReader reader;
		NGSHelper::openBAM(reader, QFINDTESTDATA("data_in/panel.bam"));
		//get pileups
		QList<Pileup> pileups;
		NGSHelper::getPileups(pileups, reader, "chr6", 110053825-5, 110053825+5);
		QCOMPARE(pileups[0].depth(true, true), 42);
		QCOMPARE(pileups[0].c(), 42);
		QCOMPARE(pileups[0].indels().count(), 0);
		QCOMPARE(pileups[1].depth(true, true), 42);
		QCOMPARE(pileups[1].t(), 42);
		QCOMPARE(pileups[1].indels().count(), 1);
		QCOMPARE(pileups[1].indels()[0], Sequence("+TT"));
		QCOMPARE(pileups[2].depth(true, true), 42);
		QCOMPARE(pileups[2].t(), 42);
		QCOMPARE(pileups[2].indels().count(), 0);
		QCOMPARE(pileups[3].depth(true, true), 42);
		QCOMPARE(pileups[3].t(), 42);
		QCOMPARE(pileups[3].indels().count(), 0);
		QCOMPARE(pileups[4].depth(true, true), 42);
		QCOMPARE(pileups[4].g(), 39);
		QCOMPARE(pileups[4].t(),3);
		QCOMPARE(pileups[4].indels().count(), 0);
		QCOMPARE(pileups[5].depth(true, true), 42);
		QCOMPARE(pileups[5].t(), 40);
		QCOMPARE(pileups[5].indels().count(), 29);
		QCOMPARE(pileups[5].indels().count("-1"), 2);
		QCOMPARE(pileups[5].indels().count("+T"), 18);
		QCOMPARE(pileups[5].indels().count("+TT"), 9);
		QCOMPARE(pileups[6].depth(true, true), 42);
		QCOMPARE(pileups[6].t(), 42);
		QCOMPARE(pileups[6].indels().count(), 0);
		QCOMPARE(pileups[7].depth(true, true), 42);
		QCOMPARE(pileups[7].t(), 42);
		QCOMPARE(pileups[7].indels().count(), 0);
		QCOMPARE(pileups[8].depth(true, true), 42);
		QCOMPARE(pileups[8].t(), 42);
		QCOMPARE(pileups[8].indels().count(), 0);
		QCOMPARE(pileups[9].depth(true, true), 42);
		QCOMPARE(pileups[9].t(), 42);
		QCOMPARE(pileups[9].indels().count(), 0);
		QCOMPARE(pileups[10].depth(true, true), 42);
		QCOMPARE(pileups[10].t(), 42);
		QCOMPARE(pileups[10].indels().count(), 0);
		QCOMPARE(pileups.count(), 11);
	}


	void getSNPs()
	{
		VariantList list = NGSHelper::getSNPs();
		QCOMPARE(list.count(), 19255);
	}

	void getIndels()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		BamReader reader;
		NGSHelper::openBAM(reader, QFINDTESTDATA("data_in/panel.bam"));
		QVector<Sequence> indels;
		int depth;
		double mapq0_frac;

		//inseration of TT
		NGSHelper::getIndels(reference, reader, "chr6", 110053825-20, 110053825+20, indels, depth, mapq0_frac);
		QCOMPARE(depth, 42);
		QCOMPARE(indels.count(), 30);
		QCOMPARE(indels.count("+TT"), 10);
		QCOMPARE(indels.count("+T"), 18);
		QCOMPARE(indels.count("-T"), 2);
		TFW::compareDouble(mapq0_frac, 0.0, 0.001);

		//deletion of AG
		NGSHelper::getIndels(reference, reader, "chr14", 53513479-10, 53513480+10, indels, depth, mapq0_frac);
		QCOMPARE(depth, 64);
		QCOMPARE(indels.count(), 14);
		QCOMPARE(indels.count("-AG"), 14);
		TFW::compareDouble(mapq0_frac, 0.0, 0.001);
	}

	void changeSeq()
	{
		QCOMPARE(NGSHelper::changeSeq("", true, true), QByteArray(""));

		QCOMPARE(NGSHelper::changeSeq("ACGT", false, false), QByteArray("ACGT"));
		QCOMPARE(NGSHelper::changeSeq("ACGTN", true, false), QByteArray("NTGCA"));
		QCOMPARE(NGSHelper::changeSeq("ACGTN", false, true), QByteArray("TGCAN"));
		QCOMPARE(NGSHelper::changeSeq("ACGTN", true, true), QByteArray("NACGT"));
	}

	void getDepthFreq()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		BamReader reader;
		NGSHelper::openBAM(reader, QFINDTESTDATA("data_in/panel.bam"));

		//inseration T (left)
		Variant v("chr6", 110053825, 110053825, "-", "T");
		VariantDetails output = NGSHelper::getVariantDetails(reader, reference, v);
		QCOMPARE(output.depth, 42);
		TFW::compareDouble(output.frequency, 0.428, 0.001);

		//inseration T (right)
		v = Variant("chr16", 89576894, 89576894, "-", "T");
		output = NGSHelper::getVariantDetails(reader, reference, v);
		QCOMPARE(output.depth, 126);
		TFW::compareDouble(output.frequency, 0.126, 0.001);

		//deletion AG
		v = Variant("chr14", 53513479, 53513480, "AG", "-");
		output = NGSHelper::getVariantDetails(reader, reference, v);
		QCOMPARE(output.depth, 64);
		TFW::compareDouble(output.frequency, 0.218, 0.001);

		//SNP A>G (het)
		v = Variant("chr4", 108868411, 108868411, "A", "G");
		output = NGSHelper::getVariantDetails(reader, reference, v);
		QCOMPARE(output.depth, 78);
		TFW::compareDouble(output.frequency, 0.333, 0.001);

		//SNP C>T (hom)
		v = Variant("chr2", 202625615, 202625615, "C", "T");
		output = NGSHelper::getVariantDetails(reader, reference, v);
		QCOMPARE(output.depth, 166);
		TFW::compareDouble(output.frequency, 1.0, 0.001);
	}

	void softClipAlignment()
	{
		BamReader reader;
		NGSHelper::openBAM(reader, QFINDTESTDATA("data_in/panel.bam"));
		BamAlignment al1;

		//first soft-clip al1
		reader.GetNextAlignment(al1);
		while(!al1.IsMapped())		reader.GetNextAlignment(al1);

		NGSHelper::softClipAlignment(al1,317587,317596);
		//build CIGAR-string
		QString clipped_cigar1 = "10S123M13I5M";
		QString clipped_cigar2 = NGSHelper::Cigar2QString(al1.CigarData);
		QCOMPARE(clipped_cigar2, clipped_cigar1);

		//second soft-clip same al1
		NGSHelper::softClipAlignment(al1,317597,317721);
		//build CIGAR-strings
		QString clipped_cigar3 = "148S3M";
		QString clipped_cigar4 = NGSHelper::Cigar2QString(al1.CigarData);
		QCOMPARE(clipped_cigar4, clipped_cigar3);

		//next alignment
		reader.GetNextAlignment(al1);
		while(!al1.IsMapped())		reader.GetNextAlignment(al1);

		//third soft-clip different al1
		NGSHelper::softClipAlignment(al1,1048687,1048692);
		//build CIGAR-strings
		NGSHelper::softClipAlignment(al1,1048540,1048576);
		QString clipped_cigar7 = "36S109M6S";
		QString clipped_cigar8 = NGSHelper::Cigar2QString(al1.CigarData);
		QCOMPARE(clipped_cigar7, clipped_cigar8);
	}

	void cigar2String()
	{
		BamReader reader;
		NGSHelper::openBAM(reader, QFINDTESTDATA("data_in/panel.bam"));
		BamAlignment al1;
		reader.GetNextAlignment(al1);
		while(!al1.IsMapped())		reader.GetNextAlignment(al1);
		reader.GetNextAlignment(al1);
		while(!al1.IsMapped())		reader.GetNextAlignment(al1);
		QString clipped_cigar5 = "36M2D115M";
		QString clipped_cigar6 = NGSHelper::Cigar2QString(al1.CigarData);
		QCOMPARE(clipped_cigar5, clipped_cigar6);
	}

};
