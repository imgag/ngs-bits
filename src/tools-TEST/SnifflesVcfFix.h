#include "TestFramework.h"
#include "TestFrameworkNGS.h"

TEST_CLASS(SnifflesVcfFix_Test)
{
Q_OBJECT
private slots:
	
	void test1()
	{
		EXECUTE("SnifflesVcfFix", "-in " + TESTDATA("data_in/SnifflesVcfFix_in1.vcf") + " -out out/SnifflesVcfFix_out1.vcf");
		COMPARE_FILES("out/SnifflesVcfFix_out1.vcf", TESTDATA("data_out/SnifflesVcfFix_out1.vcf"));
	}

	void test_with_read_names()
	{
		EXECUTE("SnifflesVcfFix", "-in " + TESTDATA("data_in/SnifflesVcfFix_in2.vcf") + " -out out/SnifflesVcfFix_out2.vcf");
		COMPARE_FILES("out/SnifflesVcfFix_out2.vcf", TESTDATA("data_out/SnifflesVcfFix_out2.vcf"));
	}

};


