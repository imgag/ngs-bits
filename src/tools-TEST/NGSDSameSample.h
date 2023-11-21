#include "TestFramework.h"
#include "Helper.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSDSameSample_Test)
{
Q_OBJECT
private slots:
	
	void same_sample()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSameSample_init.sql"));

		//test
		EXECUTE("NGSDSameSample", "-test -mode SAME_SAMPLE -ps NA12880_01 -out out/NGSDSameSample_out1.tsv");
		COMPARE_FILES("out/NGSDSameSample_out1.tsv", TESTDATA("data_out/NGSDSameSample_out1.tsv"));
	}

	void same_patient()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSameSample_init.sql"));

		//test
		EXECUTE("NGSDSameSample", "-test -mode SAME_PATIENT -ps NA12880_01 -out out/NGSDSameSample_out2.tsv");
		COMPARE_FILES("out/NGSDSameSample_out2.tsv", TESTDATA("data_out/NGSDSameSample_out2.tsv"));
	}

	void dna_only()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSameSample_init.sql"));

		//test
		EXECUTE("NGSDSameSample", "-test -sample_type DNA -ps NA12880_01 -out out/NGSDSameSample_out3.tsv");
		COMPARE_FILES("out/NGSDSameSample_out3.tsv", TESTDATA("data_out/NGSDSameSample_out3.tsv"));
	}

	void only_wgs()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSameSample_init.sql"));

		//test
		EXECUTE("NGSDSameSample", "-test -system_type WGS,lrGS -ps NA12880_01 -out out/NGSDSameSample_out4.tsv");
		COMPARE_FILES("out/NGSDSameSample_out4.tsv", TESTDATA("data_out/NGSDSameSample_out4.tsv"));
	}

	void only_nanopore()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSameSample_init.sql"));

		//test
		EXECUTE("NGSDSameSample", "-test -system SQK-114 -ps NA12880_01 -out out/NGSDSameSample_out5.tsv");
		COMPARE_FILES("out/NGSDSameSample_out5.tsv", TESTDATA("data_out/NGSDSameSample_out5.tsv"));
	}

	void include_merged()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDSameSample_init.sql"));

		//test
		EXECUTE("NGSDSameSample", "-test -include_merged -system_type WGS,lrGS -ps NA12880_01 -out out/NGSDSameSample_out6.tsv");
		COMPARE_FILES("out/NGSDSameSample_out6.tsv", TESTDATA("data_out/NGSDSameSample_out6.tsv"));
	}


};
