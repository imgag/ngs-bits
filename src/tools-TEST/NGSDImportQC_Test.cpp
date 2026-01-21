#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportQC_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();

		//run
		EXECUTE("NGSDImportQC", "-test -obo " + TESTDATA("data_in/NGSDImportQC_qcml.obo") + " -debug");

		//check terms were imported
		I_EQUAL(db.getValue("SELECT count(*) FROM qc_terms").toInt(), 43)
		I_EQUAL(db.getValue("SELECT count(*) FROM qc_terms WHERE obsolete=0").toInt(), 39)

		//check imported version
		QString version = db.getValue("SELECT version FROM db_import_info WHERE name='QC terms'").toString();
		S_EQUAL(version, "2025-11-19");
	}
};

