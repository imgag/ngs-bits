#include "TestFramework.h"

TEST_CLASS(SampleIdentity_Test)
{
private:
	
	TEST_METHOD(wes_rna_test)
	{
		EXECUTE("SampleIdentity", "-bams " + TESTDATA("data_in/SampleIdentity_in_wes.cram") + " " + TESTDATA("data_in/SampleIdentity_in_rna.cram") + " -out out/SampleIdentity_out1.tsv -basename");
		COMPARE_FILES("out/SampleIdentity_out1.tsv", TESTDATA("data_out/SampleIdentity_out1.tsv"));
	}
};
