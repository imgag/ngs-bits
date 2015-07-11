#include "TestFramework.h"
#include "FastaFileIndex.h"
#include "Settings.h"

TEST_CLASS(FastaFileIndex_Test)
{
Q_OBJECT
private slots:

	void seq_complete()
	{
		FastaFileIndex index(QFINDTESTDATA("data_in/example.fa"));
		Sequence seq = index.seq("chr14", false);
		QCOMPARE(seq.length(), 1509);
		QCOMPARE(seq.left(10), Sequence("ataaaccaac"));
		QCOMPARE(seq.right(10), Sequence("tgaaaaataa"));

		seq = index.seq("chr15", false);
		QCOMPARE(seq, Sequence("cgat"));

		seq = index.seq("chr16", false);
		QCOMPARE(seq, Sequence("gattaca"));

		seq = index.seq("chr17", false);
		QCOMPARE(seq, Sequence("acgt"));

		seq = index.seq("chr17");
		QCOMPARE(seq, Sequence("ACGT"));
	}

	void seq_substr()
	{
		FastaFileIndex index(QFINDTESTDATA("data_in/example.fa"));
		Sequence seq = index.seq("chr14", 1, 10, false);
		QCOMPARE(seq, Sequence("ataaaccaac"));
		seq = index.seq("chr14", 1500, 10, false);
		QCOMPARE(seq, Sequence("tgaaaaataa"));

		seq = index.seq("chr15", 1, 4, false);
		QCOMPARE(seq, Sequence("cgat"));

		seq = index.seq("chr16", 1, 4, false);
		QCOMPARE(seq, Sequence("gatt"));

		seq = index.seq("chr17", 1, 4, false);
		QCOMPARE(seq, Sequence("acgt"));

		seq = index.seq("chr17", 1, 4);
		QCOMPARE(seq, Sequence("ACGT"));
	}

	void seq_substr_large()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");

		FastaFileIndex index(ref_file);
		Sequence seq = index.seq("chr16", 87637935, 1);
		QCOMPARE(seq, Sequence("G"));

		seq = index.seq("chr16", 87637934, 3);
		QCOMPARE(seq, Sequence("TGT"));

		seq = index.seq("chr16", 87637933, 5);
		QCOMPARE(seq, Sequence("CTGTA"));

		seq = index.seq("chrUn_gl000249", 1, 5);
		QCOMPARE(seq, Sequence("GATCA"));
	}

	void lengthOf()
	{
		FastaFileIndex index(QFINDTESTDATA("data_in/example.fa"));
		QCOMPARE(index.lengthOf("chr14"), 1509);
	}

	void names()
	{
		FastaFileIndex index(QFINDTESTDATA("data_in/example.fa"));
		QList<QString> names = index.names();
		QCOMPARE(names.count(), 4);
		QCOMPARE(names[0], QString("14"));
		QCOMPARE(names[1], QString("15"));
		QCOMPARE(names[2], QString("16"));
		QCOMPARE(names[3], QString("17"));
	}

};
