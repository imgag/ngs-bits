#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportAnnotationData_Test)
{
Q_OBJECT
private slots:

	void test_germline_01()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportAnnotationData_init1.sql"));

		//test
		EXECUTE("NGSDExportAnnotationData", "-test -variants out/NGSDExportAnnotationData_out.vcf -genes out/NGSDExportAnnotationData_out.bed");
		EXECUTE("VcfCheck", "-in out/NGSDExportAnnotationData_out.vcf -out out/NGSDExportAnnotationData_VcfCheck_out.txt -info");
		REMOVE_LINES("out/NGSDExportAnnotationData_out.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/NGSDExportAnnotationData_out.vcf", QRegExp("##source=NGSDExportAnnotationData"));
		REMOVE_LINES("out/NGSDExportAnnotationData_out.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/NGSDExportAnnotationData_out.vcf", TESTDATA("data_out/NGSDExportAnnotationData_out.vcf"));
		COMPARE_FILES("out/NGSDExportAnnotationData_out.bed", TESTDATA("data_out/NGSDExportAnnotationData_out.bed"));
	}

	void test_germline_02()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportAnnotationData_init1.sql"));

		//test
		EXECUTE("NGSDExportAnnotationData", "-test -variants out/NGSDExportAnnotationData_out2.vcf -chr chr6");
		EXECUTE("VcfCheck", "-in out/NGSDExportAnnotationData_out2.vcf -out out/NGSDExportAnnotationData_VcfCheck_out2.txt -info");
		REMOVE_LINES("out/NGSDExportAnnotationData_out2.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/NGSDExportAnnotationData_out2.vcf", QRegExp("##source=NGSDExportAnnotationData"));
		REMOVE_LINES("out/NGSDExportAnnotationData_out2.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/NGSDExportAnnotationData_out2.vcf", TESTDATA("data_out/NGSDExportAnnotationData_out2.vcf"));
	}

	void test_somatic_01()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportAnnotationData_init2.sql"));

		//test
		EXECUTE("NGSDExportAnnotationData", "-test -variants out/NGSDExportAnnotationData_out3.vcf -mode somatic");
		EXECUTE("VcfCheck", "-in out/NGSDExportAnnotationData_out3.vcf -out out/NGSDExportAnnotationData_VcfCheck_out3.txt -info");
		REMOVE_LINES("out/NGSDExportAnnotationData_out3.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/NGSDExportAnnotationData_out3.vcf", QRegExp("##source=NGSDExportAnnotationData"));
		REMOVE_LINES("out/NGSDExportAnnotationData_out3.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/NGSDExportAnnotationData_out3.vcf", TESTDATA("data_out/NGSDExportAnnotationData_out3.vcf"));
	}

};


