#include "TestFramework.h"

TEST_CLASS(TrioMendelianErrors_Test)
{
Q_OBJECT
private slots:

	void default_parameters()
	{
		EXECUTE("TrioMendelianErrors", "-vcf " + TESTDATA("data_in/TrioMendelianErrors_in1.vcf.gz") + " -c NA12878x2_80 -f NA12891_14 -m NA12892_18 -out out/TrioMendelianErrors_out1.txt");
		COMPARE_FILES("out/TrioMendelianErrors_out1.txt", TESTDATA("data_out/TrioMendelianErrors_out1.txt"));
	}

	void min_depth()
	{
		EXECUTE("TrioMendelianErrors", "-vcf " + TESTDATA("data_in/TrioMendelianErrors_in1.vcf.gz") + " -c NA12878x2_80 -f NA12891_14 -m NA12892_18 -min_dp 15 -out out/TrioMendelianErrors_out2.txt");
		COMPARE_FILES("out/TrioMendelianErrors_out2.txt", TESTDATA("data_out/TrioMendelianErrors_out2.txt"));
	}
};
