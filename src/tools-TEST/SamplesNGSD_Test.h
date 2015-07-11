#include "TestFramework.h"
#include "Helper.h"
#include "Settings.h"

TEST_CLASS(SamplesNGSD_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		QString host = Settings::string("ngsd_host");
		if (host=="") QSKIP("Test needs access to the NGSD!");

		TFW_EXEC("SamplesNGSD", "-out out/SamplesNGSD_out1.tsv -project X-Chr -sys hpXLIDv2 -quality bad");

		QStringList output = Helper::loadTextFile("out/SamplesNGSD_out1.tsv", true, QChar::Null, true);
		for (int i=0; i<output.count(); ++i)
		{
			QString line = output[i];
			QStringList parts = line.split("\t");
			QCOMPARE(parts.count(), 12);

			//header
			if (i==0)
			{
				QCOMPARE(line[0].toLatin1(), '#');
			}
			else
			{
				QCOMPARE(parts[5], QString("hpXLIDv2"));
				QCOMPARE(parts[6], QString("X-Chr"));
			}
		}
		QVERIFY(output.count()>=44);
	}
	
};

