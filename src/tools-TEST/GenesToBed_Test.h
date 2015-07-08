#include "TestFramework.h"
#include "Settings.h"

class GenesToBed_Test
		: public QObject
{
	Q_OBJECT

private slots:
	
	void kgxref()
	{
		QString db_file = Settings::string("kgxref_joined");
		if (db_file=="") QSKIP("Test needs a database file!");
	
		TFW_EXEC("GenesToBed", "-in " + QFINDTESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out1.bed -mode exon -db_ucsc " + db_file);
		TFW::comareFiles("out/GenesToBed_out1.bed", QFINDTESTDATA("data_out/GenesToBed_out1.bed"));
	}
	

	void test_kgxref_gene()
	{
		QString db_file = Settings::string("kgxref_joined");
		if (db_file=="") QSKIP("Test needs a database file!");
	
		TFW_EXEC("GenesToBed", "-in " + QFINDTESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out2.bed -mode gene -db_ucsc " + db_file);
		TFW::comareFiles("out/GenesToBed_out2.bed", QFINDTESTDATA("data_out/GenesToBed_out2.bed"));
	}
	

	void test_ccds()
	{
		QString db_file = Settings::string("ccds_joined");
		if (db_file=="") QSKIP("Test needs a database file!");
	
		TFW_EXEC("GenesToBed", "-in " + QFINDTESTDATA("data_in/GenesToBed_in2.txt") + " -out out/GenesToBed_out3.bed -mode ccds -db_ccds " + db_file);
		TFW::comareFiles("out/GenesToBed_out3.bed", QFINDTESTDATA("data_out/GenesToBed_out3.bed"));
	}
	

	void test_kgxref_exon2()
	{
		QString db_file = Settings::string("kgxref_joined");
		if (db_file=="") QSKIP("Test needs a database file!");
	
		TFW_EXEC("GenesToBed", "-in " + QFINDTESTDATA("data_in/GenesToBed_in3.txt") + " -out out/GenesToBed_out4.bed -mode exon -db_ucsc " + db_file);
		TFW::comareFiles("out/GenesToBed_out4.bed", QFINDTESTDATA("data_out/GenesToBed_out4.bed"));
	}
	
	void test_kgxref_splice()
	{
		QString db_file = Settings::string("kgxref_joined");
		if (db_file=="") QSKIP("Test needs a database file!");
	
		TFW_EXEC("GenesToBed", "-in " + QFINDTESTDATA("data_in/GenesToBed_in1.txt") + " -out out/GenesToBed_out5.bed -mode splice -db_ucsc " + db_file);
	    TFW::comareFiles("out/GenesToBed_out2.bed", QFINDTESTDATA("data_out/GenesToBed_out2.bed"));
	}

};

TFW_DECLARE(GenesToBed_Test)


