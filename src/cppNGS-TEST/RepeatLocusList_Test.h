#include "TestFramework.h"
#include "RepeatLocusList.h"

TEST_CLASS(RepeatLocusList_Test)
{
Q_OBJECT
private slots:

	void base_tests_ExpansionHunter()
	{
		RepeatLocusList res;
		res.load(TESTDATA("data_in/RepeatLocusList_ExpansionHunter.vcf"));

		S_EQUAL(res.callerAsString(), "ExpansionHunter");
		S_EQUAL(res.callerVersion(), "v5.0.0");
		S_EQUAL(res.callingDate().toString(Qt::ISODate), "2024-04-16");
		I_EQUAL(res.count(), 84);
	}


	void base_tests_Straglr()
	{
		RepeatLocusList res;
		res.load(TESTDATA("data_in/RepeatLocusList_Straglr.vcf"));

		S_EQUAL(res.callerAsString(), "Straglr");
		S_EQUAL(res.callerVersion(), "V1.5.0");
		S_EQUAL(res.callingDate().toString(Qt::ISODate), "2024-06-12");
		I_EQUAL(res.count(), 30);
	}
};
