#include "TestFramework.h"
#include "FastaFileIndex.h"
#include "Settings.h"

TEST_CLASS(FastaFileIndex_Test)
{
Q_OBJECT
private slots:

	void seq_complete()
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

	void seq_substr()
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

	void seq_substr_large()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

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

	void lengthOf()
	{
		FastaFileIndex index(TESTDATA("data_in/example.fa"));
        I_EQUAL(index.lengthOf("chr14"), 1509);
	}

	void names()
	{
		FastaFileIndex index(TESTDATA("data_in/example.fa"));
		QList<QString> names = index.names();
		I_EQUAL(names.count(), 4);
		S_EQUAL(names[0], QString("chr14"));
		S_EQUAL(names[1], QString("chr15"));
		S_EQUAL(names[2], QString("chr16"));
		S_EQUAL(names[3], QString("chr17"));
	}

};
