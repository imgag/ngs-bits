#include "TestFramework.h"
#include "BamReader.h"
#include "BasicStatistics.h"
#include "Settings.h"


TEST_CLASS(BamWriter_Test)
{
Q_OBJECT
private slots:

/******************* Bam Writing *******************/

	void BamWriter_BamToBam()
	{
		BamReader reader(TESTDATA("data_in/bamTest.bam"));
	}


/******************* Cram Writing *******************/

#ifndef _WIN32



#endif

};
