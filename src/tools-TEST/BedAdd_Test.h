#include "TestFramework.h"

TEST_CLASS(BedAdd_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		EXECUTE("BedAdd", "-in " + TESTDATA("data_in/BedAdd_in1.bed") + " " + TESTDATA("data_in/BedAdd_in2.bed") + " -out out/BedAdd_out1.bed");
		COMPARE_FILES("out/BedAdd_out1.bed", TESTDATA("data_out/BedAdd_out1.bed"));
	}

};
