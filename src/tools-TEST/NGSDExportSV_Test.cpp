#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportSV_Test)
{
private:

	TEST_METHOD(test_default)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSV_init1.sql"));

		//create output folder
		if (QDir("./out/NGSDExportSV").exists()) QDir("./out/NGSDExportSV").removeRecursively();
		QDir(".").mkdir("out/NGSDExportSV");

		//test
		EXECUTE("NGSDExportSV", "-test -out_folder out/NGSDExportSV");
        REMOVE_LINES("out/NGSDExportSV/sv_deletion.bedpe", QRegularExpression("##fileDate="));
        REMOVE_LINES("out/NGSDExportSV/sv_duplication.bedpe", QRegularExpression("##fileDate="));
        REMOVE_LINES("out/NGSDExportSV/sv_insertion.bedpe", QRegularExpression("##fileDate="));
        REMOVE_LINES("out/NGSDExportSV/sv_inversion.bedpe", QRegularExpression("##fileDate="));
        REMOVE_LINES("out/NGSDExportSV/sv_translocation.bedpe", QRegularExpression("##fileDate="));
		COMPARE_FILES("out/NGSDExportSV/sv_deletion.bedpe", TESTDATA("data_out/NGSDExportSV/sv_deletion.bedpe"));
		COMPARE_FILES("out/NGSDExportSV/sv_duplication.bedpe", TESTDATA("data_out/NGSDExportSV/sv_duplication.bedpe"));
		COMPARE_FILES("out/NGSDExportSV/sv_insertion.bedpe", TESTDATA("data_out/NGSDExportSV/sv_insertion.bedpe"));
		COMPARE_FILES("out/NGSDExportSV/sv_inversion.bedpe", TESTDATA("data_out/NGSDExportSV/sv_inversion.bedpe"));
		COMPARE_FILES("out/NGSDExportSV/sv_translocation.bedpe", TESTDATA("data_out/NGSDExportSV/sv_translocation.bedpe"));
		COMPARE_FILES("out/NGSDExportSV/sv_breakpoint_density.igv", TESTDATA("data_out/NGSDExportSV/sv_breakpoint_density.igv"));
		IS_FALSE(QFile::exists("out/NGSDExportSV/sv_breakpoint_density_hpHBOCv5.igv"));

	}

	TEST_METHOD(test_with_system_specific_files)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSV_init1.sql"));

		//delete previous exports and create output folder
		if (QDir("./out/NGSDExportSV2").exists()) QDir("./out/NGSDExportSV2").removeRecursively();
		QDir(".").mkdir("out/NGSDExportSV2");

		//test
		EXECUTE("NGSDExportSV", "-test -out_folder out/NGSDExportSV2 -common_sys_threshold 3");
        REMOVE_LINES("out/NGSDExportSV2/sv_deletion.bedpe", QRegularExpression("##fileDate="));
        REMOVE_LINES("out/NGSDExportSV2/sv_duplication.bedpe", QRegularExpression("##fileDate="));
        REMOVE_LINES("out/NGSDExportSV2/sv_insertion.bedpe", QRegularExpression("##fileDate="));
        REMOVE_LINES("out/NGSDExportSV2/sv_inversion.bedpe", QRegularExpression("##fileDate="));
        REMOVE_LINES("out/NGSDExportSV2/sv_translocation.bedpe", QRegularExpression("##fileDate="));
		COMPARE_FILES("out/NGSDExportSV2/sv_deletion.bedpe", TESTDATA("data_out/NGSDExportSV/sv_deletion.bedpe"));
		COMPARE_FILES("out/NGSDExportSV2/sv_duplication.bedpe", TESTDATA("data_out/NGSDExportSV/sv_duplication.bedpe"));
		COMPARE_FILES("out/NGSDExportSV2/sv_insertion.bedpe", TESTDATA("data_out/NGSDExportSV/sv_insertion.bedpe"));
		COMPARE_FILES("out/NGSDExportSV2/sv_inversion.bedpe", TESTDATA("data_out/NGSDExportSV/sv_inversion.bedpe"));
		COMPARE_FILES("out/NGSDExportSV2/sv_translocation.bedpe", TESTDATA("data_out/NGSDExportSV/sv_translocation.bedpe"));
		COMPARE_FILES("out/NGSDExportSV2/sv_breakpoint_density.igv", TESTDATA("data_out/NGSDExportSV/sv_breakpoint_density.igv"));
		COMPARE_FILES("out/NGSDExportSV2/sv_breakpoint_density_hpHBOCv5.igv", TESTDATA("data_out/NGSDExportSV/sv_breakpoint_density_hpHBOCv5.igv"));
	}


};

