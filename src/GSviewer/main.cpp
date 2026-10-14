#include "MainWindow.h"
#include "Application.h"
#include <QStyleFactory>
#include "Exceptions.h"

int main(int argc, char *argv[])
{
	try
	{
		Application a(argc, argv);
		a.setStyle(QStyleFactory::create("windowsvista"));

		MainWindow w;
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
