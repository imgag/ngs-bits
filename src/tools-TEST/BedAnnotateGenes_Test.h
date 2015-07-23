#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(BedAnnotateGenes_Test)
{
Q_OBJECT
private slots:

	//with default parameters, with 3 columns input
	void test_01()
	{
		QString db_file = Settings::string("ccds");
		if (db_file=="") SKIP("Test needs a database file!");

		EXECUTE("BedAnnotateGenes", "-in " + TESTDATA("data_in/BedAnnotateGenes_in1.bed") + " -out out/BedAnnotateGenes_out1.bed -db " + db_file);
		COMPARE_FILES("out/BedAnnotateGenes_out1.bed", TESTDATA("data_out/BedAnnotateGenes_out1.bed"));
	}

	//extended by 25 bases, with 5 columns input
	void test_02()
	{
		QString db_file = Settings::string("ccds");
		if (db_file=="") SKIP("Test needs a database file!");

		EXECUTE("BedAnnotateGenes", "-in " + TESTDATA("data_in/BedAnnotateGenes_in2.bed") + " -out out/BedAnnotateGenes_out2.bed -extend 25 -db " + db_file);
		COMPARE_FILES("out/BedAnnotateGenes_out2.bed", TESTDATA("data_out/BedAnnotateGenes_out2.bed"));
	}
};
