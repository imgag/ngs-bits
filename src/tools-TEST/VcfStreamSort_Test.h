#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(VcfStreamSort_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		EXECUTE("VcfStreamSort", "-n 4 -in " + TESTDATA("data_in/VcfStreamSort_in1.vcf") + " -out out/VcfStreamSort_out1.vcf");
		COMPARE_FILES("out/VcfStreamSort_out1.vcf", TESTDATA("data_out/VcfStreamSort_out1.vcf"));
	}

};

