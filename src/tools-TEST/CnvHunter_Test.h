#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(CnvHunter_Test)
{
Q_OBJECT
private slots:

	void hpPDv3_anno_seg_debug()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/CnvHunter_init.sql"));

		QStringList in = Helper::findFiles(TESTDATA("data_in/CnvHunter1/"), "*.cov", false);

		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out1.tsv -debug GS120224_01 -seg GS120551_01 -anno -test -cnp_file " +  TESTDATA("data_in/CnvHunter_cnp_file.bed"));
        COMPARE_FILES("out/CnvHunter_out1.tsv", TESTDATA("data_out/CnvHunter_out1.tsv"));
        COMPARE_FILES("out/CnvHunter_out1_regions.tsv", TESTDATA("data_out/CnvHunter_out1_regions.tsv"));
        COMPARE_FILES("out/CnvHunter_out1_samples.tsv", TESTDATA("data_out/CnvHunter_out1_samples.tsv"));
		COMPARE_FILES("out/CnvHunter_out1_debug.tsv", TESTDATA("data_out/CnvHunter_out1_debug.tsv"));
		COMPARE_FILES("out/CnvHunter_out1.seg", TESTDATA("data_out/CnvHunter_out1.seg"));
	}

	void ssHAEv6()
	{
		QStringList in = Helper::findFiles(TESTDATA("data_in/CnvHunter2/"), "*.cov", false);
		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out2.tsv -sam_min_corr 0.8 -reg_max_cv 0.5 -sam_corr_regs 250000 -cnp_file " +  TESTDATA("data_in/CnvHunter_cnp_file.bed"));
		COMPARE_FILES("out/CnvHunter_out2.tsv", TESTDATA("data_out/CnvHunter_out2.tsv"));
	}
};
