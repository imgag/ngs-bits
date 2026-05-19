#include "PingWorker.h"
#include "HttpHandler.h"
#include <QCoreApplication>

PingWorker::PingWorker()
	: BackgroundWorkerBase("Startup Worker")
{
}

void PingWorker::process()
{
	QString version = QCoreApplication::applicationVersion().left(7);

	HttpHandler http_handler(false);
	http_handler.get("https://megsap.de/stats/gsvar.php?version="+version);
}
