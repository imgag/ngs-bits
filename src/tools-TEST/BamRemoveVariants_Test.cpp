#include "TestFramework.h"


TEST_CLASS(BamRemoveVariants_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		EXECUTE("BamRemoveVariants", "-in " + TESTDATA("data_in/BamRemoveVariants_in1.bam") + " -vcf " + TESTDATA("data_in/BamRemoveVariants_in1.vcf.gz")
				+ " -out out/BamRemoveVariants_out1.bam");
		COMPARE_GZ_FILES("out/BamRemoveVariants_out1.bam", TESTDATA("data_out/BamRemoveVariants_out1.bam"));
	}
	
};

