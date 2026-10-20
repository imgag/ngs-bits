#include "MainWindow.h"
#include "Application.h"
#include <QStyleFactory>
#include <QCommandLineParser>
#include "Exceptions.h"
#include <QMessageBox>

int main(int argc, char *argv[])
{
	Application a(argc, argv);
	try
	{
		a.setStyle(QStyleFactory::create("windowsvista"));

        QCommandLineParser args;

        args.addHelpOption();
        args.addVersionOption();

        QCommandLineOption port("port", "The port or socket path GSviewer should receive commands on.", "<PORT | PATH>");
        args.addOption(port);

        args.process(a);

        MainWindow w(args.value("port"));
		w.showMaximized();

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
	catch(...)
	{
		QMessageBox::critical(nullptr, "GSviewer startup error", "unknown exception type");
	}

	return -1;
}
