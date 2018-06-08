#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(UpdHunter_Test)
{
Q_OBJECT
private slots:

	void negative_example()
	{
		EXECUTE("UpdHunter", "-in " + TESTDATA("data_in/UpdHunter_in1.vcf.gz") + " -c CHILD -f FATHER -m MOTHER -exclude " + TESTDATA("data_in/UpdHunter_in1.bed") + " -out out/UpdHunter_out1.tsv");
        COMPARE_FILES("out/UpdHunter_out1.tsv", TESTDATA("data_out/UpdHunter_out1.tsv"));
	}

	void positive1()
	{
		EXECUTE("UpdHunter", "-in " + TESTDATA("data_in/UpdHunter_in2.vcf.gz") + " -c CHILD -f FATHER -m MOTHER -out out/UpdHunter_out2.tsv");
		COMPARE_FILES("out/UpdHunter_out2.tsv", TESTDATA("data_out/UpdHunter_out2.tsv"));
	}

	void positive2()
	{
		EXECUTE("UpdHunter", "-in " + TESTDATA("data_in/UpdHunter_in3.vcf.gz") + " -c CHILD -f FATHER -m MOTHER -out out/UpdHunter_out3.tsv");
		COMPARE_FILES("out/UpdHunter_out3.tsv", TESTDATA("data_out/UpdHunter_out3.tsv"));
	}
};
