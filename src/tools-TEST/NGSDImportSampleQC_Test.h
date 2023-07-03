#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportSampleQC_Test)
{
Q_OBJECT
private slots:
	
	void germline_files()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportSampleQC_init.sql"));

		//test
		EXECUTE("NGSDImportSampleQC", "-test -ps NA12878_45 -files " + TESTDATA("data_in/NGSDImportSampleQC_in1.qcML") + " " + TESTDATA("data_in/NGSDImportSampleQC_in2.qcML") + " " + TESTDATA("data_in/NGSDImportSampleQC_in3.qcML") + " " + TESTDATA("data_in/NGSDImportSampleQC_in4.qcML") + " -force");

		I_EQUAL(db.getValue("SELECT count(*) FROM processed_sample_qc").toInt(), 46)
	}

	void somatic_files()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportSampleQC_init.sql"));

		//test
		EXECUTE("NGSDImportSampleQC", "-test -ps NA12878_45 -files " + TESTDATA("data_in/NGSDImportSampleQC_in5.qcML") + " " + TESTDATA("data_in/NGSDImportSampleQC_in6.qcML") + " " + TESTDATA("data_in/NGSDImportSampleQC_in7.qcML") + " " + TESTDATA("data_in/NGSDImportSampleQC_in8.qcML") + " -force");

		I_EQUAL(db.getValue("SELECT count(*) FROM processed_sample_qc").toInt(), 50)

	}
};

