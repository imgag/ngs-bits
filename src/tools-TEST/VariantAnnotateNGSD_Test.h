#include "TestFramework.h"
#include "VariantList.h"
#include "Settings.h"

TEST_CLASS(VariantAnnotateNGSD_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		QString host = Settings::string("ngsd_host");
		if (host=="") SKIP("Test needs access to the NGSD!");

		bool convertion_ok;
	
		EXECUTE("VariantAnnotateNGSD", "-in " + TESTDATA("data_in/VariantAnnotateNGSD_in1.tsv") + " -out out/VariantAnnotateNGSD_out1.tsv");
	
		VariantList output;
		output.load("out/VariantAnnotateNGSD_out1.tsv");
	
		//check that the new columns are present
		int hom_i = output.annotationIndexByName("ihdb_hom", true, true);
		I_EQUAL(hom_i, 22);
		int het_i = output.annotationIndexByName("ihdb_het", true, true);
		I_EQUAL(het_i, 23);
		int wt_i = output.annotationIndexByName("ihdb_wt", true, true);
		I_EQUAL(wt_i, 24);
		int all_hom_i = output.annotationIndexByName("ihdb_allsys_hom", true, true);
		I_EQUAL(all_hom_i, 25);
		int all_het_i = output.annotationIndexByName("ihdb_allsys_het", true, true);
		I_EQUAL(all_het_i, 26);
		int cla_i = output.annotationIndexByName("classification", true, true);
		I_EQUAL(cla_i, 27);
		int val_i = output.annotationIndexByName("validated", true, true);
		I_EQUAL(val_i, 28);
		I_EQUAL(output.annotations().count(), 29);
	
		//check that 'comment' column is missing
		I_EQUAL(output.annotationIndexByName("comment", true, false), -1);
	
		//check annotation content
		I_EQUAL(output.count(), 11);
		for (int i=0; i<output.count(); ++i)
		{
			S_EQUAL(output[i].annotations()[hom_i], QByteArray("n/a"));
			S_EQUAL(output[i].annotations()[het_i], QByteArray("n/a"));
			S_EQUAL(output[i].annotations()[wt_i],  QByteArray("n/a"));

			convertion_ok = false;
			int value_int = output[i].annotations()[all_hom_i].toInt(&convertion_ok);
			IS_TRUE(convertion_ok);
			IS_TRUE(value_int >= 0);
	
			convertion_ok = false;
			value_int = output[i].annotations()[all_het_i].toInt(&convertion_ok);
			IS_TRUE(convertion_ok);
			IS_TRUE(value_int >= 0);
	
			S_EQUAL(output[i].annotations()[cla_i], QByteArray("n/a"));
			S_EQUAL(output[i].annotations()[val_i], QByteArray("n/a"));
		}
	}

	void test_02()
	{	
		QString host = Settings::string("ngsd_host");
		if (host=="") SKIP("Test needs access to the NGSD!");

	    bool convertion_ok;
	
		EXECUTE("VariantAnnotateNGSD", "-in " + TESTDATA("data_in/VariantAnnotateNGSD_in1.tsv") + " -out out/VariantAnnotateNGSD_out2.tsv -psname DX131731_01");
	
		VariantList output;
		output.load("out/VariantAnnotateNGSD_out2.tsv");
	
		//check that the new columns are present
		int hom_i = output.annotationIndexByName("ihdb_hom", true, true);
		I_EQUAL(hom_i, 22);
		int het_i = output.annotationIndexByName("ihdb_het", true, true);
		I_EQUAL(het_i, 23);
		int wt_i = output.annotationIndexByName("ihdb_wt", true, true);
		I_EQUAL(wt_i, 24);
		int all_hom_i = output.annotationIndexByName("ihdb_allsys_hom", true, true);
		I_EQUAL(all_hom_i, 25);
		int all_het_i = output.annotationIndexByName("ihdb_allsys_het", true, true);
		I_EQUAL(all_het_i, 26);
		int cla_i = output.annotationIndexByName("classification", true, true);
		I_EQUAL(cla_i, 27);
		int val_i = output.annotationIndexByName("validated", true, true);
		I_EQUAL(val_i, 28);
		I_EQUAL(output.annotations().count(), 29);
	
		//check that 'comment' column is missing
		I_EQUAL(output.annotationIndexByName("comment", true, false), -1);
	
		//check annotation content
		I_EQUAL(output.count(), 11);
		for (int i=0; i<output.count(); ++i)
		{
			double value_double = output[i].annotations()[hom_i].toDouble(&convertion_ok);
			IS_TRUE(convertion_ok);
			IS_TRUE(value_double >= 0.0 && value_double <= 1.0);
	
			value_double = output[i].annotations()[hom_i].toDouble(&convertion_ok);
			IS_TRUE(convertion_ok);
			IS_TRUE(value_double >= 0.0 && value_double <= 1.0);
	
			value_double = output[i].annotations()[het_i].toDouble(&convertion_ok);
			IS_TRUE(convertion_ok);
			IS_TRUE(value_double >= 0.0 && value_double <= 1.0);
	
			value_double = output[i].annotations()[wt_i].toDouble(&convertion_ok);
			IS_TRUE(convertion_ok);
			IS_TRUE(value_double >= 0.0 && value_double <= 1.0);
	
			convertion_ok = false;
			int value_int = output[i].annotations()[all_hom_i].toInt(&convertion_ok);
			IS_TRUE(convertion_ok);
			IS_TRUE(value_int >= 0);
	
			convertion_ok = false;
			value_int = output[i].annotations()[all_het_i].toInt(&convertion_ok);
			IS_TRUE(convertion_ok);
			IS_TRUE(value_int >= 0);
	
			S_EQUAL(output[i].annotations()[cla_i], QByteArray("n/a"));
			S_EQUAL(output[i].annotations()[val_i], QByteArray("n/a"));
		}
	}

	void test_03()
	{
		QString host = Settings::string("ngsd_host");
		if (host=="") SKIP("Test needs access to the NGSD!");

		EXECUTE("VariantAnnotateNGSD", "-in " + TESTDATA("data_in/VariantAnnotateNGSD_in3.tsv") + " -out out/VariantAnnotateNGSD_out3.tsv -mode somatic");

		VariantList output;
		output.load("out/VariantAnnotateNGSD_out3.tsv");

		//check that the new columns are present
		int val_i1 = output.annotationIndexByName("som_ihdb_c", true, false);
		I_EQUAL(val_i1, 22);
		int val_i2 = output.annotationIndexByName("som_ihdb_p", true, false);
		I_EQUAL(val_i2, 23);
		I_EQUAL(output.annotations().count(), 24);

		//check annotation content
		I_EQUAL(output.count(), 5);
		for (int i=0; i<output.count(); ++i)
		{
			if (output[i].chr().str()=="chr1")//known somatic variants
			{
				bool ok = false;
				IS_TRUE(output[i].annotations()[val_i1].toInt(&ok)>=1);
				IS_TRUE(ok);
				IS_TRUE(output[i].annotations()[val_i2]!="");
			}
			else //unknown somatic variants
			{
				S_EQUAL(output[i].annotations()[val_i1], QByteArray("0"));
				S_EQUAL(output[i].annotations()[val_i2], QByteArray(""));
			}
		}
	}
	
};


