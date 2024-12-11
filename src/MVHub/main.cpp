#include "MVHub.h"
#include "Application.h"

int main(int argc, char *argv[])
{
	Application a(argc, argv);
	MVHub w;
	w.showMaximized();
	
	return a.exec();
}
