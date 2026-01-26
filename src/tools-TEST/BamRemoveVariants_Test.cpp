#include "TestFrameworkNGS.h"


TEST_CLASS(BamRemoveVariants_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		EXECUTE("BamRemoveVariants", "-in " + TESTDATA("data_in/BamRemoveVariants_in1.bam") + " -vcf " + TESTDATA("data_in/BamRemoveVariants_in1.vcf.gz") + " -out out/BamRemoveVariants_out1.bam");

		BAM_TO_TEXT("out/BamRemoveVariants_out1.bam", "out/BamRemoveVariants_out1.bam.txt");
		BAM_TO_TEXT(TESTDATA("data_out/BamRemoveVariants_out1.bam"), "out/BamRemoveVariants_out1.expected.txt");
		COMPARE_FILES("out/BamRemoveVariants_out1.bam.txt", "out/BamRemoveVariants_out1.expected.txt");
	}
};

