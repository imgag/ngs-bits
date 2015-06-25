#include "../TestFramework.h"
#include "StatisticsReads.h"

class StatisticsReads_Test
		: public QObject
{
	Q_OBJECT

private slots:

	void reads()
	{
		StatisticsReads stats;

		FastqEntry e;
		FastqFileStream stream(QFINDTESTDATA("../tools-TEST/data_in/ReadQC_in1.fastq.gz"), false);
		while(!stream.atEnd())
		{
			stream.readEntry(e);
			stats.update(e, StatisticsReads::FORWARD);
		}
		FastqFileStream stream2(QFINDTESTDATA("../tools-TEST/data_in/ReadQC_in2.fastq.gz"), false);
		while(!stream2.atEnd())
		{
			stream2.readEntry(e);
			stats.update(e, StatisticsReads::REVERSE);
		}

		QCCollection result = stats.getResult();
		QCOMPARE(result[0].name(), QString("read count"));
		QCOMPARE(result[0].toString(), QString("25000"));
		QCOMPARE(result[1].name(), QString("read length"));
		QCOMPARE(result[1].toString(), QString("151"));
		QCOMPARE(result[2].name(), QString("Q20 read percentage"));
		QCOMPARE(result[2].toString(), QString("99.32"));
		QCOMPARE(result[3].name(), QString("Q30 base percentage"));
		QCOMPARE(result[3].toString(), QString("96.52"));
		QCOMPARE(result[4].name(), QString("no base call percentage"));
		QCOMPARE(result[4].toString(), QString("0.00"));
		QCOMPARE(result[5].name(), QString("gc content percentage"));
		QCOMPARE(result[5].toString(), QString("46.22"));
		QCOMPARE(result[6].name(), QString("base distribution plot"));
		QVERIFY(result[6].type()==QVariant::ByteArray);
		QCOMPARE(result[7].name(), QString("Q score plot"));
		QVERIFY(result[7].type()==QVariant::ByteArray);
		QCOMPARE(result.count(), 8);

		//check that there is a description for each term
		for (int i=0; i<result.count(); ++i)
		{
			QVERIFY(result[i].description()!="");
		}
	}
};

TFW_DECLARE(StatisticsReads_Test)

