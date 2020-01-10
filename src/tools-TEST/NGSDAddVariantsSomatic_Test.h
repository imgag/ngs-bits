#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"


TEST_CLASS(NGSDAddVariantsSomatic_Test)
{
Q_OBJECT
private slots:

	void adsfdf()
	{
		qDebug() <<  "werde ausgefuehrt" << endl;
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsSomatic_init.sql"));

		//1. import
		EXECUTE("NGSDAddVariantsSomatic", "-test -no_time -t_ps DX184894_01 -n_ps DX184263_01 -var " + TESTDATA("data_in/NGSDAddVariantsGermline_in1.GSvar")) );
	}

	void blubb()
	{
		IS_TRUE(true);
		IS_TRUE(false);
	}


};
