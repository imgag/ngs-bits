#include "TestFrameworkNGS.h"

TEST_CLASS(SampleIdentity_Test)
{
private:
	
	TEST_METHOD(wes_rna_test_1_thread)
	{
	  SKIP_IF_NO_HG38_GENOME();

		EXECUTE("SampleIdentity", "-bams " + TESTDATA("data_in/SampleIdentity_in_wes.cram") + " " + TESTDATA("data_in/SampleIdentity_in_rna.cram") + " -out out/SampleIdentity_out1.tsv -basename -min_snps 30 -threads 1");
		COMPARE_FILES("out/SampleIdentity_out1.tsv", TESTDATA("data_out/SampleIdentity_out1.tsv"));
	}

	TEST_METHOD(wes_rna_test_2_threads)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("SampleIdentity", "-bams " + TESTDATA("data_in/SampleIdentity_in_wes.cram") + " " + TESTDATA("data_in/SampleIdentity_in_rna.cram") + " -out out/SampleIdentity_out1.tsv -basename -min_snps 30 -threads 2");
		COMPARE_FILES("out/SampleIdentity_out1.tsv", TESTDATA("data_out/SampleIdentity_out1.tsv"));
	}

	TEST_METHOD(wes_rna_test_3_threads)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("SampleIdentity", "-bams " + TESTDATA("data_in/SampleIdentity_in_wes.cram") + " " + TESTDATA("data_in/SampleIdentity_in_rna.cram") + " -out out/SampleIdentity_out1.tsv -basename -min_snps 30 -threads 3");
		COMPARE_FILES("out/SampleIdentity_out1.tsv", TESTDATA("data_out/SampleIdentity_out1.tsv"));
	}
};
