#include "PingWorker.h"
#include <QCoreApplication>
#include "HttpRequestHandler.h"

PingWorker::PingWorker()
	: BackgroundWorkerBase("Startup Worker")
{
}

void PingWorker::process()
{
	QString version = QCoreApplication::applicationVersion().left(7);

	HttpRequestHandler http_handler;
	http_handler.get("https://megsap.de/stats/gsvar.php?version="+version);
}
