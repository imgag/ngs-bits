#include "TestFramework.h"
#include "Chromosome.h"

TEST_CLASS(Chromosome_Test)
{
private:

	TEST_METHOD(str)
	{
		S_EQUAL(Chromosome("1").str(), QByteArray("1"));
		S_EQUAL(Chromosome("chr1").str(), QByteArray("chr1"));
		S_EQUAL(Chromosome("chr1").str(), QByteArray("chr1"));
		S_EQUAL(Chromosome("CHRX").str(), QByteArray("CHRX"));
		S_EQUAL(Chromosome("chrY").str(), QByteArray("chrY"));
		S_EQUAL(Chromosome("M").str(), QByteArray("M"));
		S_EQUAL(Chromosome("MT").str(), QByteArray("MT"));

		S_EQUAL(Chromosome(QString("1")).str(), QByteArray("1"));
		S_EQUAL(Chromosome(QByteArray("1")).str(), QByteArray("1"));
	}

	TEST_METHOD(strNormalized)
	{
		S_EQUAL(Chromosome("1").strNormalized(false), QByteArray("1"));
		S_EQUAL(Chromosome("chr1").strNormalized(false), QByteArray("1"));
		S_EQUAL(Chromosome("chr01").strNormalized(false), QByteArray("01"));
		S_EQUAL(Chromosome("CHRX").strNormalized(false), QByteArray("X"));
		S_EQUAL(Chromosome("chrY").strNormalized(false), QByteArray("Y"));
		S_EQUAL(Chromosome("M").strNormalized(false), QByteArray("MT"));
        S_EQUAL(Chromosome("MT").strNormalized(false), QByteArray("MT"));

		S_EQUAL(Chromosome("1").strNormalized(true),  QByteArray("chr1"));
		S_EQUAL(Chromosome("chr1").strNormalized(true), QByteArray("chr1"));
		S_EQUAL(Chromosome("CHRX").strNormalized(true), QByteArray("chrX"));
		S_EQUAL(Chromosome("chrY").strNormalized(true), QByteArray("chrY"));
		S_EQUAL(Chromosome("M").strNormalized(true), QByteArray("chrMT"));
        S_EQUAL(Chromosome("MT").strNormalized(true), QByteArray("chrMT"));
	}

	TEST_METHOD(num)
	{
		I_EQUAL(Chromosome("1").num(), 1);
		I_EQUAL(Chromosome("chr1").num(),1);
		I_EQUAL(Chromosome("chr01").num(),1);
		I_EQUAL(Chromosome("X").num(),1001);
		I_EQUAL(Chromosome("CHRX").num(),1001);
		I_EQUAL(Chromosome("Y").num(),1002);
		I_EQUAL(Chromosome("chrY").num(),1002);
		I_EQUAL(Chromosome("M").num(),1003);
		I_EQUAL(Chromosome("MT").num(),1003);

		//chromosomes without fixed numbers
		int base = Chromosome("chrBLA").num();
		IS_TRUE(base >= 1004);
		I_EQUAL(Chromosome("chrBLA2").num(),base+1);
		I_EQUAL(Chromosome("BLA").num(),base);
		I_EQUAL(Chromosome("BLA2").num(),base+1);
	}

	TEST_METHOD(isNonSpecial)
	{
		IS_TRUE(Chromosome("1").isNonSpecial());
		IS_TRUE(Chromosome("chr1").isNonSpecial());
		IS_TRUE(Chromosome("chr01").isNonSpecial());
		IS_TRUE(Chromosome("X").isNonSpecial());
		IS_TRUE(Chromosome("CHRX").isNonSpecial());
		IS_TRUE(Chromosome("Y").isNonSpecial());
		IS_TRUE(Chromosome("chrY").isNonSpecial());
		IS_TRUE(Chromosome("M").isNonSpecial());
		IS_TRUE(Chromosome("MT").isNonSpecial());

		IS_FALSE(Chromosome("chrBLA2").isNonSpecial());
		IS_FALSE(Chromosome("BLA").isNonSpecial());
		IS_FALSE(Chromosome("BLA2").isNonSpecial());
	}

	TEST_METHOD(isAutosome)
	{
		IS_TRUE(!Chromosome("").isAutosome());
		IS_TRUE(!Chromosome("X").isAutosome());
		IS_TRUE(!Chromosome("Y").isAutosome());
		IS_TRUE(!Chromosome("M").isAutosome());
		IS_TRUE(!Chromosome("BLA").isAutosome());

		IS_TRUE(Chromosome("1").isAutosome());
		IS_TRUE(Chromosome("2").isAutosome());
		IS_TRUE(Chromosome("10").isAutosome());
		IS_TRUE(Chromosome("20").isAutosome());
		IS_TRUE(Chromosome("22").isAutosome());
		IS_TRUE(Chromosome("100").isAutosome());
	}

	TEST_METHOD(isGonosome)
	{
		IS_TRUE(Chromosome("X").isGonosome());
		IS_TRUE(Chromosome("Y").isGonosome());

		IS_TRUE(!Chromosome("").isGonosome());
		IS_TRUE(!Chromosome("M").isGonosome());
		IS_TRUE(!Chromosome("BLA").isGonosome());
		IS_TRUE(!Chromosome("1").isGonosome());
	}

	TEST_METHOD(isX)
	{
		IS_TRUE(Chromosome("X").isX());

		IS_TRUE(!Chromosome("Y").isX());
		IS_TRUE(!Chromosome("").isX());
		IS_TRUE(!Chromosome("M").isX());
		IS_TRUE(!Chromosome("BLA").isX());
		IS_TRUE(!Chromosome("1").isX());
	}


	TEST_METHOD(isY)
	{
		IS_TRUE(Chromosome("y").isY());

		IS_TRUE(!Chromosome("X").isY());
		IS_TRUE(!Chromosome("").isY());
		IS_TRUE(!Chromosome("M").isY());
		IS_TRUE(!Chromosome("BLA").isY());
		IS_TRUE(!Chromosome("1").isY());
	}


	TEST_METHOD(isM)
	{
		IS_TRUE(Chromosome("M").isM());

		IS_TRUE(!Chromosome("X").isM());
		IS_TRUE(!Chromosome("Y").isM());
		IS_TRUE(!Chromosome("").isM());
		IS_TRUE(!Chromosome("BLA").isM());
		IS_TRUE(!Chromosome("1").isM());
	}

};
