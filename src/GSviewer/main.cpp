#include "MainWindow.h"
#include "Application.h"
#include <QStyleFactory>

int main(int argc, char *argv[])
{
	Application a(argc, argv);
	a.setStyle(QStyleFactory::create("windowsvista"));

	MainWindow w;
	w.showMaximized();
	
	return a.exec();
}
