#include "../TestFramework.h"
#include "VariantList.h"
#include "Settings.h"

class VariantAnnotateNGSD_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void test_01()
	{
		QString host = Settings::string("ngsd_host");
		if (host=="") QSKIP("Test needs access to the NGSD!");

		bool convertion_ok;
	
		TFW_EXEC("VariantAnnotateNGSD", "-in " + QFINDTESTDATA("data_in/VariantAnnotateNGSD_in1.tsv") + " -out out/VariantAnnotateNGSD_out1.tsv");
	
		VariantList output;
		output.load("out/VariantAnnotateNGSD_out1.tsv");
	
		//check that the new columns are present
		int hom_i = output.annotationIndexByName("ihdb_hom_", false, true);
		QCOMPARE(hom_i, 22);
		int het_i = output.annotationIndexByName("ihdb_het_", false, true);
		QCOMPARE(het_i, 23);
		int wt_i = output.annotationIndexByName("ihdb_wt_", false, true);
		QCOMPARE(wt_i, 24);
		int all_hom_i = output.annotationIndexByName("ihdb_allsys_hom", true, true);
		QCOMPARE(all_hom_i, 25);
		int all_het_i = output.annotationIndexByName("ihdb_allsys_het", true, true);
		QCOMPARE(all_het_i, 26);
		int cla_i = output.annotationIndexByName("classification", true, true);
		QCOMPARE(cla_i, 27);
		int val_i = output.annotationIndexByName("validated", true, true);
		QCOMPARE(val_i, 28);
		QCOMPARE(output.annotations().count(), 29);
	
		//check that 'comment' column is missing
		QCOMPARE(output.annotationIndexByName("comment", true, false), -1);
	
		//check annotation content
		QCOMPARE(output.count(), 11);
		for (int i=0; i<output.count(); ++i)
		{
			QCOMPARE(output[i].annotations()[hom_i], QString("n/a"));
			QCOMPARE(output[i].annotations()[het_i], QString("n/a"));
			QCOMPARE(output[i].annotations()[wt_i],  QString("n/a"));
	
			convertion_ok = false;
			int value_int = output[i].annotations()[all_hom_i].toInt(&convertion_ok);
			QVERIFY(convertion_ok);
			QVERIFY(value_int >= 0);
	
			convertion_ok = false;
			value_int = output[i].annotations()[all_het_i].toInt(&convertion_ok);
			QVERIFY(convertion_ok);
			QVERIFY(value_int >= 0);
	
			QCOMPARE(output[i].annotations()[cla_i], QString("n/a"));
			QCOMPARE(output[i].annotations()[val_i], QString("n/a"));
		}
	}

	void test_02()
	{	
		QString host = Settings::string("ngsd_host");
		if (host=="") QSKIP("Test needs access to the NGSD!");

	    bool convertion_ok;
	
		TFW_EXEC("VariantAnnotateNGSD", "-in " + QFINDTESTDATA("data_in/VariantAnnotateNGSD_in1.tsv") + " -out out/VariantAnnotateNGSD_out2.tsv -psname DX131731_01");
	
		VariantList output;
		output.load("out/VariantAnnotateNGSD_out2.tsv");
	
		//check that the new columns are present
		int hom_i = output.annotationIndexByName("ihdb_hom_", false, true);
		QCOMPARE(hom_i, 22);
		int het_i = output.annotationIndexByName("ihdb_het_", false, true);
		QCOMPARE(het_i, 23);
		int wt_i = output.annotationIndexByName("ihdb_wt_", false, true);
		QCOMPARE(wt_i, 24);
		int all_hom_i = output.annotationIndexByName("ihdb_allsys_hom", true, true);
		QCOMPARE(all_hom_i, 25);
		int all_het_i = output.annotationIndexByName("ihdb_allsys_het", true, true);
		QCOMPARE(all_het_i, 26);
		int cla_i = output.annotationIndexByName("classification", true, true);
		QCOMPARE(cla_i, 27);
		int val_i = output.annotationIndexByName("validated", true, true);
		QCOMPARE(val_i, 28);
		QCOMPARE(output.annotations().count(), 29);
	
		//check that 'comment' column is missing
		QCOMPARE(output.annotationIndexByName("comment", true, false), -1);
	
		//check annotation content
		QCOMPARE(output.count(), 11);
		for (int i=0; i<output.count(); ++i)
		{
			double value_double = output[i].annotations()[hom_i].toDouble(&convertion_ok);
			QVERIFY(convertion_ok);
			QVERIFY(value_double >= 0.0 && value_double <= 1.0);
	
			value_double = output[i].annotations()[hom_i].toDouble(&convertion_ok);
			QVERIFY(convertion_ok);
			QVERIFY(value_double >= 0.0 && value_double <= 1.0);
	
			value_double = output[i].annotations()[het_i].toDouble(&convertion_ok);
			QVERIFY(convertion_ok);
			QVERIFY(value_double >= 0.0 && value_double <= 1.0);
	
			value_double = output[i].annotations()[wt_i].toDouble(&convertion_ok);
			QVERIFY(convertion_ok);
			QVERIFY(value_double >= 0.0 && value_double <= 1.0);
	
			convertion_ok = false;
			int value_int = output[i].annotations()[all_hom_i].toInt(&convertion_ok);
			QVERIFY(convertion_ok);
			QVERIFY(value_int >= 0);
	
			convertion_ok = false;
			value_int = output[i].annotations()[all_het_i].toInt(&convertion_ok);
			QVERIFY(convertion_ok);
			QVERIFY(value_int >= 0);
	
			QCOMPARE(output[i].annotations()[cla_i], QString("n/a"));
			QCOMPARE(output[i].annotations()[val_i], QString("n/a"));
		}
	}

	void test_03()
	{
		QString host = Settings::string("ngsd_host");
		if (host=="") QSKIP("Test needs access to the NGSD!");

		TFW_EXEC("VariantAnnotateNGSD", "-in " + QFINDTESTDATA("data_in/VariantAnnotateNGSD_in3.tsv") + " -out out/VariantAnnotateNGSD_out3.tsv -mode somatic");

		VariantList output;
		output.load("out/VariantAnnotateNGSD_out3.tsv");

		//check that the new columns are present
		int val_i1 = output.annotationIndexByName("som_ihdb_c", true, false);
		QCOMPARE(val_i1, 22);
		int val_i2 = output.annotationIndexByName("som_ihdb_p", true, false);
		QCOMPARE(val_i2, 23);
		QCOMPARE(output.annotations().count(), 24);

		//check annotation content
		QCOMPARE(output.count(), 5);
		for (int i=0; i<output.count(); ++i)
		{
			if (output[i].chr().str()=="chr1")//known somatic variants
			{
				bool ok = false;
				QVERIFY(output[i].annotations()[val_i1].toInt(&ok)>=1);
				QVERIFY(ok);
				QVERIFY(output[i].annotations()[val_i2]!="");
			}
			else //unknown somatic variants
			{
				QCOMPARE(output[i].annotations()[val_i1], QString("0"));
				QCOMPARE(output[i].annotations()[val_i2], QString(""));
			}
		}
	}
	
};

TFW_DECLARE(VariantAnnotateNGSD_Test)

