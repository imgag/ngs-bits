#include "MainWindow.h"
#include "Application.h"
#include "MultiInstanceHandler.h"
#include <QMessageBox>

int main(int argc, char *argv[])
{
	Application a(argc, argv);
	try
	{
		// Before rendering the main window, we need to check if another instance already running
		if (Helper::isWindows())
		{
			MultiInstanceHandler multi_instance_handler;
			if (multi_instance_handler.anotherInstanceExists())
			{
				qDebug() << "Another running instance of GSvar has been detected";

				const QStringList args = a.arguments();
				if (args.size()==2)
				{
					const QUrl url(args.at(1));
					if (url.scheme() == "gsvar")
					{
						if (multi_instance_handler.forwardToExistingInstance(url))
						{
							qDebug() << "Forwarded URL to existing instance:" + url.toString();
							return 0;
						}
					}
				}
			}
		}
    
		MainWindow w;
		w.showMaximized();    
		return a.exec();
	}
	catch (Exception& e)
	{
		QMessageBox::critical(nullptr, "GSvar startup error", e.message());
	}
	catch (std::exception& e)
	{
		QMessageBox::critical(nullptr, "GSvar startup error", e.what());
	}
	catch(...)
	{
		QMessageBox::critical(nullptr, "GSvar startup error", "unknown exception type");
	}

	return -1;
}
