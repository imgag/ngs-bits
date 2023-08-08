#include "TestFramework.h"
#include "GenLabDB.h"

//NOTE: all tests are based on dummy data in GenLab:
//Karl Bioinformatik - DXtest1
//Karla Bioinformatik - DXtest2

TEST_CLASS(GenLabDB_Test)
{
Q_OBJECT
private slots:

	void phenotypes()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab database!");

		GenLabDB db;
		PhenotypeList phenos = db.phenotypes("DXtest1");
		I_EQUAL(phenos.count(), 2);
		IS_TRUE(phenos.containsAccession("HP:0007281"));
		IS_TRUE(phenos.containsAccession("HP:0000750"));

		phenos = db.phenotypes("DXtest2");
		I_EQUAL(phenos.count(), 2);
		IS_TRUE(phenos.containsAccession("HP:0002066"));
		IS_TRUE(phenos.containsAccession("HP:0002070"));
	}

	void orphanet()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab database!");

		GenLabDB db;
		QStringList list = db.orphanet("DXtest1");
		I_EQUAL(list.count(), 1);
		IS_TRUE(list.contains("ORPHA:73223"));

		list = db.orphanet("DXtest2");
		I_EQUAL(list.count(), 1);
		IS_TRUE(list.contains("ORPHA:99"));
	}

	void diagnosis()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab database!");

		GenLabDB db;
		QStringList list = db.diagnosis("DXtest1");
		I_EQUAL(list.count(), 1);
		IS_TRUE(list.contains("F89"));

		list = db.diagnosis("DXtest2");
		I_EQUAL(list.count(), 1);
		IS_TRUE(list.contains("G11.0"));
	}

	void anamnesis()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab database!");

		GenLabDB db;
		QStringList list = db.anamnesis("DXtest1");
		I_EQUAL(list.count(), 1);
		IS_TRUE(list.contains("Entwicklungsverzoegerung"));

		list = db.anamnesis("DXtest2");
		I_EQUAL(list.count(), 1);
		IS_TRUE(list.contains("Ataxie"));
	}

	void tumorFraction()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab database!");

		GenLabDB db;
		I_EQUAL(db.tumorFraction("DXtest1").count(), 1);
		S_EQUAL(db.tumorFraction("DXtest1")[0], "50");
		I_EQUAL(db.tumorFraction("DXtest2").count(), 0);
	}

	void yearOfBirth()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab database!");

		GenLabDB db;
		S_EQUAL(db.yearOfBirth("DXtest1"), "2018");
		S_EQUAL(db.yearOfBirth("DXtest2"), "2001");
	}

	void yearOfOrderEntry()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab database!");

		GenLabDB db;
		S_EQUAL(db.yearOfOrderEntry("DXtest1"), "2022");
		S_EQUAL(db.yearOfOrderEntry("DXtest2"), "2022");
	}

	void diseaseInfo()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

		GenLabDB db;
		QPair<QString, QString> info = db.diseaseInfo("DXtest1");
		S_EQUAL(info.first, "Mental, behavioural or neurodevelopmental disorders");
		S_EQUAL(info.second, "Affected");

		info = db.diseaseInfo("DXtest2");
		S_EQUAL(info.first, "Diseases of the nervous system");
		S_EQUAL(info.second, "Affected");
	}

	void sapID()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

		GenLabDB db;
		S_EQUAL(db.sapID("DXtest1"), ""); //not in SAP, we can only test that the method call works...
		S_EQUAL(db.sapID("DXtest2"), ""); //not in SAP, we can only test that the method call works...
	}

	void relatives()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

		GenLabDB db;
		QList<SampleRelation> relations = db.relatives("DXtest1");
		I_EQUAL(relations.count(), 1);
		S_EQUAL(relations[0].relation, "siblings");
		S_EQUAL(relations[0].sample1, "DXtest2");
		S_EQUAL(relations[0].sample2, "DXtest1");

		relations = db.relatives("DXtest2");
		I_EQUAL(relations.count(), 1);
		S_EQUAL(relations[0].relation, "siblings");
		S_EQUAL(relations[0].sample1, "DXtest1");
		S_EQUAL(relations[0].sample2, "DXtest2");
	}

	void gender()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

		GenLabDB db;
		S_EQUAL(db.gender("DXtest1"), "male");
		S_EQUAL(db.gender("DXtest2"), "female");
	}

	void patientIdentifier()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

		GenLabDB db;
		S_EQUAL(db.patientIdentifier("DXtest1"), "179158");
		S_EQUAL(db.patientIdentifier("DXtest2"), "179159");
	}

	void studies()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

		GenLabDB db;
		I_EQUAL(db.studies("DXtest1").count(), 2);
		S_EQUAL(db.studies("DXtest1")[0], "DISCO-TWIN");
		S_EQUAL(db.studies("DXtest1")[1], "Genome+");
		I_EQUAL(db.studies("DXtest2").count(), 0);
	}

	void tissue()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab database!");

		GenLabDB db;
		S_EQUAL(db.tissue("DXtest1"), ""); //is 'DNA' but this cannot be converted to tissue
		S_EQUAL(db.tissue("DXtest2"), ""); //not set
	}
};

