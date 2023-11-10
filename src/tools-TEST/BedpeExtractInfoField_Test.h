#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(BedpeExtractInfoField_Test)
{
Q_OBJECT
private slots:

	void test_basic_annotation()
	{
		//test
		EXECUTE("BedpeExtractInfoField", "-in " + TESTDATA("data_in/BedpeExtractInfoField_in1.bedpe") + " -info_fields END:SV_END,SUPPORT,AF:allele_frequency,PRECISE "
				+ "-out out/BedpeExtractInfoField_out1.bedpe");

		COMPARE_FILES("out/BedpeExtractInfoField_out1.bedpe", TESTDATA("data_out/BedpeExtractInfoField_out1.bedpe"));
	}

	void test_custom_info_column()
	{
		//test
		EXECUTE("BedpeExtractInfoField", "-in " + TESTDATA("data_in/BedpeExtractInfoField_in2.bedpe") + " -info_column CUSTOM_INFO_COLUMN -info_fields END:SV_END,SUPPORT,AF:allele_frequency,PRECISE "
				+ "-out out/BedpeExtractInfoField_out2.bedpe");

		COMPARE_FILES("out/BedpeExtractInfoField_out2.bedpe", TESTDATA("data_out/BedpeExtractInfoField_out2.bedpe"));
	}

};
