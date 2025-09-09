#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportQC_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();

		I_EQUAL(db.getValue("SELECT count(*) FROM qc_terms").toInt(), 0)

		//test
		EXECUTE("NGSDImportQC", "-test -obo " + TESTDATA("data_in/NGSDImportQC_qcml.obo") + " -debug");


		I_EQUAL(db.getValue("SELECT count(*) FROM qc_terms").toInt(), 43)
		I_EQUAL(db.getValue("SELECT count(*) FROM qc_terms WHERE obsolete=0").toInt(), 39)
	}
};

