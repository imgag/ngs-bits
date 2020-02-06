#include "MainWindow.h"
#include "Application.h"

//TODO remove GSmix and desktop link when not used anymore

int main(int argc, char *argv[])
{
	Application a(argc, argv);
	MainWindow w;
	w.show();
	
	return a.exec();
}
