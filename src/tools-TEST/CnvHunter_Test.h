#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(CnvHunter_Test)
{
Q_OBJECT
private slots:

	void hpPDv3_noanno()
	{
		QStringList in = Helper::findFiles(TESTDATA("data_in/CnvHunter1/"), "*.cov", false);
		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out1.tsv");
		COMPARE_FILES("out/CnvHunter_out1.tsv", TESTDATA("data_out/CnvHunter_out1.tsv"));
	}

	void hpPDv3_anno()
	{
		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/CnvHunter_init.sql"));

		QStringList in = Helper::findFiles(TESTDATA("data_in/CnvHunter1/"), "*.cov", false);
		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out6.tsv -anno -test");
		COMPARE_FILES("out/CnvHunter_out6.tsv", TESTDATA("data_out/CnvHunter_out6.tsv"));
	}

	void hpSCv1()
	{
		QStringList in = Helper::findFiles(TESTDATA("data_in/CnvHunter2/"), "*.cov", false);
		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out2.tsv");
		COMPARE_FILES("out/CnvHunter_out2.tsv", TESTDATA("data_out/CnvHunter_out2.tsv"));
	}
	
	void ssX()
	{
		QStringList in = Helper::findFiles(TESTDATA("data_in/CnvHunter3/"), "*.cov", false);
		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out3.tsv");
		COMPARE_FILES("out/CnvHunter_out3.tsv", TESTDATA("data_out/CnvHunter_out3.tsv"));
	}

	void hpSCAv4_excludeReg_regionBedFile()
	{
		QStringList in = Helper::findFiles(TESTDATA("data_in/CnvHunter4/"), "*.cov", false);
		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out4.tsv -out_reg out/CnvHunter_out4.bed -exclude " +  TESTDATA("data_in/CnvHunter4/excluded.bed"));
		COMPARE_FILES("out/CnvHunter_out4.tsv", TESTDATA("data_out/CnvHunter_out4.tsv"));
	}

	void ssKM()
	{
		QStringList in = Helper::findFiles(TESTDATA("data_in/CnvHunter5/"), "*.cov", false);
		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out5.tsv");
		COMPARE_FILES("out/CnvHunter_out5.tsv", TESTDATA("data_out/CnvHunter_out5.tsv"));
	}
};
