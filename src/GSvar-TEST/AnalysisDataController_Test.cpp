#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include <QTest>
#include "AnalysisDataController.h"
#include "Application.h"
#include "MainWindow.h"

TEST_CLASS(AnalysisDataController_Test)
{

TEST_METHOD(data_loading_with_NGSD)
{
	SKIP_IF_NO_TEST_NGSD();

	//init
	NGSD db(true);
	db.init();
	db.executeQueriesFromFile(TESTDATA("data_in/NGSD_base_in.sql"));

	SqlQuery query = db.getQuery();
	query.prepare("UPDATE processed_sample SET folder_override = :0 WHERE id = :1");
	query.bindValue(0, TESTDATA("data_in/sample_data/Sample_NA12878-WES-DEFAULT_01/"));
	query.bindValue(1, 101);
	query.exec();
	query.bindValue(0, TESTDATA("data_in/sample_data/Sample_NA12878-WES-DRAGEN_01/"));
	query.bindValue(1, 102);
	query.exec();

	QString gsvar_default = TESTDATA("data_in/sample_data/Sample_NA12878-WES-DEFAULT_01/NA12878-WES-DEFAULT_01.GSvar");
	QString gsvar_dragen = TESTDATA("data_in/sample_data/Sample_NA12878-WES-DRAGEN_01/NA12878-WES-DRAGEN_01.GSvar");


	//start application
	int argc = 0;
	char* argv[0];
	Application a(argc, argv);
	MainWindow w;

	AnalysisDataController& controller = AnalysisDataController::instance();
	IS_FALSE(controller.isValid());

	//load default analysis
	controller.loadFile(gsvar_default);
	IS_TRUE(controller.isValid());

	//reset controller:
	controller.loadFile();
	IS_FALSE(controller.isValid());
	I_EQUAL(controller.getSmallVariantList().count(), 0);
	I_EQUAL(controller.getCnvList().count(), 0);
	I_EQUAL(controller.getSvList().count(), 0);
	I_EQUAL(controller.getReList().count(), 0);

	//load dragen analysis
	controller.loadFile(gsvar_dragen);
	IS_TRUE(controller.isValid());



	w.closeAndLogout();
	S_EQUAL("TODO", "DONE");
}

TEST_METHOD(report_configs)
{
    S_EQUAL("TODO", "DONE");
}

};
