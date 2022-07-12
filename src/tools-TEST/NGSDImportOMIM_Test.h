#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportOMIM_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();

		//test
		EXECUTE("NGSDImportOMIM", "-test -gene " + TESTDATA("data_in/NGSDImportOMIM_mim2gene.txt") + " -morbid " + TESTDATA("data_in/NGSDImportOMIM_morbidmap.txt"));

		//check
		I_EQUAL(db.getValue("SELECT count(*) FROM omim_gene").toInt(), 3)
		I_EQUAL(db.getValue("SELECT count(*) FROM omim_phenotype").toInt(), 5)
		I_EQUAL(db.getValue("SELECT count(*) FROM omim_phenotype op, omim_gene og WHERE op.omim_gene_id=og.id AND og.gene='ALDH2'").toInt(), 4)
		I_EQUAL(db.getValue("SELECT count(*) FROM omim_phenotype op, omim_gene og WHERE op.omim_gene_id=og.id AND og.gene='ALDH1B1'").toInt(), 0)
		I_EQUAL(db.getValue("SELECT count(*) FROM omim_phenotype op, omim_gene og WHERE op.omim_gene_id=og.id AND og.gene='ACAT2'").toInt(), 1)
	}
};

