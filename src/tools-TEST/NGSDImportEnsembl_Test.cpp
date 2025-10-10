#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(NGSDImportEnsembl_Test)
{
private:
	
	TEST_METHOD(default_parameters)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportEnsembl_init.sql"));

		//test
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDImportEnsembl_in.gff3"));

		//check transcripts
		int count = db.getValue("SELECT count(*) FROM gene_transcript").toInt();
		I_EQUAL(count, 14);
		count = db.getValue("SELECT count(*) FROM gene_transcript WHERE source='ensembl'").toInt();
		I_EQUAL(count, 10);
		count = db.getValue("SELECT count(*) FROM gene_transcript WHERE source='ccds'").toInt();
		I_EQUAL(count, 4);
		count = db.getValue("SELECT count(*) FROM gene_transcript WHERE start_coding IS NULL AND end_coding IS NULL").toInt();
		I_EQUAL(count, 3);

		//check transcript biotype
		QString biotype = db.getValue("SELECT biotype FROM gene_transcript WHERE name='ENST00000456328'").toString();
		S_EQUAL(biotype, "processed transcript");
		biotype = db.getValue("SELECT biotype FROM gene_transcript WHERE name='ENST00000306125'").toString();
		S_EQUAL(biotype, "protein coding");

		//check exons
		count = db.getValue("SELECT count(ge.start) FROM gene_exon ge, gene_transcript gt, gene g WHERE g.id=gt.gene_id AND ge.transcript_id=gt.id AND g.symbol='DDX11L1'").toInt();
		I_EQUAL(count, 9);
		count = db.getValue("SELECT count(ge.start) FROM gene_exon ge, gene_transcript gt WHERE ge.transcript_id=gt.id AND gt.name='CCDS9344'").toInt();
		I_EQUAL(count, 26);
	}

    TEST_METHOD(with_pseudogenes)
    {
		SKIP_IF_NO_TEST_NGSD();

        //init
        NGSD db(true);
        db.init();
        db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportEnsembl_init.sql"));

        //test
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDImportEnsembl_in.gff3") + " -pseudogenes " + TESTDATA("data_in/NGSDImportEnsembl_in_pseudogenes.txt"));

        //check pseudogenes
        int n_pseudogenes = db.getValue("SELECT COUNT(*) FROM gene_pseudogene_relation").toInt();
        I_EQUAL(n_pseudogenes, 2);

        int parent_gene_id = db.getValue("SELECT id FROM gene WHERE symbol='ABCD1'").toInt();
        int pseudogene_id = db.getValue("SELECT pseudogene_gene_id FROM gene_pseudogene_relation WHERE parent_gene_id=" + QByteArray::number(parent_gene_id)).toInt();
        QByteArray pseudogene = db.geneSymbol(pseudogene_id);
        S_EQUAL(pseudogene, "ABCD1P2");

        parent_gene_id = db.getValue("SELECT id FROM gene WHERE symbol='AARS1'").toInt();
        QByteArray gene_name = db.getValue("SELECT gene_name FROM gene_pseudogene_relation WHERE parent_gene_id=" + QByteArray::number(parent_gene_id)).toString().toUtf8();
        S_EQUAL(gene_name, "ENSG00000249038;RP11-149A7");

        pseudogene_id = db.getValue("SELECT id FROM gene WHERE symbol='ABCD1P2'").toInt();
        parent_gene_id = db.getValue("SELECT parent_gene_id FROM gene_pseudogene_relation WHERE pseudogene_gene_id=" + QByteArray::number(pseudogene_id)).toInt();
        QByteArray parent_gene = db.geneSymbol(parent_gene_id);
        S_EQUAL(parent_gene, "ABCD1");
    }

	TEST_METHOD(with_pseudogene_duplicates)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportEnsembl_init.sql"));

		//test
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDImportEnsembl_in.gff3") + " -pseudogenes " + TESTDATA("data_in/NGSDImportEnsembl_in_pseudogenes.txt") + " " +  TESTDATA("data_in/NGSDImportEnsembl_in_pseudogenes.txt"));

		//check pseudogenes
		int n_pseudogenes = db.getValue("SELECT COUNT(*) FROM gene_pseudogene_relation").toInt();
		I_EQUAL(n_pseudogenes, 2);

		int parent_gene_id = db.getValue("SELECT id FROM gene WHERE symbol='ABCD1'").toInt();
		int pseudogene_id = db.getValue("SELECT pseudogene_gene_id FROM gene_pseudogene_relation WHERE parent_gene_id=" + QByteArray::number(parent_gene_id)).toInt();
		QByteArray pseudogene = db.geneSymbol(pseudogene_id);
		S_EQUAL(pseudogene, "ABCD1P2");

		parent_gene_id = db.getValue("SELECT id FROM gene WHERE symbol='AARS1'").toInt();
		QByteArray gene_name = db.getValue("SELECT gene_name FROM gene_pseudogene_relation WHERE parent_gene_id=" + QByteArray::number(parent_gene_id)).toString().toUtf8();
		S_EQUAL(gene_name, "ENSG00000249038;RP11-149A7");

		pseudogene_id = db.getValue("SELECT id FROM gene WHERE symbol='ABCD1P2'").toInt();
		parent_gene_id = db.getValue("SELECT parent_gene_id FROM gene_pseudogene_relation WHERE pseudogene_gene_id=" + QByteArray::number(pseudogene_id)).toInt();
		QByteArray parent_gene = db.geneSymbol(parent_gene_id);
		S_EQUAL(parent_gene, "ABCD1");
	}

	TEST_METHOD(with_multiple_pseudogene_files)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportEnsembl_init.sql"));

		//test
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDImportEnsembl_in.gff3") + " -pseudogenes " + TESTDATA("data_in/NGSDImportEnsembl_in_pseudogenes_s1.txt") + " " +  TESTDATA("data_in/NGSDImportEnsembl_in_pseudogenes_s2.txt"));

		//check pseudogenes
		int n_pseudogenes = db.getValue("SELECT COUNT(*) FROM gene_pseudogene_relation").toInt();
		I_EQUAL(n_pseudogenes, 2);

		int parent_gene_id = db.getValue("SELECT id FROM gene WHERE symbol='ABCD1'").toInt();
		int pseudogene_id = db.getValue("SELECT pseudogene_gene_id FROM gene_pseudogene_relation WHERE parent_gene_id=" + QByteArray::number(parent_gene_id)).toInt();
		QByteArray pseudogene = db.geneSymbol(pseudogene_id);
		S_EQUAL(pseudogene, "ABCD1P2");

		parent_gene_id = db.getValue("SELECT id FROM gene WHERE symbol='AARS1'").toInt();
		QByteArray gene_name = db.getValue("SELECT gene_name FROM gene_pseudogene_relation WHERE parent_gene_id=" + QByteArray::number(parent_gene_id)).toString().toUtf8();
		S_EQUAL(gene_name, "ENSG00000249038;RP11-149A7");

		pseudogene_id = db.getValue("SELECT id FROM gene WHERE symbol='ABCD1P2'").toInt();
		parent_gene_id = db.getValue("SELECT parent_gene_id FROM gene_pseudogene_relation WHERE pseudogene_gene_id=" + QByteArray::number(pseudogene_id)).toInt();
		QByteArray parent_gene = db.geneSymbol(parent_gene_id);
		S_EQUAL(parent_gene, "ABCD1");
	}

	TEST_METHOD(with_parameter_all)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportEnsembl_init.sql"));

		//test
		EXECUTE("NGSDImportEnsembl", "-test -in " + TESTDATA("data_in/NGSDImportEnsembl_in.gff3") + " -all");

		//check transcripts
		int count = db.getValue("SELECT count(*) FROM gene_transcript").toInt();
		I_EQUAL(count, 26);
		count = db.getValue("SELECT count(*) FROM gene_transcript WHERE source='ensembl'").toInt();
		I_EQUAL(count, 22);
		count = db.getValue("SELECT count(*) FROM gene_transcript WHERE source='ccds'").toInt();
		I_EQUAL(count, 4);
		count = db.getValue("SELECT count(*) FROM gene_transcript WHERE start_coding IS NULL AND end_coding IS NULL").toInt();
		I_EQUAL(count, 11);

		//check flags
		bool is_gencode_basic = db.getValue("SELECT is_gencode_basic FROM gene_transcript WHERE name='ENST00000456328'").toBool();
		IS_TRUE(is_gencode_basic);
		is_gencode_basic = db.getValue("SELECT is_gencode_basic FROM gene_transcript WHERE name='ENST00000515242'").toBool();
		IS_FALSE(is_gencode_basic);

		bool is_gencode_primary = db.getValue("SELECT is_gencode_primary FROM gene_transcript WHERE name='ENST00000306125'").toBool();
		IS_TRUE(is_gencode_primary);
		is_gencode_primary = db.getValue("SELECT is_gencode_primary FROM gene_transcript WHERE name='ENST00000456328'").toBool();
		IS_FALSE(is_gencode_primary);

		bool is_ensembl_canonical = db.getValue("SELECT is_ensembl_canonical FROM gene_transcript WHERE name='ENST00000450305'").toBool();
		IS_TRUE(is_ensembl_canonical);
		is_ensembl_canonical = db.getValue("SELECT is_ensembl_canonical FROM gene_transcript WHERE name='ENST00000456328'").toBool();
		IS_FALSE(is_ensembl_canonical);

		bool is_mane_select = db.getValue("SELECT is_mane_select FROM gene_transcript WHERE name='ENST00000306125'").toBool();
		IS_TRUE(is_mane_select);
		is_mane_select = db.getValue("SELECT is_mane_select FROM gene_transcript WHERE name='ENST00000456328'").toBool();
		IS_FALSE(is_mane_select);

		bool is_mane_plus_clinical = db.getValue("SELECT is_mane_plus_clinical FROM gene_transcript WHERE name='ENST00000456328'").toBool();
		IS_TRUE(is_mane_plus_clinical);
		is_mane_plus_clinical = db.getValue("SELECT is_mane_plus_clinical FROM gene_transcript WHERE name='ENST00000306125'").toBool();
		IS_FALSE(is_mane_plus_clinical);

		//check exons
		count = db.getValue("SELECT count(ge.start) FROM gene_exon ge, gene_transcript gt, gene g WHERE g.id=gt.gene_id AND ge.transcript_id=gt.id AND g.symbol='DDX11L1'").toInt();
		I_EQUAL(count, 16);
		count = db.getValue("SELECT count(ge.start) FROM gene_exon ge, gene_transcript gt WHERE ge.transcript_id=gt.id AND gt.name='CCDS9344'").toInt();
		I_EQUAL(count, 26);
	}

};


