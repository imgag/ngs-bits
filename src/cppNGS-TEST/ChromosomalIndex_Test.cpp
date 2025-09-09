#include "TestFramework.h"
#include "ChromosomalIndex.h"
#include "BedFile.h"
#include "VcfFile.h"
#include "Log.h"

TEST_CLASS(ChromosomalIndex_Test)
{
private:

	TEST_METHOD(matchingIndices_BedFile)
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
		I_EQUAL(elements.count(), 0);

		//whole chr1
		elements = bed_index.matchingIndices("chr1", 0, 100000);
		I_EQUAL(elements.count(), 100);

		//3 elements
		elements = bed_index.matchingIndices("chr1", 5, 7);
		I_EQUAL(elements.count(), 3);

		//1 element
		elements = bed_index.matchingIndices("chr1", 5, 5);
		I_EQUAL(elements.count(), 1);

		//whole chr2
		elements = bed_index.matchingIndices("chr2", 0, 100000);
		I_EQUAL(elements.count(), 200);

		//5 elements
		elements = bed_index.matchingIndices("chr2", 1, 5);
		I_EQUAL(elements.count(), 5);

		//overlap with beginning
		elements = bed_index.matchingIndices("chr2", -10, 5);
		I_EQUAL(elements.count(), 5);

		//overlap with end
		elements = bed_index.matchingIndices("chr2", 200, 205);
		I_EQUAL(elements.count(), 2);

		//no overlap
		elements = bed_index.matchingIndices("chr2", 500, 505);
		I_EQUAL(elements.count(), 0);
	}


	TEST_METHOD(matchingIndex_BedFile)
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
		I_EQUAL(index, -1);

		//whole chr1
		index = bed_index.matchingIndex("chr1", 0, 100000);
		I_EQUAL(index, 0);

		//3 elements
		index = bed_index.matchingIndex("chr1", 5, 7);
		I_EQUAL(index, 4);

		//1 element
		index = bed_index.matchingIndex("chr1", 5, 5);
		I_EQUAL(index, 4);

		//whole chr2
		index = bed_index.matchingIndex("chr2", 0, 100000);
		I_EQUAL(index, 100);

		//5 elements
		index = bed_index.matchingIndex("chr2", 1, 5);
		I_EQUAL(index, 100);

		//overlap with beginning
		index = bed_index.matchingIndex("chr2", -10, 5);
		I_EQUAL(index, 100);

		//overlap with end
		index = bed_index.matchingIndex("chr2", 200, 205);
		I_EQUAL(index, 289);
		X_EQUAL(bed_file[index].chr(), Chromosome("chr2"));
		I_EQUAL(bed_file[index].start(), 190);
		I_EQUAL(bed_file[index].end(), 200);

		//no overlap
		index = bed_index.matchingIndex("chr2", 500, 505);
		I_EQUAL(index, -1);
	}

	TEST_METHOD(matchingIndices_VariantList)
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
		I_EQUAL(elements.count(), 0);

		//whole chr1
		elements = var_index.matchingIndices("chr1", 0, 100000);
		I_EQUAL(elements.count(), 100);

		//7 elements
		elements = var_index.matchingIndices("chr1", 5, 7);
		I_EQUAL(elements.count(), 7);

		//1 element
		elements = var_index.matchingIndices("chr1", 1, 1);
		I_EQUAL(elements.count(), 1);

		//whole chr2
		elements = var_index.matchingIndices("chr2", 0, 100000);
		I_EQUAL(elements.count(), 200);

		//5 elements
		elements = var_index.matchingIndices("chr2", 1, 5);
		I_EQUAL(elements.count(), 5);

		//overlap with beginning
		elements = var_index.matchingIndices("chr2", -10, 5);
		I_EQUAL(elements.count(), 5);

		//overlap with end
		elements = var_index.matchingIndices("chr2", 200, 205);
		I_EQUAL(elements.count(), 11);

		//not overlap
		elements = var_index.matchingIndices("chr2", 500, 505);
		I_EQUAL(elements.count(), 0);

	}

	TEST_METHOD(matchingIndex_VariantList)
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
		I_EQUAL(index, -1);

		//whole chr1
		index = var_index.matchingIndex("chr1", 0, 100000);
		I_EQUAL(index, 0);

		//7 elements
		index = var_index.matchingIndex("chr1", 5, 7);
		I_EQUAL(index, 0);

		//1 element
		index = var_index.matchingIndex("chr1", 1, 1);
		I_EQUAL(index, 0);

		//whole chr2
		index = var_index.matchingIndex("chr2", 0, 100000);
		I_EQUAL(index, 100);

		//no overlap
		index = var_index.matchingIndex("chr2", 500, 505);
		I_EQUAL(index, -1);
	}

	TEST_METHOD(matchingIndices_VcfFile)
	{
		VcfFile var_list;
		for (int c=1; c<=5; ++c)
		{
			for (int p=1; p<=100*c; ++p)
			{
				VcfLine variant(Chromosome("chr" + QString::number(c)), p, "AAAAAAAAAAA", QList<Sequence>() << "AG");
				var_list.append(variant);
			}
		}
		ChromosomalIndex<VcfFile> var_index(var_list);

		//chromosome not found
		QVector<int> elements = var_index.matchingIndices("chrX", 5, 15);
		I_EQUAL(elements.count(), 0);

		//whole chr1
		elements = var_index.matchingIndices("chr1", 0, 100000);
		I_EQUAL(elements.count(), 100);

		//7 elements
		elements = var_index.matchingIndices("chr1", 5, 7);
		I_EQUAL(elements.count(), 7);

		//1 element
		elements = var_index.matchingIndices("chr1", 1, 1);
		I_EQUAL(elements.count(), 1);

		//whole chr2
		elements = var_index.matchingIndices("chr2", 0, 100000);
		I_EQUAL(elements.count(), 200);

		//5 elements
		elements = var_index.matchingIndices("chr2", 1, 5);
		I_EQUAL(elements.count(), 5);

		//overlap with beginning
		elements = var_index.matchingIndices("chr2", -10, 5);
		I_EQUAL(elements.count(), 5);

		//overlap with end
		elements = var_index.matchingIndices("chr2", 200, 205);
		I_EQUAL(elements.count(), 11);

		//not overlap
		elements = var_index.matchingIndices("chr2", 500, 505);
		I_EQUAL(elements.count(), 0);

	}

	TEST_METHOD(matchingIndex_VcfFile)
	{
		VcfFile var_list;
		for (int c=1; c<=5; ++c)
		{
			for (int p=1; p<=100*c; ++p)
			{
				VcfLine variant(Chromosome("chr" + QString::number(c)), p, "AAAAAAAAAAA", QList<Sequence>() << "AG");
				var_list.append(variant);
			}
		}
		ChromosomalIndex<VcfFile> var_index(var_list);

		//chromosome not found
		int index = var_index.matchingIndex("chrX", 5, 15);
		I_EQUAL(index, -1);

		//whole chr1
		index = var_index.matchingIndex("chr1", 0, 100000);
		I_EQUAL(index, 0);

		//7 elements
		index = var_index.matchingIndex("chr1", 5, 7);
		I_EQUAL(index, 0);

		//1 element
		index = var_index.matchingIndex("chr1", 1, 1);
		I_EQUAL(index, 0);

		//whole chr2
		index = var_index.matchingIndex("chr2", 0, 100000);
		I_EQUAL(index, 100);

		//no overlap
		index = var_index.matchingIndex("chr2", 500, 505);
		I_EQUAL(index, -1);
	}
};
