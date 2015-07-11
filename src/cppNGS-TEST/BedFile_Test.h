#include "TestFramework.h"
#include "BedFile.h"


TEST_CLASS(BedFile_Test)
{
Q_OBJECT
private slots:

	void isSorted()
	{
		BedFile file;
		QVERIFY(file.isSorted());

		file.append(BedLine("chr2", 5, 10));
		QVERIFY(file.isSorted());

		file.append(BedLine("chr2", 15, 20));
		QVERIFY(file.isSorted());

		file.append(BedLine("chr21", 1, 2));
		file.append(BedLine("chr21", 15, 20));
		QVERIFY(file.isSorted());

		file.append(BedLine("chr21", 6, 9));
		QVERIFY(!file.isSorted());
	}

	void sort()
	{
		BedFile file;

		//empty sort
		file.sort();
		QCOMPARE(file.count(), 0);

		//one element sort
		file.append(BedLine("chr2", 5, 10));
		file.sort();
		QCOMPARE(file.count(), 1);

		//one chromosome sort
		file.append(BedLine("chr2", 15, 20));
		file.append(BedLine("chr2", 1, 2));
		QCOMPARE(file.isSorted(), false);
		file.sort();
		QCOMPARE(file.isSorted(), true);
		QCOMPARE(file.count(), 3);
		QCOMPARE(file[0].start(), 1);
		QCOMPARE(file[1].start(), 5);
		QCOMPARE(file[2].start(), 15);

		//several chromosomes sort
		file.append(BedLine("chr1", 14, 20));
		file.append(BedLine("chr1", 7, 23));
		QCOMPARE(file.isSorted(), false);
		file.sort();
		QCOMPARE(file.isSorted(), true);
		QCOMPARE(file.count(), 5);
		QCOMPARE(file[0].chr(), Chromosome("chr1"));
		QCOMPARE(file[0].start(), 7);
		QCOMPARE(file[1].chr(), Chromosome("chr1"));
		QCOMPARE(file[1].start(), 14);
		QCOMPARE(file[2].chr(), Chromosome("chr2"));
		QCOMPARE(file[2].start(), 1);
		QCOMPARE(file[3].chr(), Chromosome("chr2"));
		QCOMPARE(file[3].start(), 5);
		QCOMPARE(file[4].chr(), Chromosome("chr2"));
		QCOMPARE(file[4].start(), 15);
	}

	void sort_uniq()
	{
		BedFile file;
		file.append(BedLine("chr1", 7, 23));
		file.append(BedLine("chr2", 5, 10));
		file.append(BedLine("chr2", 15, 20));
		file.append(BedLine("chr2", 1, 2));
		file.append(BedLine("chr1", 14, 20));
		file.append(BedLine("chr1", 7, 23));
		file.append(BedLine("chr2", 1, 2));

		file.sort(true);
		QCOMPARE(file.isSorted(), true);
		QCOMPARE(file.count(), 5);
		QCOMPARE(file[0].chr(), Chromosome("chr1"));
		QCOMPARE(file[0].start(), 7);
		QCOMPARE(file[1].chr(), Chromosome("chr1"));
		QCOMPARE(file[1].start(), 14);
		QCOMPARE(file[2].chr(), Chromosome("chr2"));
		QCOMPARE(file[2].start(), 1);
		QCOMPARE(file[3].chr(), Chromosome("chr2"));
		QCOMPARE(file[3].start(), 5);
		QCOMPARE(file[4].chr(), Chromosome("chr2"));
		QCOMPARE(file[4].start(), 15);
	}

	void merge()
	{
		BedFile file;

		//empty merge
		file.merge();
		QCOMPARE(file.count(), 0);

		//one element merge
		file.append(BedLine("chr2", 5, 10));
		file.merge();
		QCOMPARE(file.count(), 1);

		//one chromosome merge
		file.append(BedLine("chr2", 10, 20));
		file.append(BedLine("chr2", 1, 2));
		file.merge();
		QCOMPARE(file.count(), 2);
		QCOMPARE(file[0].start(), 1);
		QCOMPARE(file[0].end(), 2);
		QCOMPARE(file[1].start(), 5);
		QCOMPARE(file[1].end(), 20);

		//several chromosomes merge
		file.append(BedLine("chr1", 14, 20));
		file.append(BedLine("chr1", 7, 23));
		file.merge();
		QCOMPARE(file.count(), 3);
		QCOMPARE(file[0].chr(), Chromosome("chr1"));
		QCOMPARE(file[0].start(), 7);
		QCOMPARE(file[0].end(), 23);
		QCOMPARE(file[1].chr(), Chromosome("chr2"));
		QCOMPARE(file[1].start(), 1);
		QCOMPARE(file[1].end(), 2);
		QCOMPARE(file[2].chr(), Chromosome("chr2"));
		QCOMPARE(file[2].start(), 5);
		QCOMPARE(file[2].end(), 20);

		//no back-to-back
		file.append(BedLine("chr1", 1, 6));
		file.append(BedLine("chr1", 24, 30));
		file.merge(false);
		QCOMPARE(file.count(), 5);
		QCOMPARE(file[0].chr(), Chromosome("chr1"));
		QCOMPARE(file[0].start(), 1);
		QCOMPARE(file[0].end(), 6);
		QCOMPARE(file[1].chr(), Chromosome("chr1"));
		QCOMPARE(file[1].start(), 7);
		QCOMPARE(file[1].end(), 23);
		QCOMPARE(file[2].chr(), Chromosome("chr1"));
		QCOMPARE(file[2].start(), 24);
		QCOMPARE(file[2].end(), 30);
		QCOMPARE(file[3].chr(), Chromosome("chr2"));
		QCOMPARE(file[3].start(), 1);
		QCOMPARE(file[3].end(), 2);
		QCOMPARE(file[4].chr(), Chromosome("chr2"));
		QCOMPARE(file[4].start(), 5);
		QCOMPARE(file[4].end(), 20);

		//merge_names (without content)
		file.merge(true, true);
		QCOMPARE(file.count(), 3);
		QCOMPARE(file[0].chr(), Chromosome("chr1"));
		QCOMPARE(file[0].start(), 1);
		QCOMPARE(file[0].end(), 30);
		QCOMPARE(file[0].annotations().count(), 1);
		QCOMPARE(file[0].annotations()[0], QString(",,"));
		QCOMPARE(file[1].chr(), Chromosome("chr2"));
		QCOMPARE(file[1].start(), 1);
		QCOMPARE(file[1].end(), 2);
		QCOMPARE(file[1].annotations().count(), 1);
		QCOMPARE(file[1].annotations()[0], QString(""));
		QCOMPARE(file[2].chr(), Chromosome("chr2"));
		QCOMPARE(file[2].start(), 5);
		QCOMPARE(file[2].end(), 20);
		QCOMPARE(file[2].annotations().count(), 1);
		QCOMPARE(file[2].annotations()[0], QString(""));

		//merge_names (with content)
		file.append(BedLine("chr1", 2, 31, QStringList() << "bli"));
		file.append(BedLine("chr2", 2, 7, QStringList() << "bla"));
		file.append(BedLine("chr2", 19, 25, QStringList() << "bluff"));
		file.merge(true, true);
		QCOMPARE(file.count(), 2);
		QCOMPARE(file[0].chr(), Chromosome("chr1"));
		QCOMPARE(file[0].start(), 1);
		QCOMPARE(file[0].end(), 31);
		QCOMPARE(file[0].annotations().count(), 1);
		QCOMPARE(file[0].annotations()[0], QString(",,,bli"));
		QCOMPARE(file[1].chr(), Chromosome("chr2"));
		QCOMPARE(file[1].start(), 1);
		QCOMPARE(file[1].end(), 25);
		QCOMPARE(file[1].annotations().count(), 1);
		QCOMPARE(file[1].annotations()[0], QString(",bla,,bluff"));
	}

	void load()
	{
		BedFile file;
		file.load(QFINDTESTDATA("data_in/demo.bed"));
		QCOMPARE(file.count(), 591);
		QCOMPARE(file[0].chr(), Chromosome("chr4"));
		QCOMPARE(file[0].start(), 843451);
		QCOMPARE(file[0].end(), 843572);
		QCOMPARE(file[590].chr(), Chromosome("chr22"));
		QCOMPARE(file[590].start(), 38565215);
		QCOMPARE(file[590].end(), 38565443);
		QCOMPARE(file.isSorted(), false);
	}

	void store()
	{
		BedFile file;
		file.load(QFINDTESTDATA("data_in/demo.bed"));
		file.store("out/BedFile_store01_out.bed");
		TFW::comareFiles("out/BedFile_store01_out.bed", QFINDTESTDATA("data_out/BedFile_store01_out.bed"));
	}

	void isMerged()
	{
		BedFile file;
		QVERIFY(file.isMerged());

		file.append(BedLine("chr2", 5, 10));
		QVERIFY(file.isMerged());

		file.append(BedLine("chr2", 15, 20));
		QVERIFY(file.isMerged());

		file.append(BedLine("chr21", 5, 10));
		file.append(BedLine("chr21", 15, 20));
		QVERIFY(file.isMerged());

		file.append(BedLine("chr21", 10, 20));
		QVERIFY(!file.isMerged());
	}

	void isMergedAndSorted()
	{
		BedFile file;
		QVERIFY(file.isMergedAndSorted());

		file.append(BedLine("chr2", 5, 10));
		QVERIFY(file.isMergedAndSorted());

		file.append(BedLine("chr2", 15, 20));
		QVERIFY(file.isMergedAndSorted());

		file.append(BedLine("chr21", 15, 20));
		file.append(BedLine("chr21", 5, 10));
		QVERIFY(!file.isMergedAndSorted());

		file.sort();
		QVERIFY(file.isMergedAndSorted());

		file.append(BedLine("chr21", 10, 15));
		QVERIFY(!file.isMergedAndSorted());
	}
	void extend()
	{
		BedFile file;
		file.append(BedLine("chr1", 15, 20));
		file.append(BedLine("chr2", 5, 10));
		file.append(BedLine("chr21", 200, 200));

		file.extend(10);

		QCOMPARE(file.count(), 3);
		QCOMPARE(file[0].chr(), Chromosome("chr1"));
		QCOMPARE(file[0].start(), 5);
		QCOMPARE(file[0].end(), 30);
		QCOMPARE(file[1].chr(), Chromosome("chr2"));
		QCOMPARE(file[1].start(), 1);
		QCOMPARE(file[1].end(), 20);
		QCOMPARE(file[2].chr(), Chromosome("chr21"));
		QCOMPARE(file[2].start(), 190);
		QCOMPARE(file[2].end(), 210);
	}

	void subtract()
	{
		BedFile file1;
		file1.append(BedLine("chr2", 1, 100));
		file1.append(BedLine("chr1", 5, 9));
		file1.append(BedLine("chr1", 9, 20));
		file1.append(BedLine("chr1", 8, 22));
		file1.append(BedLine("chr1", 10, 20));
		file1.append(BedLine("chr1", 10, 21));
		BedFile file2;
		file2.append(BedLine("chr1", 10, 20));

		file1.subtract(file2);

		QCOMPARE(file1.count(), 6);
		QCOMPARE(file1[0].chr(), Chromosome("chr2"));
		QCOMPARE(file1[0].start(), 1);
		QCOMPARE(file1[0].end(), 100);
		QCOMPARE(file1[1].chr(), Chromosome("chr1"));
		QCOMPARE(file1[1].start(), 5);
		QCOMPARE(file1[1].end(), 9);
		QCOMPARE(file1[2].chr(), Chromosome("chr1"));
		QCOMPARE(file1[2].start(), 9);
		QCOMPARE(file1[2].end(), 9);
		QCOMPARE(file1[3].chr(), Chromosome("chr1"));
		QCOMPARE(file1[3].start(), 8);
		QCOMPARE(file1[3].end(), 9);
		QCOMPARE(file1[4].chr(), Chromosome("chr1"));
		QCOMPARE(file1[4].start(), 21);
		QCOMPARE(file1[4].end(), 21);
		QCOMPARE(file1[5].chr(), Chromosome("chr1"));
		QCOMPARE(file1[5].start(), 21);
		QCOMPARE(file1[5].end(), 22);
	}

	void subtract2()
	{
		BedFile file1;
		file1.append(BedLine("chr11", 5000000, 6000000));
		file1.append(BedLine("chr13", 45000000, 46000000));
		file1.append(BedLine("chr16", 71000000, 72000000));
		file1.append(BedLine("chr16", 73000000, 74000000));
		BedFile file2;
		file2.append(BedLine("chr11", 5012596, 5012620));
		file2.append(BedLine("chr11", 5462579, 5462675));
		file2.append(BedLine("chr11", 5462707, 5462748));
		file2.append(BedLine("chr13", 45553329, 45553489));
		file2.append(BedLine("chr16", 71196324, 71196420));
		file2.append(BedLine("chr16", 71196480, 71196576));

		file1.subtract(file2);
		file1.merge();

		QCOMPARE(file1.count(), 10);
		QCOMPARE(file1[0].chr(), Chromosome("chr11"));
		QCOMPARE(file1[0].start(), 5000000);
		QCOMPARE(file1[0].end(), 5012595);
		QCOMPARE(file1[1].chr(), Chromosome("chr11"));
		QCOMPARE(file1[1].start(), 5012621);
		QCOMPARE(file1[1].end(), 5462578);
		QCOMPARE(file1[2].chr(), Chromosome("chr11"));
		QCOMPARE(file1[2].start(), 5462676);
		QCOMPARE(file1[2].end(), 5462706);
		QCOMPARE(file1[3].chr(), Chromosome("chr11"));
		QCOMPARE(file1[3].start(), 5462749);
		QCOMPARE(file1[3].end(), 6000000);
		QCOMPARE(file1[4].chr(), Chromosome("chr13"));
		QCOMPARE(file1[4].start(), 45000000);
		QCOMPARE(file1[4].end(), 45553328);
		QCOMPARE(file1[5].chr(), Chromosome("chr13"));
		QCOMPARE(file1[5].start(), 45553490);
		QCOMPARE(file1[5].end(), 46000000);
		QCOMPARE(file1[6].chr(), Chromosome("chr16"));
		QCOMPARE(file1[6].start(), 71000000);
		QCOMPARE(file1[6].end(), 71196323);
		QCOMPARE(file1[7].chr(), Chromosome("chr16"));
		QCOMPARE(file1[7].start(), 71196421);
		QCOMPARE(file1[7].end(), 71196479);
		QCOMPARE(file1[8].chr(), Chromosome("chr16"));
		QCOMPARE(file1[8].start(), 71196577);
		QCOMPARE(file1[8].end(), 72000000);
		QCOMPARE(file1[9].chr(), Chromosome("chr16"));
		QCOMPARE(file1[9].start(), 73000000);
		QCOMPARE(file1[9].end(), 74000000);
	}

	void shrink()
	{
		BedFile file1;
		file1.append(BedLine("chr11", 1, 2));
		file1.append(BedLine("chr13", 10, 20));
		file1.append(BedLine("chr16", 1, 3));
		file1.append(BedLine("chr16", 4, 10));

		file1.shrink(1);

		QCOMPARE(file1.count(), 3);
		QCOMPARE(file1[0].chr(), Chromosome("chr13"));
		QCOMPARE(file1[0].start(), 11);
		QCOMPARE(file1[0].end(), 19);
		QCOMPARE(file1[1].chr(), Chromosome("chr16"));
		QCOMPARE(file1[1].start(), 2);
		QCOMPARE(file1[1].end(), 2);
		QCOMPARE(file1[2].chr(), Chromosome("chr16"));
		QCOMPARE(file1[2].start(), 5);
		QCOMPARE(file1[2].end(), 9);

		file1.shrink(2);
		QCOMPARE(file1.count(), 2);
		QCOMPARE(file1[0].chr(), Chromosome("chr13"));
		QCOMPARE(file1[0].start(), 13);
		QCOMPARE(file1[0].end(), 17);
		QCOMPARE(file1[1].chr(), Chromosome("chr16"));
		QCOMPARE(file1[1].start(), 7);
		QCOMPARE(file1[1].end(), 7);

		file1.shrink(3);
		QCOMPARE(file1.count(), 0);
	}

	void intersect()
	{
		BedFile file1;
		BedFile file2;
		file1.intersect(file2);
		QCOMPARE(file1.count(), 0);

		file1.append(BedLine("chr1", 5, 9));
		file1.append(BedLine("chr1", 10, 20));
		file1.append(BedLine("chr2", 1, 100));
		file2.intersect(file1);
		QCOMPARE(file2.count(), 0);

		file1.append(BedLine("chr1", 10, 21));
		file1.append(BedLine("chr1", 8, 22));
		file1.append(BedLine("chr1", 9, 20));
		file1.sort();
		file2.append(BedLine("chr1", 5, 8));
		file2.append(BedLine("chr1", 21, 50));
		file1.intersect(file2);
		QCOMPARE(file1.count(), 4);
		QCOMPARE(file1[0].chr(), Chromosome("chr1"));
		QCOMPARE(file1[0].start(), 5);
		QCOMPARE(file1[0].end(), 8);
		QCOMPARE(file1[1].chr(), Chromosome("chr1"));
		QCOMPARE(file1[1].start(), 8);
		QCOMPARE(file1[1].end(), 8);
		QCOMPARE(file1[2].chr(), Chromosome("chr1"));
		QCOMPARE(file1[2].start(), 21);
		QCOMPARE(file1[2].end(), 21);
		QCOMPARE(file1[3].chr(), Chromosome("chr1"));
		QCOMPARE(file1[3].start(), 21);
		QCOMPARE(file1[3].end(), 22);
	}

	void overlapping()
	{
		BedFile file1;
		BedFile file2;
		file1.overlapping(file2);
		QCOMPARE(file1.count(), 0);

		file1.append(BedLine("chr1", 5, 9));
		file1.append(BedLine("chr1", 10, 20));
		file1.append(BedLine("chr2", 1, 100));
		file2.overlapping(file1);
		QCOMPARE(file2.count(), 0);

		file1.append(BedLine("chr1", 10, 21));
		file1.append(BedLine("chr1", 8, 22));
		file1.append(BedLine("chr1", 9, 20));
		file1.sort();
		file2.append(BedLine("chr1", 5, 8));
		file2.append(BedLine("chr1", 21, 50));
		file1.overlapping(file2);
		QCOMPARE(file1.count(), 3);
		QCOMPARE(file1[0].chr(), Chromosome("chr1"));
		QCOMPARE(file1[0].start(), 5);
		QCOMPARE(file1[0].end(), 9);
		QCOMPARE(file1[1].chr(), Chromosome("chr1"));
		QCOMPARE(file1[1].start(), 8);
		QCOMPARE(file1[1].end(), 22);
		QCOMPARE(file1[2].chr(), Chromosome("chr1"));
		QCOMPARE(file1[2].start(), 10);
		QCOMPARE(file1[2].end(), 21);
	}

	void chunk()
	{
		//first test: simple regions
		BedFile f;
		f.append(BedLine("chr1", 100, 110, QStringList() << "1"));
		f.append(BedLine("chr2", 200, 219));
		f.append(BedLine("chr3", 250, 270));
		f.append(BedLine("chr4", 300, 330, QStringList() << "1" << "2" << "3"));
		f.append(BedLine("chr5", 400, 440));
		f.append(BedLine("chr6", 500, 550));
		f.append(BedLine("chr7", 600, 660));
		f.append(BedLine("chr8", 700, 770));
		f.append(BedLine("chr9", 800, 880));
		f.append(BedLine("chr10", 900, 990));
		f.chunk(15);
		QCOMPARE(f.count(), 32);
		QCOMPARE(f[0], BedLine("chr1", 100, 110));
		QCOMPARE(f[0].annotations(), QStringList() << "1");
		QCOMPARE(f[1], BedLine("chr2", 200, 219));
		QCOMPARE(f[1].annotations(), QStringList());
		QCOMPARE(f[2], BedLine("chr3", 250, 259));
		QCOMPARE(f[2].annotations(), QStringList());
		QCOMPARE(f[3], BedLine("chr3", 260, 270));
		QCOMPARE(f[3].annotations(), QStringList());
		QCOMPARE(f[4], BedLine("chr4", 300, 315));
		QCOMPARE(f[4].annotations(), QStringList() << "1" << "2" << "3");
		QCOMPARE(f[5], BedLine("chr4", 316, 330));
		QCOMPARE(f[5].annotations(), QStringList() << "1" << "2" << "3");
		/* further tests are skipped
		chr5 size=41 chunks=(13, 14, 14)
		chr6 size=51 chunks=(17, 17, 17)
		chr7 size=61 chunks=(16, 15, 15, 15)
		chr8 size=71 chunks=(14, 14, 14, 14, 15)
		chr9 size=81 chunks=(17, 16, 16, 16, 16)
		chr10 size=91 chunks=(16, 15, 15, 15, 15, 15)
		*/

		//second test: load file, chunk, merge, check that the file is still the same...
		f.load(QFINDTESTDATA("data_in/demo.bed"));
		f.merge();
		QCOMPARE(f.count(), 591);

		BedFile f2 = f;
		f2.chunk(15);
		QCOMPARE(f2.count(), 6119);
		f2.merge();
		QCOMPARE(f2.count(), 591);
		for (int i=0; i<f.count(); ++i)
		{
			QCOMPARE(f2[i], f[i]);
		}
	}
};
