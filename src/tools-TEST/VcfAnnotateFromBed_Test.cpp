#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VcfAnnotateFromBed_Test)
{
private:
	
	TEST_METHOD(test_01)
	{
        EXECUTE("VcfAnnotateFromBed", "-in " + TESTDATA("data_in/VcfAnnotateFromBed_in1.vcf") + " -out out/VcfAnnotateFromBed_out1.vcf -name OMIM -bed " + TESTDATA("data_in/VcfAnnotateFromBed_in1.bed") );
        REMOVE_LINES("out/VcfAnnotateFromBed_out1.vcf", QRegularExpression("##INFO=<ID=OMIM"))
		COMPARE_FILES("out/VcfAnnotateFromBed_out1.vcf", TESTDATA("data_out/VcfAnnotateFromBed_out1.vcf"));
		VCF_IS_VALID_HG19("out/VcfAnnotateFromBed_out1.vcf")
	}

	TEST_METHOD(multithread)
	{
		EXECUTE("VcfAnnotateFromBed", "-in " + TESTDATA("data_in/VcfAnnotateFromBed_in1.vcf") + " -out out/VcfAnnotateFromBed_out2.vcf -name OMIM -bed " + TESTDATA("data_in/VcfAnnotateFromBed_in1.bed") + " -threads 3 -block_size 20" );
        REMOVE_LINES("out/VcfAnnotateFromBed_out2.vcf", QRegularExpression("##INFO=<ID=OMIM"))
		COMPARE_FILES("out/VcfAnnotateFromBed_out2.vcf", TESTDATA("data_out/VcfAnnotateFromBed_out1.vcf"));
		VCF_IS_VALID_HG19("out/VcfAnnotateFromBed_out2.vcf")
	}


};

