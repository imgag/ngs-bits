#include "TestFramework.h"
#include "Settings.h"
#include "GenLabDB.h"

TEST_CLASS(NGSDGenlabInfo_Test)
{
private:

	TEST_METHOD(getInfo)
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

		//test sample 1
		EXECUTE("Genlabinfo", "-ps DXtest1_01 -out out/GenlabInfo_out1.tsv");
		COMPARE_FILES("out/GenlabInfo_out1.tsv", TESTDATA("data_out/GenlabInfo_out1.tsv"));
		EXECUTE("Genlabinfo", "-ps DXtest1_02 -info SAPID,PATID -out out/GenlabInfo_out2.tsv");
		COMPARE_FILES("out/GenlabInfo_out2.tsv", TESTDATA("data_out/GenlabInfo_out2.tsv"));
		EXECUTE("Genlabinfo", "-ps DXtest1_01 -info PATID,SAPID -out out/GenlabInfo_out3.tsv");
		COMPARE_FILES("out/GenlabInfo_out3.tsv", TESTDATA("data_out/GenlabInfo_out3.tsv"));

		EXECUTE("Genlabinfo", "-ps " + TESTDATA("data_in/GenlabInfo_in1.tsv") + " -info SAPID,PATID -out out/GenlabInfo_out4.tsv");
		COMPARE_FILES("out/GenlabInfo_out4.tsv", TESTDATA("data_out/GenlabInfo_out4.tsv"));

	}

};


