#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportHGNC_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportHGNC_init.sql"));

		//test
		EXECUTE("NGSDImportHGNC", "-test -in " + TESTDATA("data_in/NGSDImportHGNC_in1.txt") + " -ensembl " + TESTDATA("data_in/NGSDImportEnsembl_in.gff3"));

		//check counts
		int gene_count = db.getValue("SELECT count(*) FROM gene").toInt();
		I_EQUAL(gene_count, 8)
		int alias_count = db.getValue("SELECT count(*) FROM gene_alias").toInt();
		I_EQUAL(alias_count, 39)

		//check TP53
		int gene_id = db.getValue("SELECT id FROM gene WHERE symbol='TP53'").toInt();
		GeneSet previous = db.previousSymbols(gene_id);
		I_EQUAL(previous.count(), 0)
		GeneSet alias = db.synonymousSymbols(gene_id);
		I_EQUAL(alias.count(), 2)
		S_EQUAL(alias[0], "LFS1")
		S_EQUAL(alias[1], "P53")
		GeneInfo gene_info = db.geneInfo("TP53");
		S_EQUAL(gene_info.name, "tumor protein p53")
		S_EQUAL(gene_info.hgnc_id, "HGNC:11998")
		QByteArray type = db.getValue("SELECT type FROM gene WHERE symbol='TP53'").toByteArray();
		S_EQUAL(type, "protein-coding gene")

		//check CA8
		gene_id = db.getValue("SELECT id FROM gene WHERE symbol='CA8'").toInt();
		previous = db.previousSymbols(gene_id);
		I_EQUAL(previous.count(), 1)
		S_EQUAL(previous[0], "CALS")
		alias = db.synonymousSymbols(gene_id);
		I_EQUAL(alias.count(), 1)
		S_EQUAL(alias[0], "CARP")

		//check update of geneinfo_germline relation: only BRCA1 and BRCA2 are left over (see NGSDImportHGNC_init.sql for input data)
		QStringList geneinfo = db.getValues("SELECT symbol FROM geneinfo_germline");
		I_EQUAL(geneinfo.count(), 2);
		S_EQUAL(geneinfo[0], "BRCA1");
		S_EQUAL(geneinfo[1], "BRCA2");

		QStringList somatic_gene_roles = db.getValues("SELECT symbol FROM somatic_gene_role");
		I_EQUAL(somatic_gene_roles.count(), 2);
		S_EQUAL(somatic_gene_roles[0], "BRCA1");
		S_EQUAL(somatic_gene_roles[1], "BRCA2");

		SqlQuery query = db.getQuery();
		query.exec("SELECT CONCAT(spg.symbol, '\t', sp.name) FROM somatic_pathway_gene spg, somatic_pathway sp WHERE sp.id=spg.pathway_id");
		I_EQUAL( query.size(), 3 );
		query.next();
		S_EQUAL(query.value(0).toString(), "BRCA1\tDNA Damage Repair");
		query.next();
		S_EQUAL(query.value(0).toString(), "BRCA2\tDNA Damage Repair");
		query.next();
		S_EQUAL(query.value(0).toString(), "BRCA2\talternative pathway");

		//expression data
		query.exec("SELECT symbol, processed_sample_id, tpm FROM expression");
		I_EQUAL( query.size(), 3 );
		query.next();
		S_EQUAL(query.value(0).toString(), "BRCA1");
		I_EQUAL(query.value(1).toInt(), 3999);
		F_EQUAL2(query.value(2).toFloat(), 8.765, 0.001);
		query.next();
		S_EQUAL(query.value(0).toString(), "BRCA2");
		I_EQUAL(query.value(1).toInt(), 3999);
		F_EQUAL2(query.value(2).toFloat(), 2.3456, 0.0001);
		query.next();
		S_EQUAL(query.value(0).toString(), "BRCA2");
		I_EQUAL(query.value(1).toInt(), 4000);
		F_EQUAL2(query.value(2).toFloat(), 1.23456, 0.00001);

	}

};

