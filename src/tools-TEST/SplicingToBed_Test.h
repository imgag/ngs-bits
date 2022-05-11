#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(SplicingToBed_TEST)
{
Q_OBJECT
private slots:

	void test1()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/SplicingToBed_init.sql"));

		EXECUTE("SplicingToBed", "-test -in " + TESTDATA("data_in/splicing.tsv") + " -report out/SplicingToBed_out1_report.tsv -gene_report out/SplicingToBed_out1_genes.tsv -bed out/SplicingToBed_out1.bed");
		COMPARE_FILES("out/SplicingToBed_out1_report.tsv", TESTDATA("data_out/SplicingToBed_out1_report.tsv"));
		COMPARE_FILES("out/SplicingToBed_out1_genes.tsv", TESTDATA("data_out/SplicingToBed_out1_genes.tsv"));
		COMPARE_FILES("out/SplicingToBed_out1.bed", TESTDATA("data_out/SplicingToBed_out1.bed"));
	}

};
