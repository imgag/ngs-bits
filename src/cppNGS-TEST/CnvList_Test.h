#include "TestFramework.h"
#include "CnvList.h"


TEST_CLASS(CnvList_Test)
{
Q_OBJECT
private slots:

	void qcMetric()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		S_EQUAL(cnvs.qcMetric("number of iterations"), "1");
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
