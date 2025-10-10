#include "TestFramework.h"
#include "NGSD.h"

TEST_CLASS(GenesToApproved_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		EXECUTE("NGSDImportHGNC", "-test -in " + TESTDATA("data_in/NGSDImportHGNC_in1.txt") + " -ensembl " + TESTDATA("data_in/NGSDImportEnsembl_in.gff3"));

		//test
		EXECUTE("GenesToApproved", "-test -in " + TESTDATA("data_in/GenesToApproved_in1.txt") + " -out out/GenesToApproved_out1.txt");
		COMPARE_FILES("out/GenesToApproved_out1.txt", TESTDATA("data_out/GenesToApproved_out1.txt"));
	}

	TEST_METHOD(with_report_ambiguous)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		EXECUTE("NGSDImportHGNC", "-test -in " + TESTDATA("data_in/NGSDImportHGNC_in1.txt") + " -ensembl " + TESTDATA("data_in/NGSDImportEnsembl_in.gff3"));

		//test
		EXECUTE("GenesToApproved", "-test -in " + TESTDATA("data_in/GenesToApproved_in1.txt") + " -report_ambiguous -out out/GenesToApproved_out2.txt");
		COMPARE_FILES("out/GenesToApproved_out2.txt", TESTDATA("data_out/GenesToApproved_out2.txt"));
	}
};

