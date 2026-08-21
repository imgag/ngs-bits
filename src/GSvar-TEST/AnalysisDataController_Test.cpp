#include "TestFramework.h"
#include <QTest>
#include "AnalysisDataController.h"

TEST_CLASS(AnalysisDataController_Test)
{

TEST_METHOD(data_loading)
{
	AnalysisDataController& controller = AnalysisDataController::instance();
	IS_FALSE(controller.isValid());
	S_EQUAL("TODO", "DONE");
}

TEST_METHOD(report_configs)
{
    S_EQUAL("TODO", "DONE");
}

};
