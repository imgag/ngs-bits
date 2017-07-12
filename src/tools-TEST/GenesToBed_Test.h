#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(GenesToBed_Test)
{
Q_OBJECT
private slots:
	
	void ensembl_gene()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToBed_init.sql"));

		//test
		EXECUTE("GenesToBed", "-test -in " + TESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out1.bed -source ensembl -mode gene");
		COMPARE_FILES("out/GenesToBed_out1.bed", TESTDATA("data_out/GenesToBed_out1.bed"));
	}

	void ensembl_exon()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToBed_init.sql"));

		//test ensembl exon
		EXECUTE("GenesToBed", "-test -in " + TESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out2.bed -source ensembl -mode exon");
		COMPARE_FILES("out/GenesToBed_out2.bed", TESTDATA("data_out/GenesToBed_out2.bed"));
	}

	void ccds_gene_annotated()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToBed_init.sql"));

		//test
		EXECUTE("GenesToBed", "-test -in " + TESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out3.bed -source ccds -mode gene -anno");
		COMPARE_FILES("out/GenesToBed_out3.bed", TESTDATA("data_out/GenesToBed_out3.bed"));
	}

	void ccds_exon_annotated()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/GenesToBed_init.sql"));

		//test
		EXECUTE("GenesToBed", "-test -in " + TESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out4.bed -source ccds -mode exon -anno");
		COMPARE_FILES("out/GenesToBed_out4.bed", TESTDATA("data_out/GenesToBed_out4.bed"));
	}

};

