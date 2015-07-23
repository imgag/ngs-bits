#include "TestFramework.h"
#include "BedFile.h"

TEST_CLASS(BedLine_Test)
{
Q_OBJECT
private slots:

	void overlapsWithComplete()
	{
		BedLine line1("chr1", 5, 10);
		IS_TRUE(!line1.overlapsWith("chr2", 5, 10));
		IS_TRUE(!line1.overlapsWith("chr1", 1, 4));
		IS_TRUE(!line1.overlapsWith("chr1", 11, 20));
		IS_TRUE(line1.overlapsWith("chr1", 1, 5));
		IS_TRUE(line1.overlapsWith("chr1", 5, 10));
		IS_TRUE(line1.overlapsWith("chr1", 6, 8));
		IS_TRUE(line1.overlapsWith("chr1", 10, 20));
		IS_TRUE(line1.overlapsWith("chr1", 1, 20));
	}

	void overlapsWithPosition()
	{
		BedLine line1("chr1", 5, 10);
		IS_TRUE(line1.overlapsWith(5, 10));
		IS_TRUE(!line1.overlapsWith(1, 4));
		IS_TRUE(!line1.overlapsWith(11, 20));
		IS_TRUE(line1.overlapsWith(1, 5));
		IS_TRUE(line1.overlapsWith(5, 10));
		IS_TRUE(line1.overlapsWith(6, 8));
		IS_TRUE(line1.overlapsWith(10, 20));
		IS_TRUE(line1.overlapsWith(1, 20));
	}

	void adjacentToComplete()
	{
		BedLine line1("chr1", 5, 10);
		IS_TRUE(!line1.adjacentTo("chr2", 1, 4));
		IS_TRUE(!line1.adjacentTo("chr1", 1, 3));
		IS_TRUE(!line1.adjacentTo("chr1", 12, 15));
		IS_TRUE(line1.adjacentTo("chr1", 11, 20));
		IS_TRUE(line1.adjacentTo("chr1", 1, 4));
	}

	void adjacentToPosition()
	{
		BedLine line1("chr1", 5, 10);
		IS_TRUE(!line1.adjacentTo(1, 3));
		IS_TRUE(!line1.adjacentTo(12, 15));
		IS_TRUE(line1.adjacentTo(11, 20));
		IS_TRUE(line1.adjacentTo(1, 4));
	}

	void operator_lessthan()
	{
		IS_FALSE(BedLine("chr1", 1, 20) < BedLine("chr1", 1, 20));
		IS_TRUE(BedLine("chr1", 1, 20) < BedLine("chr1", 5, 20));
		IS_FALSE(BedLine("chr2", 1, 20) < BedLine("chr1", 1, 20));
		IS_TRUE(BedLine("chr1", 1, 20) < BedLine("chr2", 5, 20));
	}

};
