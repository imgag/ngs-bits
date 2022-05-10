#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDExportSV_Test)
{
Q_OBJECT
private slots:

	void test_default()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDExportSV_init1.sql"));

		//create output folder
		QDir(".").mkdir("out/NGSDExportSV");

		//test
		EXECUTE("NGSDExportSV", "-test -out_folder out/NGSDExportSV");
		REMOVE_LINES("out/NGSDExportSV/sv_deletion.bedpe", QRegExp("##fileDate="));
		REMOVE_LINES("out/NGSDExportSV/sv_duplication.bedpe", QRegExp("##fileDate="));
		REMOVE_LINES("out/NGSDExportSV/sv_insertion.bedpe", QRegExp("##fileDate="));
		REMOVE_LINES("out/NGSDExportSV/sv_inversion.bedpe", QRegExp("##fileDate="));
		REMOVE_LINES("out/NGSDExportSV/sv_translocation.bedpe", QRegExp("##fileDate="));
		COMPARE_FILES("out/NGSDExportSV/sv_deletion.bedpe", TESTDATA("data_out/NGSDExportSV/sv_deletion.bedpe"));
		COMPARE_FILES("out/NGSDExportSV/sv_duplication.bedpe", TESTDATA("data_out/NGSDExportSV/sv_duplication.bedpe"));
		COMPARE_FILES("out/NGSDExportSV/sv_insertion.bedpe", TESTDATA("data_out/NGSDExportSV/sv_insertion.bedpe"));
		COMPARE_FILES("out/NGSDExportSV/sv_inversion.bedpe", TESTDATA("data_out/NGSDExportSV/sv_inversion.bedpe"));
		COMPARE_FILES("out/NGSDExportSV/sv_translocation.bedpe", TESTDATA("data_out/NGSDExportSV/sv_translocation.bedpe"));
		COMPARE_FILES("out/NGSDExportSV/sv_breakpoint_density.igv", TESTDATA("data_out/NGSDExportSV/sv_breakpoint_density.igv"));
	}


};

