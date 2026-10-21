#include "MainWindow.h"
#include "Application.h"
#include "MultiInstanceHandler.h"

int main(int argc, char *argv[])
{
	Application a(argc, argv);

	// Before rendering the main window, we need to check if another instance already running
	MultiInstanceHandler multi_instance_handler;
	if (multi_instance_handler.anotherInstanceExists())
	{
		qDebug() << "Another running instance of GSvar has been detected";

		const QStringList args = a.arguments();
		QUrl initial_url;
		if (args.size() > 1)
		{
			const QUrl url(args.at(1));
			if (url.scheme() == "gsvar") initial_url = url;
		}

		// Forward the URL and exit
		if (initial_url.isValid() && multi_instance_handler.forwardToExistingInstance(initial_url)) return 0;
	}

	MainWindow w;
	w.showMaximized();

	return a.exec();
}