#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportGenlab_Test)
{
Q_OBJECT
private slots:
	
        void general_import()
	{
            if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

            NGSD db(true);
            db.init();
            db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGenlab_init.sql"));

            //test
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest01 -rna_tissue_mapping ");
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest02 -rna_tissue_mapping ");
	}

};


