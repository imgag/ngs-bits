#include "TestFramework.h"
#include "VariantList.h"
#include "Settings.h"

TEST_CLASS(VariantAnnotateNGSD_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		QString host = Settings::string("ngsd_host");
		if (host=="") SKIP("Test needs access to the NGSD!");

		bool convertion_ok;
	
		EXECUTE("VariantAnnotateNGSD", "-in " + TESTDATA("data_in/VariantAnnotateNGSD_in1.tsv") + " -out out/VariantAnnotateNGSD_out1.tsv");
	
		VariantList output;
		output.load("out/VariantAnnotateNGSD_out1.tsv");
	
		//check that the new columns are present
		int gpd_g_i = output.annotationIndexByName("GPD_gene", true, true);
		I_EQUAL(gpd_g_i, 22);
		int gpd_v_i = output.annotationIndexByName("GPD_var", true, true);
		I_EQUAL(gpd_v_i, 23);
		int hom_i = output.annotationIndexByName("ihdb_hom", true, true);
		I_EQUAL(hom_i, 24);
		int het_i = output.annotationIndexByName("ihdb_het", true, true);
		I_EQUAL(het_i, 25);
		int wt_i = output.annotationIndexByName("ihdb_wt", true, true);
		I_EQUAL(wt_i, 26);
		int all_hom_i = output.annotationIndexByName("ihdb_allsys_hom", true, true);
		I_EQUAL(all_hom_i, 27);
		int all_het_i = output.annotationIndexByName("ihdb_allsys_het", true, true);
		I_EQUAL(all_het_i, 28);
		int cla_i = output.annotationIndexByName("classification", true, true);
		I_EQUAL(cla_i, 29);
		int val_i = output.annotationIndexByName("validated", true, true);
		I_EQUAL(val_i, 30);
		int com_i = output.annotationIndexByName("comment", true, true);
		I_EQUAL(com_i, 31);
		I_EQUAL(output.annotations().count(), 32);
	
		//check annotation content
		I_EQUAL(output.count(), 12);
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
	
			if (i!=11)
			{
				S_EQUAL(output[i].annotations()[cla_i], QByteArray(""));
			}
			else
			{
				S_EQUAL(output[i].annotations()[cla_i], QByteArray("1"));
			}
			S_EQUAL(output[i].annotations()[val_i], QByteArray("n/a"));
			S_EQUAL(output[i].annotations()[com_i], QByteArray("n/a"));
		}
	}

	void psname_given()
	{	
		QString host = Settings::string("ngsd_host");
		if (host=="") SKIP("Test needs access to the NGSD!");

	    bool convertion_ok;
	
		EXECUTE("VariantAnnotateNGSD", "-in " + TESTDATA("data_in/VariantAnnotateNGSD_in1.tsv") + " -out out/VariantAnnotateNGSD_out2.tsv -psname DX131731_01");
	
		VariantList output;
		output.load("out/VariantAnnotateNGSD_out2.tsv");
	
		//check that the new columns are present
		int gpd_g_i = output.annotationIndexByName("GPD_gene", true, true);
		I_EQUAL(gpd_g_i, 22);
		int gpd_v_i = output.annotationIndexByName("GPD_var", true, true);
		I_EQUAL(gpd_v_i, 23);
		int hom_i = output.annotationIndexByName("ihdb_hom", true, true);
		I_EQUAL(hom_i, 24);
		int het_i = output.annotationIndexByName("ihdb_het", true, true);
		I_EQUAL(het_i, 25);
		int wt_i = output.annotationIndexByName("ihdb_wt", true, true);
		I_EQUAL(wt_i, 26);
		int all_hom_i = output.annotationIndexByName("ihdb_allsys_hom", true, true);
		I_EQUAL(all_hom_i, 27);
		int all_het_i = output.annotationIndexByName("ihdb_allsys_het", true, true);
		I_EQUAL(all_het_i, 28);
		int cla_i = output.annotationIndexByName("classification", true, true);
		I_EQUAL(cla_i, 29);
		int val_i = output.annotationIndexByName("validated", true, true);
		I_EQUAL(val_i, 30);
		int com_i = output.annotationIndexByName("comment", true, true);
		I_EQUAL(com_i, 31);
		I_EQUAL(output.annotations().count(), 32);

		//check annotation content
		I_EQUAL(output.count(), 12);
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

			if (i!=11)
			{
				S_EQUAL(output[i].annotations()[cla_i], QByteArray(""));
				S_EQUAL(output[i].annotations()[val_i], QByteArray(""));
				S_EQUAL(output[i].annotations()[com_i], QByteArray(""));
			}
			else
			{
				S_EQUAL(output[i].annotations()[cla_i], QByteArray("1"));
				S_EQUAL(output[i].annotations()[val_i], QByteArray("n/a (2xTP, 0xFP)"));
				IS_TRUE(output[i].annotations()[com_i].startsWith("n/a ("));
			}

		}
	}

	void somatic_mode()
	{
		QString host = Settings::string("ngsd_host");
		if (host=="") SKIP("Test needs access to the NGSD!");

		EXECUTE("VariantAnnotateNGSD", "-in " + TESTDATA("data_in/VariantAnnotateNGSD_in3.tsv") + " -out out/VariantAnnotateNGSD_out3.tsv -mode somatic");

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
	}
	
};


