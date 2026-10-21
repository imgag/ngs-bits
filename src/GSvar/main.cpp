#include "MainWindow.h"
#include "Application.h"
#include "MultiInstanceHandler.h"

int main(int argc, char *argv[])
{
	Application a(argc, argv);

	const QStringList args = a.arguments();
	QUrl initial_url;
	if (args.size() > 1)
	{
		const QUrl url(args.at(1));
		if (url.scheme() == "gsvar") initial_url = url;
	}

	MultiInstanceHandler multi_instance_handler;

	// If another instance is running, forward the URL and exit
	if (multi_instance_handler.anotherInstanceExists())
	{
		if (initial_url.isValid() && multi_instance_handler.forwardToExistingInstance(initial_url)) return 0;

		Log::error("Another GSvar instance exists, but URL forwarding failed");
		return 1;
	}

	// Start the server for subsequent instances
	if (!multi_instance_handler.startServer()) Log::error("Failed to start the server for handling multiple instances: " + multi_instance_handler.errorString());

	MainWindow w;
	QObject::connect(
		&multi_instance_handler,
		&MultiInstanceHandler::urlReceived,
		&w,
		[&w](const QUrl url)
		{
			w.showNormal();
			w.raise();
			w.activateWindow();
			w.openGSvarUrl(url);
	});
	w.showMaximized();

	// Process the URL that launched this first instance
	if (initial_url.isValid()) emit multi_instance_handler.urlReceived(initial_url);

	return a.exec();
}