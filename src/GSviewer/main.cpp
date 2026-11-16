#include "MainWindow.h"
#include "Application.h"
#include "Helper.h"
#include <QStyleFactory>
#include <QCommandLineParser>
#include "Exceptions.h"
#include <QMessageBox>

int main(int argc, char* argv[])
{
	Application a(argc, argv);
	try
	{
		a.setStyle(QStyleFactory::create("windowsvista"));

		//define CLI
		QCommandLineParser args;
		args.addHelpOption();
		args.addVersionOption();
		args.addOption(QCommandLineOption("port", "The port GSviewer should receive commands on. The commands are a sub-set of the IGV batch commands.", "port"));
		args.process(a);

		//start app
		MainWindow w;
		w.showMaximized();

		//open port if requested
		QString port_str = args.value("port");
		if (!port_str.isEmpty())
		{
			w.openPort(Helper::toInt(port_str, "CLI parameter 'port'"));
		}

		return a.exec();
	}
	catch (Exception& e)
	{
		QMessageBox::critical(nullptr, "GSviewer startup error", e.message());
	}
	catch (std::exception& e)
	{
		QMessageBox::critical(nullptr, "GSviewer startup error", e.what());
	}
	catch (...)
	{
		QMessageBox::critical(nullptr, "GSviewer startup error", "unknown exception type");
	}

	return -1;
}
