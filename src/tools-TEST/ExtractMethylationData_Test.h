#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(ExtractMethylationData_Test)
{
Q_OBJECT
private slots:
	
	void default_parameters()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		EXECUTE("ExtractMethylationData", "-in " + TESTDATA("data_in/ExtractMethylationData_in1.bed.gz") + " -loci " + TESTDATA("data_in/ExtractMethylationData_loci1.bed")
				+ " -out out/ExtractMethylationData_out1.bed");
		COMPARE_FILES("out/ExtractMethylationData_out1.bed", TESTDATA("data_out/ExtractMethylationData_out1.bed"));
	}
	
	void with_type_columns()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		EXECUTE("ExtractMethylationData", "-add_methylation_types -in " + TESTDATA("data_in/ExtractMethylationData_in1.bed.gz") + " -loci " + TESTDATA("data_in/ExtractMethylationData_loci1.bed")
				+ " -out out/ExtractMethylationData_out2.bed");
		COMPARE_FILES("out/ExtractMethylationData_out2.bed", TESTDATA("data_out/ExtractMethylationData_out2.bed"));
	}
};

