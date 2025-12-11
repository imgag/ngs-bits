#include "MVHub.h"
#include "Application.h"
#include <QStyleFactory>

int main(int argc, char *argv[])
{
	Application a(argc, argv);
	MVHub w;
	w.setStyle(QStyleFactory::create("windowsvista"));
	w.showMaximized();
	
	return a.exec();
}
