#include "TestFramework.h"
#include "CnvList.h"


TEST_CLASS(CnvList_Test)
{
Q_OBJECT
private slots:


	void load()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		I_EQUAL(cnvs.comments().count(), 5);
		S_EQUAL(cnvs.build(), "GRCh38");
		I_EQUAL(cnvs.count(), 67);
		S_EQUAL(cnvs.qcMetric("number of iterations"), "1");
	}

	void loadHeaderOnly()
	{
		CnvList cnvs;
		cnvs.loadHeaderOnly(TESTDATA("data_in/CnvList_ClinCNV_germline_multi.tsv"));

		I_EQUAL(cnvs.comments().count(), 16);
		S_EQUAL(cnvs.build(), "");
		I_EQUAL(cnvs.count(), 0);
	}

	void import_export_ClinCNV_germline()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));
		cnvs.store("out/CnvList_ClinCNV_germline.tsv");
		COMPARE_FILES("out/CnvList_ClinCNV_germline.tsv", TESTDATA("data_out/CnvList_ClinCNV_germline.tsv"));
	}

	void import_export_ClinCNV_germline_multi()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline_multi.tsv"));
		cnvs.store("out/CnvList_ClinCNV_germline_multi.tsv");
		COMPARE_FILES("out/CnvList_ClinCNV_germline_multi.tsv", TESTDATA("data_out/CnvList_ClinCNV_germline_multi.tsv"));
	}

	void import_export_ClinCNV_somatic()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_somatic.tsv"));
		cnvs.store("out/CnvList_ClinCNV_somatic.tsv");
		COMPARE_FILES("out/CnvList_ClinCNV_somatic.tsv", TESTDATA("data_out/CnvList_ClinCNV_somatic.tsv"));
	}

	void import_export_CnvHunter_germline()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_CnvHunter_germline.tsv"));
		cnvs.store("out/CnvList_CnvHunter_germline.tsv");
		COMPARE_FILES("out/CnvList_CnvHunter_germline.tsv", TESTDATA("data_out/CnvList_CnvHunter_germline.tsv"));
	}

};
