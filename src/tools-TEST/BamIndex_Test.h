#include "TestFramework.h"

TEST_CLASS(BamIndex_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
#ifdef WIN32
		SKIP("Creating indices does not work (only Windows, mingw 4.91)");
#endif

	    QFile::copy(TESTDATA("../cppNGS-TEST/data_in/panel.bam"), "out/BamIndex.bam");
	    EXECUTE("BamIndex", "-in out/BamIndex.bam");
		IS_TRUE(QFile::exists("out/BamIndex.bam.bai"));
	}
};
