#include "TestFramework.h"
#include "RepeatLocusList.h"

TEST_CLASS(RepeatLocusList_Test)
{
private:

	TEST_METHOD(base_tests_ExpansionHunter)
	{
		RepeatLocusList res;
		res.load(TESTDATA("data_in/RepeatLocusList_ExpansionHunter.vcf"));

		S_EQUAL(res.callerAsString(), "ExpansionHunter");
		S_EQUAL(res.callerVersion(), "v5.0.0");
		S_EQUAL(res.callingDate().toString(Qt::ISODate), "2024-04-16");
		I_EQUAL(res.count(), 84);
	}


	TEST_METHOD(base_tests_Straglr)
	{
		RepeatLocusList res;
		res.load(TESTDATA("data_in/RepeatLocusList_Straglr.vcf"));

		S_EQUAL(res.callerAsString(), "Straglr");
		S_EQUAL(res.callerVersion(), "V1.5.0");
		S_EQUAL(res.callingDate().toString(Qt::ISODate), "2024-06-12");
		I_EQUAL(res.count(), 30);
	}

	TEST_METHOD(findMatch)
	{
		RepeatLocusList res;
		res.load(TESTDATA("data_in/RepeatLocusList_findMatch.vcf"));

		RepeatLocus rl;
		rl.setRegion(BedLine(Chromosome("chr4"), 3074876, 3074933));
		rl.setUnit("CAG");
		rl.setAllele1("34.4");
		rl.setAllele2("17.2");

		//exact match
		I_EQUAL(res.findMatch(rl, false), 3);
		rl.setAllele1("35");
		I_EQUAL(res.findMatch(rl, false), -1);
		I_EQUAL(res.findMatch(rl, true), 3);
	}
};
