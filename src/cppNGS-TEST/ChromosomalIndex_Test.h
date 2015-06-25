#include "../TestFramework.h"
#include "ChromosomalIndex.h"
#include "BedFile.h"
#include "VariantList.h"
#include "Log.h"

class ChromosomalIndex_Test
		: public QObject
{
	Q_OBJECT

private slots:

	void matchingIndices_BedFile()
	{
		BedFile bed_file;
		for (int c=1; c<=22; ++c)
		{
			for (int p=1; p<=100*c; ++p)
			{
				BedLine line ("chr" + QString::number(c), p, p);
				if (p%10==0) line.setEnd(p + 10);
				bed_file.append(line);
			}
		}
		ChromosomalIndex<BedFile> bed_index(bed_file);

		//chromosome not found
		QVector<int> elements = bed_index.matchingIndices("chrX", 5, 15);
		QCOMPARE(elements.count(), 0);

		//whole chr1
		elements = bed_index.matchingIndices("chr1", 0, 100000);
		QCOMPARE(elements.count(), 100);

		//3 elements
		elements = bed_index.matchingIndices("chr1", 5, 7);
		QCOMPARE(elements.count(), 3);

		//1 element
		elements = bed_index.matchingIndices("chr1", 5, 5);
		QCOMPARE(elements.count(), 1);

		//whole chr2
		elements = bed_index.matchingIndices("chr2", 0, 100000);
		QCOMPARE(elements.count(), 200);

		//5 elements
		elements = bed_index.matchingIndices("chr2", 1, 5);
		QCOMPARE(elements.count(), 5);

		//overlap with beginning
		elements = bed_index.matchingIndices("chr2", -10, 5);
		QCOMPARE(elements.count(), 5);

		//overlap with end
		elements = bed_index.matchingIndices("chr2", 200, 205);
		QCOMPARE(elements.count(), 2);

		//no overlap
		elements = bed_index.matchingIndices("chr2", 500, 505);
		QCOMPARE(elements.count(), 0);
	}


	void matchingIndex_BedFile()
	{
		BedFile bed_file;
		for (int c=1; c<=22; ++c)
		{
			for (int p=1; p<=100*c; ++p)
			{
				BedLine line ("chr" + QString::number(c), p, p);
				if (p%10==0) line.setEnd(p + 10);
				bed_file.append(line);
			}
		}
		ChromosomalIndex<BedFile> bed_index(bed_file);

		//chromosome not found
		int index = bed_index.matchingIndex("chrX", 5, 15);
		QCOMPARE(index, -1);

		//whole chr1
		index = bed_index.matchingIndex("chr1", 0, 100000);
		QCOMPARE(index, 0);

		//3 elements
		index = bed_index.matchingIndex("chr1", 5, 7);
		QCOMPARE(index, 4);

		//1 element
		index = bed_index.matchingIndex("chr1", 5, 5);
		QCOMPARE(index, 4);

		//whole chr2
		index = bed_index.matchingIndex("chr2", 0, 100000);
		QCOMPARE(index, 100);

		//5 elements
		index = bed_index.matchingIndex("chr2", 1, 5);
		QCOMPARE(index, 100);

		//overlap with beginning
		index = bed_index.matchingIndex("chr2", -10, 5);
		QCOMPARE(index, 100);

		//overlap with end
		index = bed_index.matchingIndex("chr2", 200, 205);
		QCOMPARE(index, 289);
		QCOMPARE(bed_file[index].chr(), Chromosome("chr2"));
		QCOMPARE(bed_file[index].start(), 190);
		QCOMPARE(bed_file[index].end(), 200);

		//no overlap
		index = bed_index.matchingIndex("chr2", 500, 505);
		QCOMPARE(index, -1);
	}

	void matchingIndices_VariantList()
	{
		VariantList var_list;
		for (int c=1; c<=5; ++c)
		{
			for (int p=1; p<=100*c; ++p)
			{
				Variant variant("chr" + QString::number(c), p, p+10, "A", "G");
				var_list.append(variant);
			}
		}
		ChromosomalIndex<VariantList> var_index(var_list);

		//chromosome not found
		QVector<int> elements = var_index.matchingIndices("chrX", 5, 15);
		QCOMPARE(elements.count(), 0);

		//whole chr1
		elements = var_index.matchingIndices("chr1", 0, 100000);
		QCOMPARE(elements.count(), 100);

		//7 elements
		elements = var_index.matchingIndices("chr1", 5, 7);
		QCOMPARE(elements.count(), 7);

		//1 element
		elements = var_index.matchingIndices("chr1", 1, 1);
		QCOMPARE(elements.count(), 1);

		//whole chr2
		elements = var_index.matchingIndices("chr2", 0, 100000);
		QCOMPARE(elements.count(), 200);

		//5 elements
		elements = var_index.matchingIndices("chr2", 1, 5);
		QCOMPARE(elements.count(), 5);

		//overlap with beginning
		elements = var_index.matchingIndices("chr2", -10, 5);
		QCOMPARE(elements.count(), 5);

		//overlap with end
		elements = var_index.matchingIndices("chr2", 200, 205);
		QCOMPARE(elements.count(), 11);

		//not overlap
		elements = var_index.matchingIndices("chr2", 500, 505);
		QCOMPARE(elements.count(), 0);

	}

	void matchingIndex_VariantList()
	{
		VariantList var_list;
		for (int c=1; c<=5; ++c)
		{
			for (int p=1; p<=100*c; ++p)
			{
				Variant variant("chr" + QString::number(c), p, p+10, "A", "G");
				var_list.append(variant);
			}
		}
		ChromosomalIndex<VariantList> var_index(var_list);

		//chromosome not found
		int index = var_index.matchingIndex("chrX", 5, 15);
		QCOMPARE(index, -1);

		//whole chr1
		index = var_index.matchingIndex("chr1", 0, 100000);
		QCOMPARE(index, 0);

		//7 elements
		index = var_index.matchingIndex("chr1", 5, 7);
		QCOMPARE(index, 0);

		//1 element
		index = var_index.matchingIndex("chr1", 1, 1);
		QCOMPARE(index, 0);

		//whole chr2
		index = var_index.matchingIndex("chr2", 0, 100000);
		QCOMPARE(index, 100);

		//no overlap
		index = var_index.matchingIndex("chr2", 500, 505);
		QCOMPARE(index, -1);
	}

	///bin size benchmark
	void benchmarkBinSizes()
	{
		return; //activate in release mode only!

		BedFile file;
		file.load(QFINDTESTDATA("../tools-TEST/data_in/exome.bed"));

		QList<int> sizes;
		sizes << 5 << 10 << 20 << 30 << 40 << 50 << 100 << 200 << 400 << 800;
		foreach(int bin_size, sizes)
		{
			QTime timer;
			timer.start();
			ChromosomalIndex<BedFile> idx(file, bin_size);
			for (int i=0; i<file.count(); ++i)
			{
				idx.matchingIndices(file[i].chr(), file[i].start(), file[i].end());
			}
			Log::perf(QString::number(bin_size) + ":", timer);
		}
	}

};

TFW_DECLARE(ChromosomalIndex_Test)
