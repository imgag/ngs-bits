#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "AnalysisDataController.h"

#include <QTest>
#include <QApplication>
#include "Application.h"
#include <QMessageBox>
#include <QTimer>
#include <QSignalSpy>

#include "MainWindow.h"
#include "LoginManager.h"
#include "Background/BackgroundWorkerBase.h"
#include "IgvSessionManager.h"

TEST_CLASS(AnalysisDataController_Test)
{


// Minimal worker used to demonstrate MainWindow's asynchronous job handling.
class TestBackgroundWorker : public BackgroundWorkerBase
{
public:
	TestBackgroundWorker()
		: BackgroundWorkerBase("MainWindow event-loop test worker")
	{
	}

	void process() override
	{
		// Nothing to calculate. The important part is that process()
		// is executed by QThreadPool rather than the GUI thread.
	}
};

void init_test(NGSD& db)
{
	db.init();
	db.executeQueriesFromFile(TESTDATA("data_in/NGSD_base_in.sql"));

	//add override paths for processed samples
	SqlQuery query = db.getQuery();
	query.prepare("UPDATE processed_sample SET folder_override = :0 WHERE id = :1");
	query.bindValue(0, TESTDATA("data_in/sample_data/Sample_NA12878-WES-DEFAULT_01/"));
	query.bindValue(1, 101);
	query.exec();
	query.bindValue(0, TESTDATA("data_in/sample_data/Sample_NA12878-WES-DRAGEN_01/"));
	query.bindValue(1, 102);
	query.exec();

}

/*
TEST_METHOD(controller_load_file)
{
	SKIP_IF_NO_TEST_NGSD();

	//init
	NGSD db(true);
	init_test(db);

	QString gsvar_default = TESTDATA("data_in/sample_data/Sample_NA12878-WES-DEFAULT_01/NA12878-WES-DEFAULT_01.GSvar");
	QString gsvar_dragen = TESTDATA("data_in/sample_data/Sample_NA12878-WES-DRAGEN_01/NA12878-WES-DRAGEN_01.GSvar");

	IgvSessionManager::create(nullptr, "test", Settings::path("igv_app").trimmed(), Settings::string("igv_host"), Settings::path("igv_genome"));

	AnalysisDataController& controller = AnalysisDataController::instance();
	IS_FALSE(controller.isValid());

	//load default analysis
	controller.loadFile(gsvar_default);
	IS_TRUE(controller.isValid());
	I_EQUAL(controller.getSmallVariantList().count(), 14351);

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

}

/*


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
	int argc = 1;
	char app_name[] = "GSvar-TEST";
	char* argv[] = {app_name, nullptr};
	//Application a(argc, argv);
	//MainWindow w;
	//w.showMaximized();

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



	// w.closeAndLogout();
	S_EQUAL("TODO", "DONE");
}

TEST_METHOD(report_configs)
{
    S_EQUAL("TODO", "DONE");
}

TEST_METHOD(use_mainwindow)
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

	query.prepare("UPDATE processed_sample SET folder_override_client = :0 WHERE id = :1");
	query.bindValue(0, TESTDATA("data_in/sample_data/Sample_NA12878-WES-DEFAULT_01/"));
	query.bindValue(1, 101);
	query.exec();
	query.bindValue(0, TESTDATA("data_in/sample_data/Sample_NA12878-WES-DRAGEN_01/"));
	query.bindValue(1, 102);
	query.exec();

	QString gsvar_default = TESTDATA("data_in/sample_data/Sample_NA12878-WES-DEFAULT_01/NA12878-WES-DEFAULT_01.GSvar");
	QString gsvar_dragen = TESTDATA("data_in/sample_data/Sample_NA12878-WES-DRAGEN_01/NA12878-WES-DRAGEN_01.GSvar");

	qWarning(gsvar_default.toUtf8());

	//sanity check init
	const QString processed_sample_name = QStringLiteral("NA12878-WES-DEFAULT_01");
	IS_TRUE(!db.processedSampleId(processed_sample_name, true).isEmpty());


	// MainWindow() may open this modal dialog when automatic configuration
	// is required. QMessageBox::question() runs a nested Qt event loop.
	//
	// Queue a callback before constructing MainWindow so it can close that
	// dialog from inside the nested event loop.
	QTimer::singleShot(0, []()
					   {
						   QWidget* modal = QApplication::activeModalWidget();
						   auto* box = qobject_cast<QMessageBox*>(modal);

						   if (box != nullptr)
						   {
							   box->reject();
						   }
					   });

	// Create the Qt application exactly as in the existing test setup.
	int argc = 1;
	char app_name[] = "GSvar-TEST";
	char* argv[] = { app_name, nullptr };

	Application a(argc, argv);

	// Construct MainWindow. The timer above prevents the constructor from
	// getting stuck in the configuration message box.
	MainWindow w;

	// Access the singleton controller used by MainWindow.
	AnalysisDataController& controller = AnalysisDataController::instance();

	// Make sure the test database contains the processed sample expected
	// by openProcessedSampleFromNGSD().

	//login to have DB access from Mainwindow
	LoginManager::login("ahmustm1", "", true);

	// Watch the controller signal that MainWindow connects to in its
	// constructor:
	//
	//   smallVariantsFilterResultChanged()
	//       -> MainWindow::refreshVariantTable()
	//
	QSignalSpy filter_result_changed(
		&controller,
		SIGNAL(smallVariantsFilterResultChanged())
		);

	QVERIFY(filter_result_changed.isValid());

	// Execute the actual MainWindow functionality under test.
	//
	// search_multi=false avoids the multi-analysis selection path.

	QTimer::singleShot(0, []()
					   {
						   QWidget* modal = QApplication::activeModalWidget();
						   auto* box = qobject_cast<QDialog*>(modal);

						   if (box != nullptr)
						   {
							   qWarning(box->windowTitle().toUtf8());
							   box->accept();
						   }
					   });

	qWarning("Loading gsvar file!");
	// w.loadFile(gsvar_default);

	// openProcessedSampleFromNGSD() -> loadFile() -> AnalysisDataController
	// is synchronous. Therefore the signal and MainWindow slot have already
	// been processed when the function returns.
	QVERIFY2(
		!filter_result_changed.isEmpty(),
		"Expected AnalysisDataController::smallVariantsFilterResultChanged()"
		);

	// Verify that the controller now contains the loaded analysis.
	QVERIFY(controller.isValid());
	QVERIFY(!controller.getFilename().isEmpty());

	// MainWindow's UI contains a VariantTable named "vars".
	// VariantTable is a QAbstractItemView, so we can inspect its model
	// without depending on the concrete VariantTable implementation.
	auto* variant_table = qobject_cast<QAbstractItemView*>(
		w.findChild<QWidget*>(QStringLiteral("vars"))
		);

	IS_TRUE(variant_table != nullptr);
	IS_TRUE(variant_table->model() != nullptr);

	// resetFilters() makes all variants pass initially. refreshVariantTable()
	// should therefore display the same number of rows as the filter result.
	QCOMPARE(
		variant_table->model()->rowCount(),
		controller.getSmallVariantsFilterResult().countPassing()
		);

	w.closeAndLogout();
}

TEST_METHOD(use_mainwindow_eventloop)
{
	// MainWindow() may show a modal "Configuration check" message box.
	// Arm this timer BEFORE constructing MainWindow.
	QTimer::singleShot(0, []()
					   {
						   QWidget* modal = QApplication::activeModalWidget();
						   auto* box = qobject_cast<QMessageBox*>(modal);

						   if (box != nullptr)
						   {
							   box->reject();
						   }
					   });

	// MainWindow::delayedInitialization() may open a LoginDialog once
	// the event loop starts. The guard prevents this test from becoming
	// interactive. It only runs while this test is active.
	QTimer modal_guard;
	QObject::connect(&modal_guard, &QTimer::timeout, []()
					 {
						 QWidget* modal = QApplication::activeModalWidget();
						 if (modal == nullptr) return;

						 // Only dismiss the dialogs which MainWindow can open during
						 // initialization. Do not close arbitrary application dialogs.
						 if (modal->metaObject()->className() == QByteArrayLiteral("LoginDialog"))
						 {
							 modal->close();
							 return;
						 }

						 auto* box = qobject_cast<QMessageBox*>(modal);
						 if (box != nullptr &&
							 (box->windowTitle() == QStringLiteral("GSvar setup error") ||
							  box->windowTitle() == QStringLiteral("Virus genome not set")))
						 {
							 box->reject();
						 }
					 });
	modal_guard.start(10);

	// Create the GUI application.
	int argc = 1;
	char app_name[] = "GSvar-TEST";
	char* argv[] = { app_name, nullptr };

	Application a(argc, argv);

	// Construct MainWindow.
	MainWindow w;

	// Start an actual background job through MainWindow.
	auto* worker = new TestBackgroundWorker();
	const int job_id = w.startJob(worker, false);

	QVERIFY(job_id >= 0);

	// BackgroundJobDialog::start() initially stores the job as "queued".
	QCOMPARE(w.getJobStatus(job_id), QStringLiteral("queued"));

	// The worker runs in QThreadPool.
	//
	// BackgroundWorkerBase emits finished() from the worker thread.
	// BackgroundJobDialog::finished() therefore has to be delivered to
	// the GUI thread through Qt's event system.
	//
	// QTRY_COMPARE_WITH_TIMEOUT() repeatedly processes events until
	// MainWindow reports that the job has finished.
	QTRY_COMPARE_WITH_TIMEOUT(
		w.getJobStatus(job_id),
		QStringLiteral("finished"),
		5000
		);

	// Give the event loop one more opportunity to process the queued
	// deleteLater() generated by BackgroundJobDialog::finished().
	QCoreApplication::processEvents();

	modal_guard.stop();

	w.closeAndLogout();
}
*/


};
