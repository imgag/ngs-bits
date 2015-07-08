#include "TestFramework.h"
#include "Chromosome.h"

class Chromosome_Test
		: public QObject
{
	Q_OBJECT

private slots:

	void str()
	{
		QCOMPARE(Chromosome("1").str(), QByteArray("1"));
		QCOMPARE(Chromosome("chr1").str(), QByteArray("chr1"));
		QCOMPARE(Chromosome("CHRX").str(), QByteArray("CHRX"));
		QCOMPARE(Chromosome("chrY").str(), QByteArray("chrY"));
		QCOMPARE(Chromosome("M").str(), QByteArray("M"));
		QCOMPARE(Chromosome("MT").str(), QByteArray("MT"));

		QCOMPARE(Chromosome(QString("1")).str(), QByteArray("1"));
		QCOMPARE(Chromosome(QByteArray("1")).str(), QByteArray("1"));
	}

	void strNormalized()
	{
		QCOMPARE(Chromosome("1").strNormalized(false), QByteArray("1"));
		QCOMPARE(Chromosome("chr1").strNormalized(false), QByteArray("1"));
		QCOMPARE(Chromosome("CHRX").strNormalized(false), QByteArray("X"));
		QCOMPARE(Chromosome("chrY").strNormalized(false), QByteArray("Y"));
		QCOMPARE(Chromosome("M").strNormalized(false), QByteArray("M"));
		QCOMPARE(Chromosome("MT").strNormalized(false), QByteArray("M"));

		QCOMPARE(Chromosome("1").strNormalized(true),  QByteArray("chr1"));
		QCOMPARE(Chromosome("chr1").strNormalized(true), QByteArray("chr1"));
		QCOMPARE(Chromosome("CHRX").strNormalized(true), QByteArray("chrX"));
		QCOMPARE(Chromosome("chrY").strNormalized(true), QByteArray("chrY"));
		QCOMPARE(Chromosome("M").strNormalized(true), QByteArray("chrM"));
		QCOMPARE(Chromosome("MT").strNormalized(true), QByteArray("chrM"));
	}

	void num()
	{
		QCOMPARE(Chromosome("1").num(), 1);
		QCOMPARE(Chromosome("chr1").num(),1);
		QCOMPARE(Chromosome("X").num(),1001);
		QCOMPARE(Chromosome("CHRX").num(),1001);
		QCOMPARE(Chromosome("Y").num(),1002);
		QCOMPARE(Chromosome("chrY").num(),1002);
		QCOMPARE(Chromosome("M").num(),1003);
		QCOMPARE(Chromosome("MT").num(),1003);

		//chromosomes without fixed numbers
		int base = Chromosome("chrBLA").num();
		QVERIFY(base >= 1004);
		QCOMPARE(Chromosome("chrBLA2").num(),base+1);
		QCOMPARE(Chromosome("BLA").num(),base);
		QCOMPARE(Chromosome("BLA2").num(),base+1);
	}

	void isAutosome()
	{
		QVERIFY(!Chromosome("").isAutosome());
		QVERIFY(!Chromosome("X").isAutosome());
		QVERIFY(!Chromosome("Y").isAutosome());
		QVERIFY(!Chromosome("M").isAutosome());
		QVERIFY(!Chromosome("BLA").isAutosome());

		QVERIFY(Chromosome("1").isAutosome());
		QVERIFY(Chromosome("2").isAutosome());
		QVERIFY(Chromosome("10").isAutosome());
		QVERIFY(Chromosome("20").isAutosome());
		QVERIFY(Chromosome("22").isAutosome());
		QVERIFY(Chromosome("100").isAutosome());
	}

	void isGonosome()
	{
		QVERIFY(Chromosome("X").isGonosome());
		QVERIFY(Chromosome("Y").isGonosome());

		QVERIFY(!Chromosome("").isGonosome());
		QVERIFY(!Chromosome("M").isGonosome());
		QVERIFY(!Chromosome("BLA").isGonosome());
		QVERIFY(!Chromosome("1").isGonosome());
	}

	void isX()
	{
		QVERIFY(Chromosome("X").isX());

		QVERIFY(!Chromosome("Y").isX());
		QVERIFY(!Chromosome("").isX());
		QVERIFY(!Chromosome("M").isX());
		QVERIFY(!Chromosome("BLA").isX());
		QVERIFY(!Chromosome("1").isX());
	}


	void isY()
	{
		QVERIFY(Chromosome("y").isY());

		QVERIFY(!Chromosome("X").isY());
		QVERIFY(!Chromosome("").isY());
		QVERIFY(!Chromosome("M").isY());
		QVERIFY(!Chromosome("BLA").isY());
		QVERIFY(!Chromosome("1").isY());
	}


	void isM()
	{
		QVERIFY(Chromosome("M").isM());

		QVERIFY(!Chromosome("X").isM());
		QVERIFY(!Chromosome("Y").isM());
		QVERIFY(!Chromosome("").isM());
		QVERIFY(!Chromosome("BLA").isM());
		QVERIFY(!Chromosome("1").isM());
	}

};

TFW_DECLARE(Chromosome_Test)
