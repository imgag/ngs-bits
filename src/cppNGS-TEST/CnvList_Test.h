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

};
