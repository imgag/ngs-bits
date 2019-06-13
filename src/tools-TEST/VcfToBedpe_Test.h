#include "TestFramework.h"
#include "VcfToBedpe.h"
#include "VcfToBedpe.cpp"
#include <qDebug>

TEST_CLASS(VcfToBedpe_Test)
{
Q_OBJECT
private slots:

	void test_01()
	{
		VcfToBedpe test_delly("D:\\test_files\\DX192435_02_delly_var_structural.vcf.gz");
		test_delly.writeFile("D:\\test_files\\test_out_delly.bedpe");

		VcfToBedpe test_manta("D:\\test_files\\DX192435_02_manta_var_structural.vcf.gz");
		test_manta.writeFile("D:\\test_files\\test_out_manta.bedpe");

		exit(0);
	}

};
