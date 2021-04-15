#include "TestFramework.h"
#include "BedFile.h"


TEST_CLASS(BedFile_Test)
{
Q_OBJECT
private slots:

	void convenience_constructor()
	{
		BedFile file("chr1", 1, 100);
		I_EQUAL(file.count(), 1);
		I_EQUAL(file.baseCount(), 100);
	}

	void isSorted()
	{
		BedFile file;
		IS_TRUE(file.isSorted());

		file.append(BedLine("chr2", 5, 10));
		IS_TRUE(file.isSorted());

		file.append(BedLine("chr2", 15, 20));
		IS_TRUE(file.isSorted());

		file.append(BedLine("chr21", 1, 2));
		file.append(BedLine("chr21", 15, 20));
		IS_TRUE(file.isSorted());

		file.append(BedLine("chr21", 15, 20));
		IS_TRUE(file.isSorted());

		file.append(BedLine("chr21", 6, 9));
		IS_TRUE(!file.isSorted());
	}

	void sort()
	{
		BedFile file;

		//empty sort
		file.sort();
		I_EQUAL(file.count(), 0);

		//one element sort
		file.append(BedLine("chr2", 5, 10));
		file.sort();
		I_EQUAL(file.count(), 1);

		//one chromosome sort
		file.append(BedLine("chr2", 15, 20));
		file.append(BedLine("chr2", 1, 2));
		IS_FALSE(file.isSorted());
		file.sort();
		IS_TRUE(file.isSorted());
		I_EQUAL(file.count(), 3);
		I_EQUAL(file[0].start(), 1);
		I_EQUAL(file[1].start(), 5);
		I_EQUAL(file[2].start(), 15);

		//several chromosomes sort
		file.append(BedLine("chr1", 14, 20));
		file.append(BedLine("chr1", 7, 23));
		IS_FALSE(file.isSorted());
		file.sort();
		IS_TRUE(file.isSorted());
		I_EQUAL(file.count(), 5);
		X_EQUAL(file[0].chr(), Chromosome("chr1"));
		I_EQUAL(file[0].start(), 7);
		X_EQUAL(file[1].chr(), Chromosome("chr1"));
		I_EQUAL(file[1].start(), 14);
		X_EQUAL(file[2].chr(), Chromosome("chr2"));
		I_EQUAL(file[2].start(), 1);
		X_EQUAL(file[3].chr(), Chromosome("chr2"));
		I_EQUAL(file[3].start(), 5);
		X_EQUAL(file[4].chr(), Chromosome("chr2"));
		I_EQUAL(file[4].start(), 15);
	}

	void sortWithName()
	{
		BedFile file;
		file.append(BedLine("chr1", 14, 20, QByteArrayList() << "X"));
		file.append(BedLine("chr1", 14, 20, QByteArrayList()));
		file.append(BedLine("chr1", 14, 20, QByteArrayList() << "Y"));
		file.append(BedLine("chr1", 7, 9, QByteArrayList() << "C"));
		file.append(BedLine("chr1", 7, 9, QByteArrayList() << "B"));
		file.append(BedLine("chr1", 7, 9, QByteArrayList() << "A"));

		file.sortWithName();
		IS_TRUE(file.isSorted());
		I_EQUAL(file.count(), 6);
		I_EQUAL(file[0].annotations().count(), 1);
		S_EQUAL(file[0].annotations()[0], "A");
		I_EQUAL(file[1].annotations().count(), 1);
		S_EQUAL(file[1].annotations()[0], "B");
		I_EQUAL(file[2].annotations().count(), 1);
		S_EQUAL(file[2].annotations()[0], "C");
		I_EQUAL(file[3].annotations().count(), 0);
		I_EQUAL(file[4].annotations().count(), 1);
		S_EQUAL(file[4].annotations()[0], "X");
		I_EQUAL(file[5].annotations().count(), 1);
		S_EQUAL(file[5].annotations()[0], "Y");
	}

	void removeDuplicates()
	{
		BedFile file;
		file.append(BedLine("chr1", 7, 23));
		file.append(BedLine("chr1", 7, 23));
		file.append(BedLine("chr1", 14, 20));
		file.append(BedLine("chr2", 1, 2));
		file.append(BedLine("chr2", 1, 2));
		file.append(BedLine("chr2", 5, 10));
		file.append(BedLine("chr2", 15, 20));

		file.removeDuplicates();
		IS_TRUE(file.isSorted());
		I_EQUAL(file.count(), 5);
		X_EQUAL(file[0].chr(), Chromosome("chr1"));
		I_EQUAL(file[0].start(), 7);
		X_EQUAL(file[1].chr(), Chromosome("chr1"));
		I_EQUAL(file[1].start(), 14);
		X_EQUAL(file[2].chr(), Chromosome("chr2"));
		I_EQUAL(file[2].start(), 1);
		X_EQUAL(file[3].chr(), Chromosome("chr2"));
		I_EQUAL(file[3].start(), 5);
		X_EQUAL(file[4].chr(), Chromosome("chr2"));
		I_EQUAL(file[4].start(), 15);
	}

	void merge()
	{
		BedFile file;

		//empty merge
		file.merge();
		I_EQUAL(file.count(), 0);

		//one element merge
		file.append(BedLine("chr2", 5, 10));
		file.merge();
		I_EQUAL(file.count(), 1);

		//one chromosome merge
		file.append(BedLine("chr2", 10, 20));
		file.append(BedLine("chr2", 1, 2));
		file.merge();
		I_EQUAL(file.count(), 2);
		I_EQUAL(file[0].start(), 1);
		I_EQUAL(file[0].end(), 2);
		I_EQUAL(file[1].start(), 5);
		I_EQUAL(file[1].end(), 20);

		//several chromosomes merge
		file.append(BedLine("chr1", 14, 20));
		file.append(BedLine("chr1", 7, 23));
		file.merge();
		I_EQUAL(file.count(), 3);
		X_EQUAL(file[0].chr(), Chromosome("chr1"));
		I_EQUAL(file[0].start(), 7);
		I_EQUAL(file[0].end(), 23);
		X_EQUAL(file[1].chr(), Chromosome("chr2"));
		I_EQUAL(file[1].start(), 1);
		I_EQUAL(file[1].end(), 2);
		X_EQUAL(file[2].chr(), Chromosome("chr2"));
		I_EQUAL(file[2].start(), 5);
		I_EQUAL(file[2].end(), 20);

		//no back-to-back
		file.append(BedLine("chr1", 1, 6));
		file.append(BedLine("chr1", 24, 30));
		file.merge(false);
		I_EQUAL(file.count(), 5);
		X_EQUAL(file[0].chr(), Chromosome("chr1"));
		I_EQUAL(file[0].start(), 1);
		I_EQUAL(file[0].end(), 6);
		X_EQUAL(file[1].chr(), Chromosome("chr1"));
		I_EQUAL(file[1].start(), 7);
		I_EQUAL(file[1].end(), 23);
		X_EQUAL(file[2].chr(), Chromosome("chr1"));
		I_EQUAL(file[2].start(), 24);
		I_EQUAL(file[2].end(), 30);
		X_EQUAL(file[3].chr(), Chromosome("chr2"));
		I_EQUAL(file[3].start(), 1);
		I_EQUAL(file[3].end(), 2);
		X_EQUAL(file[4].chr(), Chromosome("chr2"));
		I_EQUAL(file[4].start(), 5);
		I_EQUAL(file[4].end(), 20);

		//merge_names (without content)
		file.merge(true, true);
		I_EQUAL(file.count(), 3);
		X_EQUAL(file[0].chr(), Chromosome("chr1"));
		I_EQUAL(file[0].start(), 1);
		I_EQUAL(file[0].end(), 30);
		I_EQUAL(file[0].annotations().count(), 1);
		S_EQUAL(file[0].annotations()[0], QString(",,"));
		X_EQUAL(file[1].chr(), Chromosome("chr2"));
		I_EQUAL(file[1].start(), 1);
		I_EQUAL(file[1].end(), 2);
		I_EQUAL(file[1].annotations().count(), 1);
		X_EQUAL(file[1].annotations()[0], QString(""));
		X_EQUAL(file[2].chr(), Chromosome("chr2"));
		I_EQUAL(file[2].start(), 5);
		I_EQUAL(file[2].end(), 20);
		I_EQUAL(file[2].annotations().count(), 1);
		S_EQUAL(file[2].annotations()[0], QString(""));

		//merge_names (with content)
		file.append(BedLine("chr1", 2, 31, QList<QByteArray>() << "bli"));
		file.append(BedLine("chr2", 2, 7, QList<QByteArray>() << "bla"));
		file.append(BedLine("chr2", 19, 25, QList<QByteArray>() << "bluff"));
		file.merge(true, true);
		I_EQUAL(file.count(), 2);
		X_EQUAL(file[0].chr(), Chromosome("chr1"));
		I_EQUAL(file[0].start(), 1);
		I_EQUAL(file[0].end(), 31);
		I_EQUAL(file[0].annotations().count(), 1);
		S_EQUAL(file[0].annotations()[0], QString(",,,bli"));
		X_EQUAL(file[1].chr(), Chromosome("chr2"));
		I_EQUAL(file[1].start(), 1);
		I_EQUAL(file[1].end(), 25);
		I_EQUAL(file[1].annotations().count(), 1);
		S_EQUAL(file[1].annotations()[0], QString(",bla,,bluff"));
	}

	void load()
	{
		BedFile file;
		file.load(TESTDATA("data_in/demo.bed"));
		I_EQUAL(file.count(), 591);
		X_EQUAL(file[0].chr(), Chromosome("chr4"));
		I_EQUAL(file[0].start(), 843451);
		I_EQUAL(file[0].end(), 843572);
		X_EQUAL(file[590].chr(), Chromosome("chr22"));
		I_EQUAL(file[590].start(), 38565215);
		I_EQUAL(file[590].end(), 38565443);
		IS_FALSE(file.isSorted());
	}

	void store()
	{
		BedFile file;
		file.load(TESTDATA("data_in/demo.bed"));
		file.store("out/BedFile_store01_out.bed");
		COMPARE_FILES("out/BedFile_store01_out.bed", TESTDATA("data_out/BedFile_store01_out.bed"));
	}

	void isMerged()
	{
		BedFile file;
		IS_TRUE(file.isMerged());

		file.append(BedLine("chr2", 5, 10));
		IS_TRUE(file.isMerged());

		file.append(BedLine("chr2", 15, 20));
		IS_TRUE(file.isMerged());

		file.append(BedLine("chr21", 5, 10));
		file.append(BedLine("chr21", 15, 20));
		IS_TRUE(file.isMerged());

		file.append(BedLine("chr21", 10, 20));
		IS_TRUE(!file.isMerged());
	}

	void isMergedAndSorted()
	{
		BedFile file;
		IS_TRUE(file.isMergedAndSorted());

		file.append(BedLine("chr2", 5, 10));
		IS_TRUE(file.isMergedAndSorted());

		file.append(BedLine("chr2", 15, 20));
		IS_TRUE(file.isMergedAndSorted());

		file.append(BedLine("chr21", 15, 20));
		file.append(BedLine("chr21", 5, 10));
		IS_TRUE(!file.isMergedAndSorted());

		file.sort();
		IS_TRUE(file.isMergedAndSorted());

		file.append(BedLine("chr21", 10, 15));
		IS_TRUE(!file.isMergedAndSorted());
	}
	void extend()
	{
		BedFile file;
		file.append(BedLine("chr1", 15, 20));
		file.append(BedLine("chr2", 5, 10));
		file.append(BedLine("chr21", 200, 200));

		file.extend(10);

		I_EQUAL(file.count(), 3);
		X_EQUAL(file[0].chr(), Chromosome("chr1"));
		I_EQUAL(file[0].start(), 5);
		I_EQUAL(file[0].end(), 30);
		X_EQUAL(file[1].chr(), Chromosome("chr2"));
		I_EQUAL(file[1].start(), 1);
		I_EQUAL(file[1].end(), 20);
		X_EQUAL(file[2].chr(), Chromosome("chr21"));
		I_EQUAL(file[2].start(), 190);
		I_EQUAL(file[2].end(), 210);
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

		I_EQUAL(file1.count(), 6);
		X_EQUAL(file1[0].chr(), Chromosome("chr2"));
		I_EQUAL(file1[0].start(), 1);
		I_EQUAL(file1[0].end(), 100);
		X_EQUAL(file1[1].chr(), Chromosome("chr1"));
		I_EQUAL(file1[1].start(), 5);
		I_EQUAL(file1[1].end(), 9);
		X_EQUAL(file1[2].chr(), Chromosome("chr1"));
		I_EQUAL(file1[2].start(), 9);
		I_EQUAL(file1[2].end(), 9);
		X_EQUAL(file1[3].chr(), Chromosome("chr1"));
		I_EQUAL(file1[3].start(), 8);
		I_EQUAL(file1[3].end(), 9);
		X_EQUAL(file1[4].chr(), Chromosome("chr1"));
		I_EQUAL(file1[4].start(), 21);
		I_EQUAL(file1[4].end(), 21);
		X_EQUAL(file1[5].chr(), Chromosome("chr1"));
		I_EQUAL(file1[5].start(), 21);
		I_EQUAL(file1[5].end(), 22);
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

		I_EQUAL(file1.count(), 10);
		X_EQUAL(file1[0].chr(), Chromosome("chr11"));
		I_EQUAL(file1[0].start(), 5000000);
		I_EQUAL(file1[0].end(), 5012595);
		X_EQUAL(file1[1].chr(), Chromosome("chr11"));
		I_EQUAL(file1[1].start(), 5012621);
		I_EQUAL(file1[1].end(), 5462578);
		X_EQUAL(file1[2].chr(), Chromosome("chr11"));
		I_EQUAL(file1[2].start(), 5462676);
		I_EQUAL(file1[2].end(), 5462706);
		X_EQUAL(file1[3].chr(), Chromosome("chr11"));
		I_EQUAL(file1[3].start(), 5462749);
		I_EQUAL(file1[3].end(), 6000000);
		X_EQUAL(file1[4].chr(), Chromosome("chr13"));
		I_EQUAL(file1[4].start(), 45000000);
		I_EQUAL(file1[4].end(), 45553328);
		X_EQUAL(file1[5].chr(), Chromosome("chr13"));
		I_EQUAL(file1[5].start(), 45553490);
		I_EQUAL(file1[5].end(), 46000000);
		X_EQUAL(file1[6].chr(), Chromosome("chr16"));
		I_EQUAL(file1[6].start(), 71000000);
		I_EQUAL(file1[6].end(), 71196323);
		X_EQUAL(file1[7].chr(), Chromosome("chr16"));
		I_EQUAL(file1[7].start(), 71196421);
		I_EQUAL(file1[7].end(), 71196479);
		X_EQUAL(file1[8].chr(), Chromosome("chr16"));
		I_EQUAL(file1[8].start(), 71196577);
		I_EQUAL(file1[8].end(), 72000000);
		X_EQUAL(file1[9].chr(), Chromosome("chr16"));
		I_EQUAL(file1[9].start(), 73000000);
		I_EQUAL(file1[9].end(), 74000000);
	}

	void shrink()
	{
		BedFile file1;
		file1.append(BedLine("chr11", 1, 2));
		file1.append(BedLine("chr13", 10, 20));
		file1.append(BedLine("chr16", 1, 3));
		file1.append(BedLine("chr16", 4, 10));

		file1.shrink(1);

		I_EQUAL(file1.count(), 3);
		X_EQUAL(file1[0].chr(), Chromosome("chr13"));
		I_EQUAL(file1[0].start(), 11);
		I_EQUAL(file1[0].end(), 19);
		X_EQUAL(file1[1].chr(), Chromosome("chr16"));
		I_EQUAL(file1[1].start(), 2);
		I_EQUAL(file1[1].end(), 2);
		X_EQUAL(file1[2].chr(), Chromosome("chr16"));
		I_EQUAL(file1[2].start(), 5);
		I_EQUAL(file1[2].end(), 9);

		file1.shrink(2);
		I_EQUAL(file1.count(), 2);
		X_EQUAL(file1[0].chr(), Chromosome("chr13"));
		I_EQUAL(file1[0].start(), 13);
		I_EQUAL(file1[0].end(), 17);
		X_EQUAL(file1[1].chr(), Chromosome("chr16"));
		I_EQUAL(file1[1].start(), 7);
		I_EQUAL(file1[1].end(), 7);

		file1.shrink(3);
		I_EQUAL(file1.count(), 0);
	}

	void intersect()
	{
		BedFile file1;
		BedFile file2;
		file1.intersect(file2);
		I_EQUAL(file1.count(), 0);

		file1.append(BedLine("chr1", 5, 9));
		file1.append(BedLine("chr1", 10, 20));
		file1.append(BedLine("chr2", 1, 100));
		file2.intersect(file1);
		I_EQUAL(file2.count(), 0);

		file1.append(BedLine("chr1", 10, 21));
		file1.append(BedLine("chr1", 8, 22));
		file1.append(BedLine("chr1", 9, 20));
		file1.sort();
		file2.append(BedLine("chr1", 5, 8));
		file2.append(BedLine("chr1", 21, 50));
		file1.intersect(file2);
		I_EQUAL(file1.count(), 4);
		X_EQUAL(file1[0].chr(), Chromosome("chr1"));
		I_EQUAL(file1[0].start(), 5);
		I_EQUAL(file1[0].end(), 8);
		X_EQUAL(file1[1].chr(), Chromosome("chr1"));
		I_EQUAL(file1[1].start(), 8);
		I_EQUAL(file1[1].end(), 8);
		X_EQUAL(file1[2].chr(), Chromosome("chr1"));
		I_EQUAL(file1[2].start(), 21);
		I_EQUAL(file1[2].end(), 21);
		X_EQUAL(file1[3].chr(), Chromosome("chr1"));
		I_EQUAL(file1[3].start(), 21);
		I_EQUAL(file1[3].end(), 22);
	}

	void overlapping()
	{
		BedFile file1;
		BedFile file2;
		file1.overlapping(file2);
		I_EQUAL(file1.count(), 0);

		file1.append(BedLine("chr1", 5, 9));
		file1.append(BedLine("chr1", 10, 20));
		file1.append(BedLine("chr2", 1, 100));
		file2.overlapping(file1);
		I_EQUAL(file2.count(), 0);

		file1.append(BedLine("chr1", 10, 21));
		file1.append(BedLine("chr1", 8, 22));
		file1.append(BedLine("chr1", 9, 20));
		file1.sort();
		file2.append(BedLine("chr1", 5, 8));
		file2.append(BedLine("chr1", 21, 50));
		file1.overlapping(file2);
		I_EQUAL(file1.count(), 3);
		X_EQUAL(file1[0].chr(), Chromosome("chr1"));
		I_EQUAL(file1[0].start(), 5);
		I_EQUAL(file1[0].end(), 9);
		X_EQUAL(file1[1].chr(), Chromosome("chr1"));
		I_EQUAL(file1[1].start(), 8);
		I_EQUAL(file1[1].end(), 22);
		X_EQUAL(file1[2].chr(), Chromosome("chr1"));
		I_EQUAL(file1[2].start(), 10);
		I_EQUAL(file1[2].end(), 21);
	}

	void chunk()
	{
		//first test: simple regions
		BedFile f;
		f.append(BedLine("chr1", 100, 110, QList<QByteArray>() << "1"));
		f.append(BedLine("chr2", 200, 219));
		f.append(BedLine("chr3", 250, 270));
		f.append(BedLine("chr4", 300, 330, QList<QByteArray>() << "1" << "2" << "3"));
		f.append(BedLine("chr5", 400, 440));
		f.append(BedLine("chr6", 500, 550));
		f.append(BedLine("chr7", 600, 660));
		f.append(BedLine("chr8", 700, 770));
		f.append(BedLine("chr9", 800, 880));
		f.append(BedLine("chr10", 900, 990));
		f.chunk(15);
		I_EQUAL(f.count(), 32);
		X_EQUAL(f[0], BedLine("chr1", 100, 110));
		X_EQUAL(f[0].annotations(), QList<QByteArray>() << "1");
		X_EQUAL(f[1], BedLine("chr2", 200, 219));
		X_EQUAL(f[1].annotations(), QList<QByteArray>());
		X_EQUAL(f[2], BedLine("chr3", 250, 259));
		X_EQUAL(f[2].annotations(), QList<QByteArray>());
		X_EQUAL(f[3], BedLine("chr3", 260, 270));
		X_EQUAL(f[3].annotations(), QList<QByteArray>());
		X_EQUAL(f[4], BedLine("chr4", 300, 315));
		X_EQUAL(f[4].annotations(), QList<QByteArray>() << "1" << "2" << "3");
		X_EQUAL(f[5], BedLine("chr4", 316, 330));
		X_EQUAL(f[5].annotations(), QList<QByteArray>() << "1" << "2" << "3");
		/* further tests are skipped
		chr5 size=41 chunks=(13, 14, 14)
		chr6 size=51 chunks=(17, 17, 17)
		chr7 size=61 chunks=(16, 15, 15, 15)
		chr8 size=71 chunks=(14, 14, 14, 14, 15)
		chr9 size=81 chunks=(17, 16, 16, 16, 16)
		chr10 size=91 chunks=(16, 15, 15, 15, 15, 15)
		*/

		//second test: load file, chunk, merge, check that the file is still the same...
		f.load(TESTDATA("data_in/demo.bed"));
		f.merge();
		I_EQUAL(f.count(), 591);

		BedFile f2 = f;
		f2.chunk(15);
		I_EQUAL(f2.count(), 6119);
		f2.merge();
		I_EQUAL(f2.count(), 591);
		for (int i=0; i<f.count(); ++i)
		{
			X_EQUAL(f2[i], f[i]);
		}
	}

	void chromosomes()
	{
		BedFile f;
		QSet<Chromosome> expected;

		f.append(BedLine("chr1", 100, 110));
		expected.insert(Chromosome("chr1"));
		X_EQUAL(f.chromosomes(), expected);

		f.append(BedLine("chr2", 100, 110));
		expected.insert(Chromosome("chr2"));
		X_EQUAL(f.chromosomes(), expected);

		f.append(BedLine("chr2", 200, 210));
		X_EQUAL(f.chromosomes(), expected);

		f.append(BedLine("chr1", 200, 210));
		X_EQUAL(f.chromosomes(), expected);

		f.append(BedLine("chr3", 100, 110));
		expected.insert(Chromosome("chr3"));
		X_EQUAL(f.chromosomes(), expected);
	}

	void fromText()
	{
		BedFile f = BedFile::fromText("#bla\n#track name='dummy'\nchr1\t0\t99\n\nchr2\t0\t99");
		I_EQUAL(f.headers().count(), 2);
		S_EQUAL(f.headers()[0], "#bla");
		S_EQUAL(f.headers()[1], "#track name='dummy'");
		I_EQUAL(f.count(), 2);
		I_EQUAL(f.baseCount(), 198);
	}
};
