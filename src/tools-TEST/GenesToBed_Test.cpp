#include "TestFrameworkNGS.h"
#include "NGSD.h"

TEST_CLASS(GenesToBed_Test)
{
private:
	
	TEST_METHOD(ensembl_gene)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToBed_init.sql"));

		//test
		EXECUTE("GenesToBed", "-test -in " + TESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out1.bed -source ensembl -mode gene");
		COMPARE_FILES("out/GenesToBed_out1.bed", TESTDATA("data_out/GenesToBed_out1.bed"));
	}

	TEST_METHOD(ensembl_exon)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToBed_init.sql"));

		//test ensembl exon
		EXECUTE("GenesToBed", "-test -in " + TESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out2.bed -source ensembl -mode exon");
		COMPARE_FILES("out/GenesToBed_out2.bed", TESTDATA("data_out/GenesToBed_out2.bed"));
	}

	TEST_METHOD(ccds_gene_annotated)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToBed_init.sql"));

		//test
		EXECUTE("GenesToBed", "-test -in " + TESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out3.bed -source ccds -mode gene -anno");
		COMPARE_FILES("out/GenesToBed_out3.bed", TESTDATA("data_out/GenesToBed_out3.bed"));
	}

	TEST_METHOD(ccds_exon_annotated)
	{
		SKIP_IF_NO_TEST_NGSD();

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToBed_init.sql"));

		//test
		EXECUTE("GenesToBed", "-test -in " + TESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out4.bed -source ccds -mode exon -anno");
		COMPARE_FILES("out/GenesToBed_out4.bed", TESTDATA("data_out/GenesToBed_out4.bed"));
	}

};

