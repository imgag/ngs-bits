#include "TestFramework.h"

TEST_CLASS(SampleSimilarity_Test)
{
Q_OBJECT
private slots:
	
	void test_gsvar_multisample()
	{
		EXECUTE("SampleSimilarity", "-in " + TESTDATA("data_in/SampleSimilarity_in1.GSvar") + " " + TESTDATA("data_in/SampleSimilarity_in2.GSvar") + " " + TESTDATA("data_in/SampleSimilarity_in3.GSvar") + " -out out/SampleSimilarity_out1.tsv -include_gonosomes -mode gsvar");
		COMPARE_FILES("out/SampleSimilarity_out1.tsv", TESTDATA("data_out/SampleSimilarity_out1.tsv"));
	}

	void test_bam()
	{
		EXECUTE("SampleSimilarity", "-in " + TESTDATA("data_in/SampleSimilarity_in4.bam") + " " + TESTDATA("data_in/SampleSimilarity_in5.bam") + " -out out/SampleSimilarity_out2.tsv -mode bam -max_snps 200");
		COMPARE_FILES("out/SampleSimilarity_out2.tsv", TESTDATA("data_out/SampleSimilarity_out2.tsv"));
	}

	void test_bam_roi()
	{
		EXECUTE("SampleSimilarity", "-in " + TESTDATA("data_in/SampleSimilarity_in4.bam") + " " + TESTDATA("data_in/SampleSimilarity_in5.bam") + " -out out/SampleSimilarity_out3.tsv -mode bam -max_snps 100 -roi " + TESTDATA("data_in/SampleSimilarity_roi.bed"));
		COMPARE_FILES("out/SampleSimilarity_out3.tsv", TESTDATA("data_out/SampleSimilarity_out3.tsv"));
	}

	void test_vcf()
	{
		EXECUTE("SampleSimilarity", "-in " + TESTDATA("data_in/SampleSimilarity_in6.vcf.gz") + " " + TESTDATA("data_in/SampleSimilarity_in7.vcf.gz") + " -mode vcf -skip_multi -out out/SampleSimilarity_out4.tsv");
		COMPARE_FILES("out/SampleSimilarity_out4.tsv", TESTDATA("data_out/SampleSimilarity_out4.tsv"));
	}

	void test_vcf_roi_one_input_file()
	{
		QString tmp = Helper::tempFileName("_samples.txt");
		Helper::storeTextFile(tmp, QStringList() << TESTDATA("data_in/SampleSimilarity_in6.vcf.gz") << TESTDATA("data_in/SampleSimilarity_in7.vcf.gz"));
		EXECUTE("SampleSimilarity", "-in " + tmp + " -mode vcf -skip_multi -out out/SampleSimilarity_out5.tsv -roi " + TESTDATA("data_in/SampleSimilarity_roi.bed"));
		COMPARE_FILES("out/SampleSimilarity_out5.tsv", TESTDATA("data_out/SampleSimilarity_out5.tsv"));
	}

};
