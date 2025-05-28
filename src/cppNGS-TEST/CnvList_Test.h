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

		I_EQUAL(cnvs.comments().count(), 7);
		S_EQUAL(cnvs.build(), "GRCh38");
		I_EQUAL(cnvs.count(), 67);
		S_EQUAL(cnvs.qcMetric("number of iterations"), "1");
	}

	void loadHeaderOnly()
	{
		CnvList cnvs;
		cnvs.loadHeaderOnly(TESTDATA("data_in/CnvList_ClinCNV_germline_multi.tsv"));

		I_EQUAL(cnvs.comments().count(), 17);
		S_EQUAL(cnvs.build(), "GRCh38");
		I_EQUAL(cnvs.count(), 0);
	}

	void basic_test_germline_single()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline.tsv"));

		//test calling parsing
		S_EQUAL(cnvs.build(), "GRCh38");
		S_EQUAL(cnvs.callerAsString(), "ClinCNV");
		S_EQUAL(cnvs.callerVersion(), "v1.18.3");
		S_EQUAL(cnvs.callingDate().toString(Qt::ISODate), "2025-05-28");

		//store and compare files
		cnvs.store("out/CnvList_ClinCNV_germline.tsv");
		COMPARE_FILES("out/CnvList_ClinCNV_germline.tsv", TESTDATA("data_out/CnvList_ClinCNV_germline.tsv"));
	}

	void basic_test_germline_multi()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_germline_multi.tsv"));

		//test calling parsing
		S_EQUAL(cnvs.build(), "GRCh38");
		S_EQUAL(cnvs.callerAsString(), "ClinCNV");
		S_EQUAL(cnvs.callerVersion(), "v1.16.0");
		S_EQUAL(cnvs.callingDate().toString(Qt::ISODate), "2019-07-30");

		//store and compare files
		cnvs.store("out/CnvList_ClinCNV_germline_multi.tsv");
		COMPARE_FILES("out/CnvList_ClinCNV_germline_multi.tsv", TESTDATA("data_out/CnvList_ClinCNV_germline_multi.tsv"));
	}

	void basic_test_somatic()
	{
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/CnvList_ClinCNV_somatic.tsv"));

		//test calling parsing
		S_EQUAL(cnvs.build(), "GRCh38");
		S_EQUAL(cnvs.callerAsString(), "ClinCNV");
		S_EQUAL(cnvs.callerVersion(), "v1.18.3");
		S_EQUAL(cnvs.callingDate().toString(Qt::ISODate), "2025-05-27");

		//store and compare files
		cnvs.store("out/CnvList_ClinCNV_somatic.tsv");
		COMPARE_FILES("out/CnvList_ClinCNV_somatic.tsv", TESTDATA("data_out/CnvList_ClinCNV_somatic.tsv"));
	}

};
