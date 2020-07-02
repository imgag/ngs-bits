#include "TestFramework.h"
#include "VcfFileHandler.h"

TEST_CLASS(VcfFileHandler_Test)
{
Q_OBJECT
private slots:

	void loadVCF()
	{
		VcfFileHandler vcfH;
		qDebug() << __FILE__ << __LINE__;
		vcfH.load(TESTDATA("data_in/VcfFileHandler_in.vcf"));
		vcfH.store("out/VcfFileHandler_out.vcf");

		COMPARE_FILES("out/VcfFileHandler_out.vcf", TESTDATA("data_out/VcfFileHandler_out.vcf"));
	}

};
