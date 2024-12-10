#include "MainWindow.h"
#include "Application.h"

int main(int argc, char *argv[])
{
	Application a(argc, argv);
	MainWindow w;
	w.showMaximized();
	
	return a.exec();
}
