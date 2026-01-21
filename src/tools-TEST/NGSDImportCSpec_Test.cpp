#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportCSpec_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.getQuery().exec("INSERT INTO `gene` (`id`, `hgnc_id`, `symbol`, `name`, `type`, `ensembl_id`, `ncbi_id`) VALUES (669634, 9588, 'PTEN', 'phosphatase and tensin homolog', 'protein-coding gene', 'ENSG00000171862', 5728)");
		db.getQuery().exec("INSERT INTO `gene` (`id`, `hgnc_id`, `symbol`, `name`, `type`, `ensembl_id`, `ncbi_id`) VALUES (664242, 7577, 'MYH7', 'myosin heavy chain 7', 'protein-coding gene', 'ENSG00000092054', 4625)");

		//test
		EXECUTE("NGSDImportCSpec", "-test -in " + TESTDATA("data_in/NGSDImportCSpec_in1.json"));

		//check genes
		QStringList genes = db.getValues("SELECT gene FROM cspec_data");
		I_EQUAL(genes.count(), 2);
		IS_TRUE(genes.contains("PTEN"));
		IS_TRUE(genes.contains("MYH7"));

		//check imported version
		QString version = db.getValue("SELECT version FROM db_import_info WHERE name='CSpec'").toString();
		S_EQUAL(version, "2026-01-17");
	}
};


