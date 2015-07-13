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
		if (host=="") SKIP("Test needs access to the NGSD!");

		EXECUTE("SamplesNGSD", "-out out/SamplesNGSD_out1.tsv -project X-Chr -sys hpXLIDv2 -quality bad");

		QStringList output = Helper::loadTextFile("out/SamplesNGSD_out1.tsv", true, QChar::Null, true);
		for (int i=0; i<output.count(); ++i)
		{
			QString line = output[i];
			QStringList parts = line.split("\t");
			I_EQUAL(parts.count(), 12);

			//header
			if (i==0)
			{
				S_EQUAL(line[0].toLatin1(), '#');
			}
			else
			{
				S_EQUAL(parts[5], QString("hpXLIDv2"));
				S_EQUAL(parts[6], QString("X-Chr"));
			}
		}
		IS_TRUE(output.count()>=44);
	}
	
};

