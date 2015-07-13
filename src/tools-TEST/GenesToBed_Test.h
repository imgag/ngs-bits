#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(GenesToBed_Test)
{
Q_OBJECT
private slots:
	
	void kgxref()
	{
		QString db_file = Settings::string("kgxref_joined");
		if (db_file=="") SKIP("Test needs a database file!");
	
		EXECUTE("GenesToBed", "-in " + TESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out1.bed -mode exon -db_ucsc " + db_file);
		COMPARE_FILES("out/GenesToBed_out1.bed", TESTDATA("data_out/GenesToBed_out1.bed"));
	}
	

	void test_kgxref_gene()
	{
		QString db_file = Settings::string("kgxref_joined");
		if (db_file=="") SKIP("Test needs a database file!");
	
		EXECUTE("GenesToBed", "-in " + TESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out2.bed -mode gene -db_ucsc " + db_file);
		COMPARE_FILES("out/GenesToBed_out2.bed", TESTDATA("data_out/GenesToBed_out2.bed"));
	}
	

	void test_ccds()
	{
		QString db_file = Settings::string("ccds_joined");
		if (db_file=="") SKIP("Test needs a database file!");
	
		EXECUTE("GenesToBed", "-in " + TESTDATA("data_in/GenesToBed_in2.txt") + " -out out/GenesToBed_out3.bed -mode ccds -db_ccds " + db_file);
		COMPARE_FILES("out/GenesToBed_out3.bed", TESTDATA("data_out/GenesToBed_out3.bed"));
	}
	

	void test_kgxref_exon2()
	{
		QString db_file = Settings::string("kgxref_joined");
		if (db_file=="") SKIP("Test needs a database file!");
	
		EXECUTE("GenesToBed", "-in " + TESTDATA("data_in/GenesToBed_in3.txt") + " -out out/GenesToBed_out4.bed -mode exon -db_ucsc " + db_file);
		COMPARE_FILES("out/GenesToBed_out4.bed", TESTDATA("data_out/GenesToBed_out4.bed"));
	}
	
	void test_kgxref_splice()
	{
		QString db_file = Settings::string("kgxref_joined");
		if (db_file=="") SKIP("Test needs a database file!");
	
		EXECUTE("GenesToBed", "-in " + TESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out5.bed -mode splice -db_ucsc " + db_file);
	    COMPARE_FILES("out/GenesToBed_out2.bed", TESTDATA("data_out/GenesToBed_out2.bed"));
	}

};

