#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(ExtractMethylationData_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("ExtractMethylationData", "-in " + TESTDATA("data_in/ExtractMethylationData_in1.bed.gz") + " -loci " + TESTDATA("data_in/ExtractMethylationData_loci1.bed")
				+ " -out out/ExtractMethylationData_out1.bed");
		COMPARE_FILES("out/ExtractMethylationData_out1.bed", TESTDATA("data_out/ExtractMethylationData_out1.bed"));
	}
	
	TEST_METHOD(with_type_columns)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("ExtractMethylationData", "-add_methylation_types -in " + TESTDATA("data_in/ExtractMethylationData_in1.bed.gz") + " -loci " + TESTDATA("data_in/ExtractMethylationData_loci1.bed")
				+ " -out out/ExtractMethylationData_out2.bed");
		COMPARE_FILES("out/ExtractMethylationData_out2.bed", TESTDATA("data_out/ExtractMethylationData_out2.bed"));
	}
};

