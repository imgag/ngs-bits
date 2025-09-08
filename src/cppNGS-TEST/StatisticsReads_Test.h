#include "TestFramework.h"
#include "StatisticsReads.h"

TEST_CLASS(StatisticsReads_Test)
{
private:

	void reads()
	{
		StatisticsReads stats;

		FastqEntry e;
		FastqFileStream stream(TESTDATA("data_in/example6.fastq.gz"), false);
		while(!stream.atEnd())
		{
			stream.readEntry(e);
			stats.update(e, StatisticsReads::FORWARD);
		}
		FastqFileStream stream2(TESTDATA("data_in/example7.fastq.gz"), false);
		while(!stream2.atEnd())
		{
			stream2.readEntry(e);
			stats.update(e, StatisticsReads::REVERSE);
		}

		QCCollection result = stats.getResult();
		S_EQUAL(result[0].name(), QString("read count"));
		S_EQUAL(result[0].toString(), QString("5000"));
		S_EQUAL(result[1].name(), QString("read length"));
		S_EQUAL(result[1].toString(), QString("151"));
        S_EQUAL(result[2].name(), QString("bases sequenced (MB)"));
        S_EQUAL(result[2].toString(), QString("0.76"));
        S_EQUAL(result[3].name(), QString("Q20 read percentage"));
        S_EQUAL(result[3].toString(), QString("99.40"));
		S_EQUAL(result[4].name(), QString("Q20 base percentage"));
		S_EQUAL(result[4].toString(), QString("96.77"));
		S_EQUAL(result[5].name(), QString("Q30 base percentage"));
		S_EQUAL(result[5].toString(), QString("96.30"));
		S_EQUAL(result[6].name(), QString("no base call percentage"));
		S_EQUAL(result[6].toString(), QString("0.00"));
		S_EQUAL(result[7].name(), QString("gc content percentage"));
		S_EQUAL(result[7].toString(), QString("46.26"));
		S_EQUAL(result[8].name(), QString("base distribution plot"));
		IS_TRUE(result[8].type()==QCValueType::IMAGE);
		S_EQUAL(result[9].name(), QString("Q score plot"));
		IS_TRUE(result[9].type()==QCValueType::IMAGE);
		S_EQUAL(result[10].name(), QString("read Q score distribution"));
		IS_TRUE(result[10].type()==QCValueType::IMAGE);
		I_EQUAL(result.count(), 11);

		//check that there is a description for each term
		for (int i=0; i<result.count(); ++i)
		{
			IS_TRUE(result[i].description()!="");
		}
	}
};
