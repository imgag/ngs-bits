#include "TestFrameworkNGS.h"
#include "FastaFileIndex.h"
#include "Settings.h"

TEST_CLASS(FastaFileIndex_Test)
{
private:

	TEST_METHOD(seq_complete)
	{
		FastaFileIndex index(TESTDATA("data_in/example.fa"));
		Sequence seq = index.seq("chr14", false);
		I_EQUAL(seq.length(), 1509);
		S_EQUAL(seq.left(10), Sequence("ataaaccaac"));
		S_EQUAL(seq.right(10), Sequence("tgaaaaataa"));

		seq = index.seq("chr15", false);
		S_EQUAL(seq, Sequence("cgat"));

		seq = index.seq("chr16", false);
		S_EQUAL(seq, Sequence("gattaca"));

		seq = index.seq("chr17", false);
		S_EQUAL(seq, Sequence("acgt"));

		seq = index.seq("chr17");
		S_EQUAL(seq, Sequence("ACGT"));
	}

	TEST_METHOD(seq_substr)
	{
		FastaFileIndex index(TESTDATA("data_in/example.fa"));
		Sequence seq = index.seq("chr14", 1, 10, false);
		S_EQUAL(seq, Sequence("ataaaccaac"));
		seq = index.seq("chr14", 1500, 10, false);
		S_EQUAL(seq, Sequence("tgaaaaataa"));

		seq = index.seq("chr15", 1, 4, false);
		S_EQUAL(seq, Sequence("cgat"));

		seq = index.seq("chr16", 1, 4, false);
		S_EQUAL(seq, Sequence("gatt"));

		seq = index.seq("chr17", 1, 4, false);
		S_EQUAL(seq, Sequence("acgt"));

		seq = index.seq("chr17", 1, 4);
		S_EQUAL(seq, Sequence("ACGT"));
	}

	TEST_METHOD(seq_substr_large)
	{
		SKIP_IF_NO_HG38_GENOME();

		QString ref_file = Settings::string("reference_genome", true);
		FastaFileIndex index(ref_file);
		Sequence seq = index.seq("chr16", 87604329, 1);
		S_EQUAL(seq, Sequence("G"));

		seq = index.seq("chr16", 87604328, 3);
		S_EQUAL(seq, Sequence("TGT"));

		seq = index.seq("chr16", 87604327, 5);
		S_EQUAL(seq, Sequence("CTGTA"));

		seq = index.seq("chrUn_GL000218v1", 1, 5);
		S_EQUAL(seq, Sequence("GAATT"));
	}

	TEST_METHOD(lengthOf)
	{
		FastaFileIndex index(TESTDATA("data_in/example.fa"));
        I_EQUAL(index.lengthOf("chr14"), 1509);
	}

	TEST_METHOD(n)
	{
		FastaFileIndex index(TESTDATA("data_in/example.fa"));
		I_EQUAL(index.n("chr14"), 4);
	}

	TEST_METHOD(names)
	{
		FastaFileIndex index(TESTDATA("data_in/example.fa"));
		QList<Chromosome> names = index.chromosomes();
		I_EQUAL(names.count(), 4);
		S_EQUAL(names[0].str(), QString("chr14"));
		S_EQUAL(names[1].str(), QString("chr15"));
		S_EQUAL(names[2].str(), QString("chr16"));
		S_EQUAL(names[3].str(), QString("chr17"));
	}

};
