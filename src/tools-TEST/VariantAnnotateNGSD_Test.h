#include "TestFramework.h"
#include "VariantList.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(VariantAnnotateNGSD_Test)
{
Q_OBJECT
private slots:
	
	void germline()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/VariantAnnotateNGSD_init1.sql"));

		//test
		EXECUTE("VariantAnnotateNGSD", "-test -in " + TESTDATA("data_in/VariantAnnotateNGSD_in1.tsv") + " -out out/VariantAnnotateNGSD_out1.tsv");
		COMPARE_FILES("out/VariantAnnotateNGSD_out1.tsv", TESTDATA("data_out/VariantAnnotateNGSD_out1.tsv"));
	}

	void germline_with_psname()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/VariantAnnotateNGSD_init1.sql"));

		//test
		EXECUTE("VariantAnnotateNGSD", "-test -psname DUMMY_01 -in " + TESTDATA("data_in/VariantAnnotateNGSD_in1.tsv") + " -out out/VariantAnnotateNGSD_out2.tsv");
		COMPARE_FILES("out/VariantAnnotateNGSD_out2.tsv", TESTDATA("data_out/VariantAnnotateNGSD_out2.tsv"));
	}

	void germline_empty_input_file()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/VariantAnnotateNGSD_init1.sql"));

		//test
		EXECUTE("VariantAnnotateNGSD", "-test -in " + TESTDATA("data_in/VariantAnnotateNGSD_in2.tsv") + " -out out/VariantAnnotateNGSD_out4.tsv");
		COMPARE_FILES("out/VariantAnnotateNGSD_out4.tsv", TESTDATA("data_out/VariantAnnotateNGSD_out4.tsv"));
	}

	void somatic()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/VariantAnnotateNGSD_init2.sql"));

		EXECUTE("VariantAnnotateNGSD", "-test -in " + TESTDATA("data_in/VariantAnnotateNGSD_in3.tsv") + " -out out/VariantAnnotateNGSD_out3.tsv -mode somatic");

		VariantList output;
		output.load("out/VariantAnnotateNGSD_out3.tsv");

		//check that the new columns are present
		int val_i1 = output.annotationIndexByName("NGSD_som_c", true, false);
		I_EQUAL(val_i1, 22);
		int val_i2 = output.annotationIndexByName("NGSD_som_p", true, false);
		I_EQUAL(val_i2, 23);
		I_EQUAL(output.annotations().count(), 24);

		//check annotation content
		I_EQUAL(output.count(), 5);
		for (int i=0; i<output.count(); ++i)
		{
			if (output[i].chr().str()=="chr1" && output[i].start()==62728784)//somatic variant found previously
			{
				bool ok = false;
				IS_TRUE(output[i].annotations()[val_i1].toInt(&ok)==1);
				IS_TRUE(ok);
				IS_TRUE(output[i].annotations()[val_i2]!="");
			}
			else if (output[i].chr().str()=="chr3" && output[i].start()==142277575)//somatic variant found previously
			{
				bool ok = false;
				IS_TRUE(output[i].annotations()[val_i1].toInt(&ok)==2);
				IS_TRUE(ok);
				IS_TRUE(output[i].annotations()[val_i2]!="");
			}
			else if (output[i].chr().str()=="chr2" && output[i].start()==48032740)//somatic variant found previously
			{
				bool ok = false;
				IS_TRUE(output[i].annotations()[val_i1].toInt(&ok)==1);
				IS_TRUE(ok);
				IS_TRUE(output[i].annotations()[val_i2]!="");
			}
			else //somatic variants unknown so far
			{
				S_EQUAL(output[i].annotations()[val_i1], QByteArray("0"));
				S_EQUAL(output[i].annotations()[val_i2], QByteArray(""));
			}
		}
	}
	
};


