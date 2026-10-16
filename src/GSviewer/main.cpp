#include "MainWindow.h"
#include "Application.h"
#include <QStyleFactory>
#include <QCommandLineParser>
#include "Exceptions.h"

int main(int argc, char *argv[])
{
	try
	{
		Application a(argc, argv);
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
		QTextStream(stderr) << "Uncaught ngs-bits exception: " << e.message() << Qt::endl;
	}
	catch (std::exception& e)
	{
		QTextStream(stderr) << "Uncaught ngs-bits exception: " << e.what() << Qt::endl;
	}
	catch(...)
	{
		QTextStream(stderr) << "Uncaught ngs-bits exception: unknown exception" << Qt::endl;
	}

	return -1;
}
