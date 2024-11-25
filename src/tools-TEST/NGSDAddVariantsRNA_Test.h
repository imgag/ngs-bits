#include "TestFramework.h"
#include "NGSD.h"
#include "Settings.h"
#include <QSqlRecord>

TEST_CLASS(NGSDAddVariantsRna_Test)
{
Q_OBJECT

	bool checkLogFile(QString logfile, QByteArray searchterm)
	{
		QFile file(logfile);
		file.open(QIODevice::ReadOnly);
		while(! file.atEnd())
		{
			if (file.readLine().contains(searchterm))
			{
				return true;
			}
		}
		return false;
	}

private slots:
	void test_addFusions()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDAddVariantsSomatic_init.sql"));

		EXECUTE("NGSDAddVariantsRNA", "-test -no_time -ps RNA184894_01 -fusion " + TESTDATA("data_in/NGSDAddVariantsRNA_in1.tsv"));

		DBTable table = db.createTable("test", "SELECT * FROM rna_fusion");

		I_EQUAL(table.rowCount(), 21); //21 imported fusions
		S_EQUAL(table.row(0).asString(';'), "1;1;NAPG;chr18;10530837;ENST00000322897;SRPK2(1688),PUS7(38665);chr7;105400996;.;translocation;out-of-frame");
		S_EQUAL(table.row(1).asString(';'), "2;1;TSHZ2;chr20;53256444;ENST00000371497;TSHZ2;chr20;53487144;ENST00000371497;deletion/read-through;out-of-frame");
		S_EQUAL(table.row(2).asString(';'), "3;1;ENSG00000271774;chr20;53256444;.;TSHZ2;chr20;53487144;ENST00000371497;deletion/read-through/3'-3';.");
		S_EQUAL(table.row(3).asString(';'), "4;1;DIAPH2;chrX;96763120;ENST00000324765;DIAPH2;chrX;96881579;ENST00000324765;deletion/read-through;in-frame");
		S_EQUAL(table.row(4).asString(';'), "5;1;LRBA;chr4;150599059;ENST00000357115;LRBA;chr4;150342832;ENST00000357115;deletion/read-through;in-frame");

		//Fusions already imported -> import is skipped, check for skipped in log-file
		EXECUTE("NGSDAddVariantsRNA", "-test -no_time -ps RNA184894_01 -fusion " + TESTDATA("data_in/NGSDAddVariantsRNA_in1.tsv"));
		IS_TRUE(checkLogFile("out/NGSDAddVariantsRNA_Test_line45.log", "Skipped import of fusion variants for sample RNA184894_01: fusion callset already exists for this sample!"));


		//Fusions already imported force
		EXECUTE("NGSDAddVariantsRNA", "-fusion_force -test -no_time -ps RNA184894_01 -fusion " + TESTDATA("data_in/NGSDAddVariantsRNA_in1.tsv"));
	}
};
