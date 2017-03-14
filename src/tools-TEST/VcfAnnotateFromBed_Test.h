#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(VcfAnnotateFromBed_Test)
{
Q_OBJECT
private slots:
	
	void test_01()
	{
		EXECUTE("VcfAnnotateFromBed", "-in " + TESTDATA("data_in/VcfAnnotateFromBed_in1.vcf") + " -out out/VcfAnnotateFromBed_out1.vcf -name OMIM -bed " + TESTDATA("data_in/VcfAnnotateFromBed_in1.bed") );
		REMOVE_LINES("out/VcfAnnotateFromBed_out1.vcf", QRegExp("##INFO=<ID=OMIM"))
		COMPARE_FILES("out/VcfAnnotateFromBed_out1.vcf", TESTDATA("data_out/VcfAnnotateFromBed_out1.vcf"));
	}

};

