#include "TestFramework.h"
#include "BedFile.h"

TEST_CLASS(BedLine_Test)
{
private:

	TEST_METHOD(isValid)
	{
		BedLine line1;
		IS_FALSE(line1.isValid());

		BedLine line2("chr1", 5, 10);
		IS_TRUE(line2.isValid());
	}

	TEST_METHOD(overlapsWithLine)
	{
		BedLine line1("chr1", 5, 10);
		IS_FALSE(line1.overlapsWith(BedLine("chr2", 5, 10)));
		IS_FALSE(line1.overlapsWith(BedLine("chr1", 1, 4)));
		IS_FALSE(line1.overlapsWith(BedLine("chr1", 11, 20)));
		IS_TRUE(line1.overlapsWith(BedLine("chr1", 1, 5)));
		IS_TRUE(line1.overlapsWith(BedLine("chr1", 5, 10)));
		IS_TRUE(line1.overlapsWith(BedLine("chr1", 6, 8)));
		IS_TRUE(line1.overlapsWith(BedLine("chr1", 10, 20)));
		IS_TRUE(line1.overlapsWith(BedLine("chr1", 1, 20)));
	}

	TEST_METHOD(overlapsWithChrStartEnd)
	{
		BedLine line1("chr1", 5, 10);
		IS_FALSE(line1.overlapsWith("chr2", 5, 10));
		IS_FALSE(line1.overlapsWith("chr1", 1, 4));
		IS_FALSE(line1.overlapsWith("chr1", 11, 20));
		IS_TRUE(line1.overlapsWith("chr1", 1, 5));
		IS_TRUE(line1.overlapsWith("chr1", 5, 10));
		IS_TRUE(line1.overlapsWith("chr1", 6, 8));
		IS_TRUE(line1.overlapsWith("chr1", 10, 20));
		IS_TRUE(line1.overlapsWith("chr1", 1, 20));
	}

	TEST_METHOD(overlapsWithPosition)
	{
		BedLine line1("chr1", 5, 10);
		IS_TRUE(line1.overlapsWith(5, 10));
		IS_FALSE(line1.overlapsWith(1, 4));
		IS_FALSE(line1.overlapsWith(11, 20));
		IS_TRUE(line1.overlapsWith(1, 5));
		IS_TRUE(line1.overlapsWith(5, 10));
		IS_TRUE(line1.overlapsWith(6, 8));
		IS_TRUE(line1.overlapsWith(10, 20));
		IS_TRUE(line1.overlapsWith(1, 20));
	}

	TEST_METHOD(adjacentToComplete)
	{
		BedLine line1("chr1", 5, 10);
		IS_TRUE(!line1.adjacentTo("chr2", 1, 4));
		IS_TRUE(!line1.adjacentTo("chr1", 1, 3));
		IS_TRUE(!line1.adjacentTo("chr1", 12, 15));
		IS_TRUE(line1.adjacentTo("chr1", 11, 20));
		IS_TRUE(line1.adjacentTo("chr1", 1, 4));
	}

	TEST_METHOD(adjacentToPosition)
	{
		BedLine line1("chr1", 5, 10);
		IS_TRUE(!line1.adjacentTo(1, 3));
		IS_TRUE(!line1.adjacentTo(12, 15));
		IS_TRUE(line1.adjacentTo(11, 20));
		IS_TRUE(line1.adjacentTo(1, 4));
	}

	TEST_METHOD(operator_lessthan)
	{
		IS_FALSE(BedLine("chr1", 1, 20) < BedLine("chr1", 1, 20));
		IS_TRUE(BedLine("chr1", 1, 20) < BedLine("chr1", 5, 20));
		IS_FALSE(BedLine("chr2", 1, 20) < BedLine("chr1", 1, 20));
		IS_TRUE(BedLine("chr1", 1, 20) < BedLine("chr2", 5, 20));
	}

	TEST_METHOD(fromString_toString)
	{
		QString line = "chr1\t1\t20";
		S_EQUAL(line, BedLine::fromString(line).toStringWithAnnotations());
		line = "chr15\t3589921\t3699921";
		S_EQUAL(line, BedLine::fromString(line).toStringWithAnnotations());
		line = "chr1\t1\t20\tGene";
		S_EQUAL(line, BedLine::fromString(line).toStringWithAnnotations());
		line = "chr1\t1\t20\tGene\ttwo_Anno\tthree,Anno";
		S_EQUAL(line, BedLine::fromString(line).toStringWithAnnotations());
	}

};
