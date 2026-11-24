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

		//Test first repeat
		const RepeatLocus& rl = res[0];
		S_EQUAL(rl.allele1(), "12.6");
		S_EQUAL(rl.allele2(), "12.6");
		S_EQUAL(rl.alleles(), "12.6/12.6");
		S_EQUAL(rl.confidenceIntervals(), "7.0-16.7/7.0-16.7");
		S_EQUAL(rl.coverage(), "87");
		IS_TRUE(rl.filters().isEmpty());
		S_EQUAL(rl.geneSymbol(), "GLS");
		S_EQUAL(rl.name(), "GLS");
		S_EQUAL(rl.overlappingInsertions().join(","), "37.6 (het)");
		I_EQUAL(rl.refSize(), 7);
	}

	TEST_METHOD(base_tests_trgt)
	{
		RepeatLocusList res;
		res.load(TESTDATA("data_in/RepeatLocusList_trgt.vcf"));

		S_EQUAL(res.callerAsString(), "trgt");
		S_EQUAL(res.callerVersion(), "V5.1.0-ec66463");
		S_EQUAL(res.callingDate().toString(Qt::ISODate), "2026-09-24");
		I_EQUAL(res.count(), 9);

		//Test first repeat
		RepeatLocus rl = res[0];
		S_EQUAL(rl.allele1(), "2.0");
		S_EQUAL(rl.allele2(), "2.0");
		S_EQUAL(rl.alleles(), "2.0/2.0");
		S_EQUAL(rl.confidenceIntervals(), "2.0-2.0/2.0-2.0");
		S_EQUAL(rl.coverage(), "");
		IS_TRUE(rl.filters().isEmpty());
		S_EQUAL(rl.geneSymbol(), "VWA1");
		S_EQUAL(rl.name(), "VWA1");
		S_EQUAL(rl.unit(), "GGCGCGGAGC");

		//Test repeat with CI
		rl = res[5];
		IS_TRUE(rl.region() == BedLine(Chromosome("chr17"), 80147003, 80147139));
		S_EQUAL(rl.allele1(), "6.8");
		S_EQUAL(rl.allele2(), "9.8");
		S_EQUAL(rl.alleles(), "6.8/9.8");
		S_EQUAL(rl.confidenceIntervals(), "6.8-6.8/9.8-9.8");
		S_EQUAL(rl.coverage(), "");
		IS_TRUE(rl.filters().isEmpty());
		S_EQUAL(rl.geneSymbol(), "EIF4A3");
		S_EQUAL(rl.name(), "EIF4A3");
		S_EQUAL(rl.unit(), "CCTCGCTGYGCCGCTGCCGA");

		//Test empty repeat
		rl = res[7];
		IS_TRUE(rl.region() == BedLine(Chromosome("chrX"), 149631723, 149631735));
		S_EQUAL(rl.allele1(), "");
		S_EQUAL(rl.allele2(), "");
		S_EQUAL(rl.alleles(), "");
		S_EQUAL(rl.confidenceIntervals(), "");
		S_EQUAL(rl.coverage(), "");
		IS_TRUE(rl.filters().isEmpty());
		S_EQUAL(rl.geneSymbol(), "TMEM185A");
		S_EQUAL(rl.name(), "TMEM185A_CGCCGT");
		S_EQUAL(rl.unit(), "CGCCGT");

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
