#include "../TestFramework.h"

int main(int argc, char *argv[])
{
	int fails = TFW::run(argc, argv);
	qDebug() << "#########" << fails << "TEST(S) FAILED #########";
	return fails;
}
