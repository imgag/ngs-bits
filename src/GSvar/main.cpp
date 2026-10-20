#include "MainWindow.h"
#include "Application.h"
#include <QMessageBox>

int main(int argc, char *argv[])
{
	Application a(argc, argv);
	try
	{
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
