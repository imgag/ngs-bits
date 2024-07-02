#include "TestFramework.h"


TEST_CLASS(ExtractMethylationData_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		EXECUTE("ExtractMethylationData", "-in " + TESTDATA("data_in/ExtractMethylationData_in1.bed.gz") + " -loci " + TESTDATA("data_in/ExtractMethylationData_loci1.bed")
				+ " -out out/ExtractMethylationData_out1.bed");
		COMPARE_FILES("out/ExtractMethylationData_out1.bed", TESTDATA("data_out/ExtractMethylationData_out1.bed"));
	}
	
};

