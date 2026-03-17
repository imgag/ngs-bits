#include "TestFramework.h"

int main(int argc, char *argv[])
{
	qputenv("QT_QPA_PLATFORM", "offscreen");
	return TFW::run(argc, argv);
}
