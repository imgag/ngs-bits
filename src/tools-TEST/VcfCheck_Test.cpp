#include "TestFrameworkNGS.h"
#include "Settings.h"

TEST_CLASS(VcfCheck_Test)
{
private:
	
	TEST_METHOD(no_warnings)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in1.vcf") + " -out out/VcfCheck_out1.txt");
		COMPARE_FILES("out/VcfCheck_out1.txt", TESTDATA("data_out/VcfCheck_out1.txt"));
	}

	TEST_METHOD(no_warnings_with_info_lines)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in1.vcf") + " -out out/VcfCheck_out2.txt -info -lines 200");
		COMPARE_FILES("out/VcfCheck_out2.txt", TESTDATA("data_out/VcfCheck_out2.txt"));
	}

	TEST_METHOD(with_warnings)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in2.vcf") + " -out out/VcfCheck_out3.txt");
        REMOVE_LINES("out/VcfCheck_out3.txt", QRegularExpression("^chr"));
		COMPARE_FILES("out/VcfCheck_out3.txt", TESTDATA("data_out/VcfCheck_out3.txt"));
	}

	TEST_METHOD(no_warnings_vcfgz)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in1.vcf.gz") + " -out out/VcfCheck_out4.txt");
		COMPARE_FILES("out/VcfCheck_out4.txt", TESTDATA("data_out/VcfCheck_out1.txt"));
	}

	TEST_METHOD(empty_info_column)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE_FAIL("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in3.vcf") + " -out out/VcfCheck_out5.txt");
        REMOVE_LINES("out/VcfCheck_out5.txt", QRegularExpression("^chr"));
		COMPARE_FILES("out/VcfCheck_out5.txt", TESTDATA("data_out/VcfCheck_out5.txt"));
	}

	TEST_METHOD(invalid_reference_sequence_of_deletion)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE_FAIL("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in4.vcf") + " -out out/VcfCheck_out6.txt");
		COMPARE_FILES("out/VcfCheck_out6.txt", TESTDATA("data_out/VcfCheck_out6.txt"));
	}

	TEST_METHOD(space_in_info_column_value)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE_FAIL("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in5.vcf") + " -out out/VcfCheck_out7.txt");
		COMPARE_FILES("out/VcfCheck_out7.txt", TESTDATA("data_out/VcfCheck_out7.txt"));
	}

	TEST_METHOD(missing_header_line)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE_FAIL("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in6.vcf") + " -out out/VcfCheck_out8.txt");
		COMPARE_FILES("out/VcfCheck_out8.txt", TESTDATA("data_out/VcfCheck_out8.txt"));
	}

	TEST_METHOD(invalid_reference_sequence)
	{
		SKIP_IF_NO_HG38_GENOME();

		EXECUTE_FAIL("VcfCheck", "-in " + TESTDATA("data_in/VcfCheck_in7.vcf") + " -out out/VcfCheck_out9.txt");
		COMPARE_FILES("out/VcfCheck_out9.txt", TESTDATA("data_out/VcfCheck_out9.txt"));
	}
};

