#include "TestFramework.h"

TEST_CLASS(TsvTo_Test)
{
private:
	TEST_METHOD(format_html)
	{
		EXECUTE("TsvTo", "-in " + TESTDATA("data_in/TsvTo_in1.tsv") + " -format html -out out/TsvTo_out1.html");
		COMPARE_FILES("out/TsvTo_out1.html", TESTDATA("data_out/TsvTo_out1.html"));
	}

	TEST_METHOD(format_txt)
	{
		EXECUTE("TsvTo", "-in " + TESTDATA("data_in/TsvTo_in1.tsv") + " -format txt -out out/TsvTo_out2.txt");
		COMPARE_FILES("out/TsvTo_out2.txt", TESTDATA("data_out/TsvTo_out2.txt"));
	}

	TEST_METHOD(format_md)
	{
		EXECUTE("TsvTo", "-in " + TESTDATA("data_in/TsvTo_in1.tsv") + " -format md -out out/TsvTo_out3.md");
		COMPARE_FILES("out/TsvTo_out3.md", TESTDATA("data_out/TsvTo_out3.md"));
	}
};

