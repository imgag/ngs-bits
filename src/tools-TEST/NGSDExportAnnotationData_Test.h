#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportAnnotationData_Test)
{
Q_OBJECT
private slots:

	void test_germline_one_thread_with_genes()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");
		if (Settings::string("reference_genome", true)=="") SKIP("Test needs access to the reference genome!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportAnnotationData_init1.sql"));

		//test
		EXECUTE("NGSDExportAnnotationData", "-test -germline out/NGSDExportAnnotationData_out.vcf -threads 1 -genes out/NGSDExportAnnotationData_out.bed");
		EXECUTE("VcfCheck", "-in out/NGSDExportAnnotationData_out.vcf -out out/NGSDExportAnnotationData_VcfCheck_out.txt -info");
		REMOVE_LINES("out/NGSDExportAnnotationData_out.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/NGSDExportAnnotationData_out.vcf", QRegExp("##source=NGSDExportAnnotationData"));
		REMOVE_LINES("out/NGSDExportAnnotationData_out.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/NGSDExportAnnotationData_out.vcf", TESTDATA("data_out/NGSDExportAnnotationData_out.vcf"));
		COMPARE_FILES("out/NGSDExportAnnotationData_out.bed", TESTDATA("data_out/NGSDExportAnnotationData_out.bed"));
	}

	void test_germline_several_threads()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");
		if (Settings::string("reference_genome", true)=="") SKIP("Test needs access to the reference genome!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportAnnotationData_init1.sql"));

		//test
		EXECUTE("NGSDExportAnnotationData", "-test -germline out/NGSDExportAnnotationData_out2.vcf -threads 4");
		EXECUTE("VcfCheck", "-in out/NGSDExportAnnotationData_out2.vcf -out out/NGSDExportAnnotationData_VcfCheck_out2.txt -info");
		REMOVE_LINES("out/NGSDExportAnnotationData_out2.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/NGSDExportAnnotationData_out2.vcf", QRegExp("##source=NGSDExportAnnotationData"));
		REMOVE_LINES("out/NGSDExportAnnotationData_out2.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/NGSDExportAnnotationData_out2.vcf", TESTDATA("data_out/NGSDExportAnnotationData_out.vcf"));
	}

	void test_somatic_one_thread()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");
		if (Settings::string("reference_genome", true)=="") SKIP("Test needs access to the reference genome!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportAnnotationData_init2.sql"));

		//test
		EXECUTE("NGSDExportAnnotationData", "-test -somatic out/NGSDExportAnnotationData_out3.vcf -threads 1");
		EXECUTE("VcfCheck", "-in out/NGSDExportAnnotationData_out3.vcf -out out/NGSDExportAnnotationData_VcfCheck_out3.txt -info");
		REMOVE_LINES("out/NGSDExportAnnotationData_out3.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/NGSDExportAnnotationData_out3.vcf", QRegExp("##source=NGSDExportAnnotationData"));
		REMOVE_LINES("out/NGSDExportAnnotationData_out3.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/NGSDExportAnnotationData_out3.vcf", TESTDATA("data_out/NGSDExportAnnotationData_out3.vcf"));
	}

	void test_somatic_several_threads_with_vicc_with_germline()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");
		if (Settings::string("reference_genome", true)=="") SKIP("Test needs access to the reference genome!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportAnnotationData_init2.sql"));

		//test export of VICC configuration details
		EXECUTE("NGSDExportAnnotationData", "-test -somatic out/NGSDExportAnnotationData_out4.vcf -vicc_config_details -threads 4 -germline out/NGSDExportAnnotationData_out5.vcf");
		REMOVE_LINES("out/NGSDExportAnnotationData_out4.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/NGSDExportAnnotationData_out4.vcf", QRegExp("##source=NGSDExportAnnotationData"));
		REMOVE_LINES("out/NGSDExportAnnotationData_out4.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/NGSDExportAnnotationData_out4.vcf", TESTDATA("data_out/NGSDExportAnnotationData_out4.vcf"));
		REMOVE_LINES("out/NGSDExportAnnotationData_out5.vcf", QRegExp("##fileDate="));
		REMOVE_LINES("out/NGSDExportAnnotationData_out5.vcf", QRegExp("##source=NGSDExportAnnotationData"));
		REMOVE_LINES("out/NGSDExportAnnotationData_out5.vcf", QRegExp("##reference="));
		COMPARE_FILES("out/NGSDExportAnnotationData_out5.vcf", TESTDATA("data_out/NGSDExportAnnotationData_out5.vcf")); //nearly empty file, just to test that export of germline and somatic at a time works as well
	}
};


