#include "TestFramework.h"

TEST_CLASS(CnvReferenceCohort_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		EXECUTE("CnvReferenceCohort", "-in " + TESTDATA("data_in/CnvReferenceCohort_in.cov") + " -in_ref " + TESTDATA("data_in/CnvReferenceCohort_in_ref1.cov") + " " + TESTDATA("data_in/CnvReferenceCohort_in_ref2.cov") + " " + TESTDATA("data_in/CnvReferenceCohort_in_ref3.cov.gz") + " " + TESTDATA("data_in/CnvReferenceCohort_in_ref4.cov.gz") + " " + TESTDATA("data_in/CnvReferenceCohort_in_ref5.cov.gz") + " -exclude " + TESTDATA("data_in/CnvReferenceCohort_exclude1.bed") + " " + TESTDATA("data_in/CnvReferenceCohort_exclude2.bed") + " " + TESTDATA("data_in/CnvReferenceCohort_exclude3.bed") + " -out out/CnvReferenceCohort_test01_out.tsv -cov_max 3");
		COMPARE_FILES("out/CnvReferenceCohort_test01_out.tsv", TESTDATA("data_out/CnvReferenceCohort_test01_out.tsv"));
		COMPARE_FILES("out/CnvReferenceCohort_Test_line10.log", TESTDATA("data_out/CnvReferenceCohort_out.log"));
	}
	
};
