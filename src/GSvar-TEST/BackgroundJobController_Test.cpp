
#include "TestFramework.h"
#include "Background/BackgroundJobController.h"

#include <QTest>
#include <QSignalSpy>

#include "Background/BackgroundWorkerBase.h"

TEST_CLASS(BackgroundJobController_Test)
{

// Test worker to check
class TestBackgroundWorker : public BackgroundWorkerBase
{
public:
	TestBackgroundWorker(QString suffix, int sleep_ms, bool error=false)
		: BackgroundWorkerBase("TestBackgroundWorker_" + suffix)
		, sleep_ms_(sleep_ms)
		, error_(error)
	{
	}

	void process() override
	{
		QTest::qWait(sleep_ms_);
		if (error_) THROW(ProgrammingException, "TestBackgroundWorker: Error flag was set!");
	}

private:
	int sleep_ms_;
	bool error_;
};


TEST_METHOD(run_job)
{
	BackgroundJobController& controller = BackgroundJobController::instance();

	TestBackgroundWorker* wk1 = new TestBackgroundWorker("run_job1", 0);

	QSignalSpy start_signal(wk1, SIGNAL(started()));
	QSignalSpy finished_signal(wk1, SIGNAL(finished()));
	QSignalSpy error_signal(wk1, SIGNAL(failed()));
	QSignalSpy busy_signal(&controller, SIGNAL(showBusyDialog(QString, BusyDialog*)));


	int job_id = controller.start(wk1, true);


	QTest::qWait(5);
	//execute called slots (started() , finished())
	QCoreApplication::processEvents();


	I_EQUAL(job_id, 0);
	I_EQUAL(start_signal.count(), 1);
	I_EQUAL(finished_signal.count(), 1);
	I_EQUAL(error_signal.count(), 0);
	I_EQUAL(busy_signal.count(), 1);
	S_EQUAL(controller.getJobStatus(job_id), "finished");

	TestBackgroundWorker* wk2 = new TestBackgroundWorker("run_job2", 0, true);

	QSignalSpy start_signal2(wk2, SIGNAL(started()));
	QSignalSpy finished_signal2(wk2, SIGNAL(finished()));
	QSignalSpy error_signal2(wk2, SIGNAL(failed()));

	int job_id2 = controller.start(wk2, false);

	QTest::qWait(5);
	QCoreApplication::processEvents();

	I_EQUAL(job_id2, 1);
	I_EQUAL(start_signal2.count(), 1);
	I_EQUAL(finished_signal2.count(), 0);
	I_EQUAL(error_signal2.count(), 1);
	I_EQUAL(busy_signal.count(), 1); // no new signal was emitted
	S_EQUAL(controller.getJobStatus(job_id2), "failed");
	S_EQUAL(controller.getJobMessages(job_id2), "TestBackgroundWorker: Error flag was set!");
}

TEST_METHOD(run_long_job)
{
	BackgroundJobController& controller = BackgroundJobController::instance();

	TestBackgroundWorker* wk = new TestBackgroundWorker("run_long_job", 20);

	QSignalSpy start_signal(wk, SIGNAL(started()));
	QSignalSpy finished_signal(wk, SIGNAL(finished()));
	QSignalSpy error_signal(wk, SIGNAL(failed()));
	QSignalSpy busy_signal(&controller, SIGNAL(showBusyDialog(QString, BusyDialog*)));


	int job_id = controller.start(wk, true);
	S_EQUAL(controller.getJobStatus(job_id), "queued")

	QTest::qWait(5);
	QCoreApplication::processEvents();

	I_EQUAL(start_signal.count(), 1);
	I_EQUAL(finished_signal.count(), 0);
	I_EQUAL(error_signal.count(), 0);
	I_EQUAL(busy_signal.count(), 1);
	S_EQUAL(controller.getJobStatus(job_id), "started");

	QTest::qWait(17);
	QCoreApplication::processEvents();

	I_EQUAL(start_signal.count(), 1);
	I_EQUAL(finished_signal.count(), 1);
	I_EQUAL(error_signal.count(), 0);
	I_EQUAL(busy_signal.count(), 1);
	S_EQUAL(controller.getJobStatus(job_id), "finished");
}

TEST_METHOD(run_multiple_jobs)
{
	BackgroundJobController& controller = BackgroundJobController::instance();

	TestBackgroundWorker* wk1 = new TestBackgroundWorker("job1", 20);
	TestBackgroundWorker* wk2 = new TestBackgroundWorker("job2", 20);
	TestBackgroundWorker* wk3 = new TestBackgroundWorker("job3", 20);
	TestBackgroundWorker* wk4 = new TestBackgroundWorker("job4", 20);

	QSignalSpy busy_signal(&controller, SIGNAL(showBusyDialog(QString, BusyDialog*)));

	QSignalSpy start_signal1(wk1, SIGNAL(started()));
	QSignalSpy finished_signal1(wk1, SIGNAL(finished()));

	QSignalSpy start_signal2(wk2, SIGNAL(started()));
	QSignalSpy finished_signal2(wk2, SIGNAL(finished()));

	QSignalSpy start_signal3(wk3, SIGNAL(started()));
	QSignalSpy finished_signal3(wk3, SIGNAL(finished()));

	QSignalSpy start_signal4(wk4, SIGNAL(started()));
	QSignalSpy finished_signal4(wk4, SIGNAL(finished()));

	int job_id1 = controller.start(wk1, false);
	int job_id2 = controller.start(wk2, false);
	int job_id3 = controller.start(wk3, false);
	int job_id4 = controller.start(wk4, false);

	S_EQUAL(controller.getJobStatus(job_id1), "queued");
	S_EQUAL(controller.getJobStatus(job_id2), "queued");
	S_EQUAL(controller.getJobStatus(job_id3), "queued");
	S_EQUAL(controller.getJobStatus(job_id4), "queued");

	QTest::qWait(5);
	QCoreApplication::processEvents();

	I_EQUAL(start_signal1.count(), 1);
	I_EQUAL(start_signal2.count(), 1);
	I_EQUAL(start_signal3.count(), 1);
	I_EQUAL(start_signal4.count(), 0);

	S_EQUAL(controller.getJobStatus(job_id1), "started");
	S_EQUAL(controller.getJobStatus(job_id2), "started");
	S_EQUAL(controller.getJobStatus(job_id3), "started");
	S_EQUAL(controller.getJobStatus(job_id4), "queued");

	QTest::qWait(20);
	QCoreApplication::processEvents();

	I_EQUAL(start_signal4.count(), 1);

	I_EQUAL(finished_signal1.count(), 1);
	I_EQUAL(finished_signal2.count(), 1);
	I_EQUAL(finished_signal3.count(), 1);
	I_EQUAL(finished_signal4.count(), 0);

	S_EQUAL(controller.getJobStatus(job_id1), "finished");
	S_EQUAL(controller.getJobStatus(job_id2), "finished");
	S_EQUAL(controller.getJobStatus(job_id3), "finished");
	S_EQUAL(controller.getJobStatus(job_id4), "started");

	QTest::qWait(20);
	QCoreApplication::processEvents();

	I_EQUAL(finished_signal1.count(), 1);
	I_EQUAL(finished_signal2.count(), 1);
	I_EQUAL(finished_signal3.count(), 1);
	I_EQUAL(finished_signal4.count(), 1);

	S_EQUAL(controller.getJobStatus(job_id1), "finished");
	S_EQUAL(controller.getJobStatus(job_id2), "finished");
	S_EQUAL(controller.getJobStatus(job_id3), "finished");
	S_EQUAL(controller.getJobStatus(job_id4), "finished");
}

};