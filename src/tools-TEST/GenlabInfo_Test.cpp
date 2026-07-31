#include "TestFrameworkNGS.h"
#include "GenLabDB.h"

TEST_CLASS(GenlabInfo_Test)
{
private:

	TEST_METHOD(getInfo)
	{
		SKIP_IF_NO_PROD_GENLAB();

		//sample to SAP ID
		EXECUTE("GenlabInfo", "-ps DX172305_01 -out out/GenlabInfo_out1.tsv");
		COMPARE_FILES("out/GenlabInfo_out1.tsv", TESTDATA("data_out/GenlabInfo_out1.tsv"));
		EXECUTE("GenlabInfo", "-ps DXtest1_02 -info SAPID,PATID -out out/GenlabInfo_out2.tsv");
		COMPARE_FILES("out/GenlabInfo_out2.tsv", TESTDATA("data_out/GenlabInfo_out2.tsv"));
		EXECUTE("GenlabInfo", "-ps DXtest1_01 -info PATID,SAPID -out out/GenlabInfo_out3.tsv");
		COMPARE_FILES("out/GenlabInfo_out3.tsv", TESTDATA("data_out/GenlabInfo_out3.tsv"));

		EXECUTE("GenlabInfo", "-ps " + TESTDATA("data_in/GenlabInfo_in1.tsv") + " -info SAPID,PATID -out out/GenlabInfo_out4.tsv");
		COMPARE_FILES("out/GenlabInfo_out4.tsv", TESTDATA("data_out/GenlabInfo_out4.tsv"));
	}
	
	TEST_METHOD(getInfoSapId)
	{
		SKIP_IF_NO_PROD_GENLAB();

		//SAP ID to sample
		EXECUTE("GenlabInfo", "-sap_id 4942684 -out out/GenlabInfo_out5.tsv");
		COMPARE_FILES("out/GenlabInfo_out5.tsv", TESTDATA("data_out/GenlabInfo_out5.tsv"));

		EXECUTE("GenlabInfo", "-sap_id " + TESTDATA("data_in/GenlabInfo_in2.tsv") + " -info SAPID,PATID -out out/GenlabInfo_out6.tsv");
		COMPARE_FILES("out/GenlabInfo_out6.tsv", TESTDATA("data_out/GenlabInfo_out6.tsv"));		
	}
};


