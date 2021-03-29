#include "TestFramework.h"
#include "HttpsServer.h"
#include "ServerHelper.h"
#include "WorkerThread.h"
#include <QJsonDocument>
#include <QJsonObject>

class FakeRequestHandler : public QObject {
	Q_OBJECT
	public:
	inline FakeRequestHandler()
	{
		qDebug() << "=======================================";


	};
	inline ~FakeRequestHandler()
	{

	}

	inline void handleResults(const Response &response)
	{
		qDebug() << "handleResults *******************************************";
//		QJsonDocument json_doc = QJsonDocument::fromJson(response.body);
//		QJsonObject json_obj = json_doc.object();
//		qDebug() << "MSG =" << json_obj.value("Error message").toString();

	}

	inline void exceute()
	{
		qDebug() << "/////////////------------------------------------------------------------";
		Request request {};
		request.method = Request::MethodType::GET;
		request.return_type = ContentType::APPLICATION_JSON;
		request.prefix = "v1";
		request.path = "info";

		WorkerThread *workerThread = new WorkerThread(request);
		connect(workerThread, &WorkerThread::resultReady, this, &FakeRequestHandler::handleResults);
		connect(workerThread, &WorkerThread::finished, workerThread, &QObject::deleteLater);
		workerThread->start();
	}
};

TEST_CLASS(WorkerThread_Test)
{
Q_OBJECT


private slots:

	void test_run()
	{

		FakeRequestHandler *f = new FakeRequestHandler();

		f->exceute();





		QThread::msleep(5000);
		IS_TRUE(1);

	}
};
