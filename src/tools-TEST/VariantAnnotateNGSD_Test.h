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

	void somatic()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");

		SKIP("not implemented"); //TODO implement somatic test

		/*
		EXECUTE("VariantAnnotateNGSD", "-test -in " + TESTDATA("data_in/VariantAnnotateNGSD_in3.tsv") + " -out out/VariantAnnotateNGSD_out3.tsv -mode somatic");

		VariantList output;
		output.load("out/VariantAnnotateNGSD_out3.tsv");

		//check that the new columns are present
		int gpd_g_i = output.annotationIndexByName("GPD_gene", true, true);
		I_EQUAL(gpd_g_i, 22);
		int gpd_v_i = output.annotationIndexByName("GPD_var", true, true);
		I_EQUAL(gpd_v_i, 23);
		int val_i1 = output.annotationIndexByName("som_ihdb_c", true, false);
		I_EQUAL(val_i1, 24);
		int val_i2 = output.annotationIndexByName("som_ihdb_p", true, false);
		I_EQUAL(val_i2, 25);
		I_EQUAL(output.annotations().count(), 26);

		//check annotation content
		I_EQUAL(output.count(), 5);
		for (int i=0; i<output.count(); ++i)
		{
			if (output[i].chr().str()=="chr1")//somatic variants found previously
			{
				bool ok = false;
				IS_TRUE(output[i].annotations()[val_i1].toInt(&ok)>=1);
				IS_TRUE(ok);
				IS_TRUE(output[i].annotations()[val_i2]!="");
			}
			else //somatic variants unknown so far
			{
				S_EQUAL(output[i].annotations()[val_i1], QByteArray("0"));
				S_EQUAL(output[i].annotations()[val_i2], QByteArray(""));
			}

			if(output[i].chr().str()=="chr3" || output[i].chr().str()=="chr15")//somatic variants with gene annotations but no variant annotations in GPD
			{
				IS_TRUE(output[i].annotations()[gpd_g_i].length()>0);
				IS_TRUE(output[i].annotations()[gpd_v_i]=="");
			}
			else//somatic variants without annotations
			{
				S_EQUAL(output[i].annotations()[gpd_g_i], QByteArray(""));
				S_EQUAL(output[i].annotations()[gpd_v_i], QByteArray(""));
			}
		}
		*/
	}
	
};


