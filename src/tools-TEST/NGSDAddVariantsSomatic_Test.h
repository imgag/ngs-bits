#include "TestFramework.h"
#include "NGSD.h"
#include "Settings.h"
#include <QSqlRecord>

TEST_CLASS(NGSDAddVariantsSomatic_Test)
{
Q_OBJECT
private slots:
	void test_addSmallVariants()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsSomatic_init.sql"));
		EXECUTE("NGSDAddVariantsSomatic", "-test -no_time -t_ps DX184894_01 -n_ps DX184263_01 -var " + TESTDATA("data_in/NGSDAddVariantsSomatic_in1.GSvar"));

		S_EQUAL(db.variant("1").toString(), "chr2:178096717-178096717 T>C");
		S_EQUAL(db.variant("2").toString(), "chr3:138456487-138456488 AT>-");
		S_EQUAL(db.variant("3").toString(), "chr16:56870524-56870524 A>C");

		//Check variant entries in detected_somatic_variants
		DBTable table = db.createTable("test", "SELECT * FROM detected_somatic_variant");
		I_EQUAL(table.rowCount(), 3);
		S_EQUAL(table.row(0).asString(';'), "1;8;7;1;0.1057;389;229");
		S_EQUAL(table.row(1).asString(';'), "2;8;7;2;0.1304;26;22");
		S_EQUAL(table.row(2).asString(';'), "3;8;7;3;0.1254;639;330");
	}
};
