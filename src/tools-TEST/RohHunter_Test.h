#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(RohHunter_Test)
{
Q_OBJECT
private slots:

	void default_values()
	{
		EXECUTE("RohHunter", "-in " + TESTDATA("data_in/RohHunter_in1.vcf.gz") + " -out out/RohHunter_out1.tsv");
        COMPARE_FILES("out/RohHunter_out1.tsv", TESTDATA("data_out/RohHunter_out1.tsv"));
	}
};
