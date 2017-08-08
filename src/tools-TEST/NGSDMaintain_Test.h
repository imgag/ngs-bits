#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDMaintain_Test)
{
Q_OBJECT
private slots:

	//NOTE: merged sample processing is not tested
	void check_and_fix()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDMaintain_in1.sql"));

		//test, no fix
		QString messages;
		QTextStream stream(&messages);
		db.maintain(&stream, false);
		//qDebug() << messages;

		//test, fix
		QString messages2;
		QTextStream stream2(&messages2);
		db.maintain(&stream2, true);
		S_EQUAL(messages2, messages);
		//qDebug() << messages2;

		//test, nothing to fix
		QString messages3;
		QTextStream stream3(&messages3);
		db.maintain(&stream3, true);
		S_EQUAL(messages3, "");
		//qDebug() << messages3;
	}
};


