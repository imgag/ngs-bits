#include "TestFramework.h"
#include "FastqFileStream.h"

TEST_CLASS(BamToFastq_Test)
{
Q_OBJECT
private slots:

	void test_01()
	{
		EXECUTE("BamToFastq", "-in " + TESTDATA("data_in/BamToFastq_in1.bam") + " -out1 out/BamToFastq_out1.fastq.gz -out2 out/BamToFastq_out2.fastq.gz");
		IS_TRUE(QFile::exists("out/BamToFastq_out1.fastq.gz"));
		IS_TRUE(QFile::exists("out/BamToFastq_out2.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out1.fastq.gz", TESTDATA("data_out/BamToFastq_out1.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out2.fastq.gz", TESTDATA("data_out/BamToFastq_out2.fastq.gz"));
	}

	void test_remove_duplicates()
	{
		EXECUTE("BamToFastq", "-in " + TESTDATA("data_in/BamToFastq_in1.bam") + " -remove_duplicates -out1 out/BamToFastq_out3.fastq.gz -out2 out/BamToFastq_out4.fastq.gz");
		IS_TRUE(QFile::exists("out/BamToFastq_out3.fastq.gz"));
		IS_TRUE(QFile::exists("out/BamToFastq_out4.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out3.fastq.gz", TESTDATA("data_out/BamToFastq_out3.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out4.fastq.gz", TESTDATA("data_out/BamToFastq_out4.fastq.gz"));
	}
};


