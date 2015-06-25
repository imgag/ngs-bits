#include "../TestFramework.h"
#include "Settings.h"

class GenesToApproved_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		QString db_file = Settings::string("hgnc");
		if (db_file=="") QSKIP("Test needs a database file!");

		TFW_EXEC("GenesToApproved", "-in " + QFINDTESTDATA("data_in/GenesToApproved_in1.txt") + " -out out/GenesToApproved_out1.txt -db " + db_file);
		TFW::comareFiles("out/GenesToApproved_out1.txt", QFINDTESTDATA("data_out/GenesToApproved_out1.txt"));
	}

};

TFW_DECLARE(GenesToApproved_Test)


