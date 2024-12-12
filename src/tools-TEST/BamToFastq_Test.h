#include "TestFramework.h"
#include "FastqFileStream.h"

TEST_CLASS(BamToFastq_Test)
{
Q_OBJECT
private slots:

	void test_mandatory_parameter()
	{
		EXECUTE("BamToFastq", "-in " + TESTDATA("data_in/BamToFastq_in1.bam") + " -out1 out/BamToFastq_out1.fastq.gz -out2 out/BamToFastq_out2.fastq.gz -write_buffer_size 1");
		IS_TRUE(QFile::exists("out/BamToFastq_out1.fastq.gz"));
		IS_TRUE(QFile::exists("out/BamToFastq_out2.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out1.fastq.gz", TESTDATA("data_out/BamToFastq_out1.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out2.fastq.gz", TESTDATA("data_out/BamToFastq_out2.fastq.gz"));
	}

	void test_remove_duplicates()
	{
		EXECUTE("BamToFastq", "-in " + TESTDATA("data_in/BamToFastq_in1.bam") + " -remove_duplicates -out1 out/BamToFastq_out3.fastq.gz -out2 out/BamToFastq_out4.fastq.gz -write_buffer_size 1");
		IS_TRUE(QFile::exists("out/BamToFastq_out3.fastq.gz"));
		IS_TRUE(QFile::exists("out/BamToFastq_out4.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out3.fastq.gz", TESTDATA("data_out/BamToFastq_out3.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out4.fastq.gz", TESTDATA("data_out/BamToFastq_out4.fastq.gz"));
	}

	void test_reg()
	{
		EXECUTE("BamToFastq", "-in " + TESTDATA("data_in/BamToFastq_in1.bam") + " -reg chr17:7571319-7575084 -out1 out/BamToFastq_out5.fastq.gz -out2 out/BamToFastq_out6.fastq.gz -write_buffer_size 1");
		IS_TRUE(QFile::exists("out/BamToFastq_out5.fastq.gz"));
		IS_TRUE(QFile::exists("out/BamToFastq_out6.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out5.fastq.gz", TESTDATA("data_out/BamToFastq_out5.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out6.fastq.gz", TESTDATA("data_out/BamToFastq_out6.fastq.gz"));
	}

	void single_end()
	{
		EXECUTE("BamToFastq", "-in " + TESTDATA("data_in/BamToFastq_in3.bam") + " -out1 out/BamToFastq_out7.fastq.gz -mode single-end -write_buffer_size 1");
		IS_TRUE(QFile::exists("out/BamToFastq_out7.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out7.fastq.gz", TESTDATA("data_out/BamToFastq_out7.fastq.gz"));
	}

	void extend()
	{
		EXECUTE("BamToFastq", "-in " + TESTDATA("data_in/BamToFastq_in1.bam") + " -extend 151 -out1 out/BamToFastq_out8.fastq.gz -out2 out/BamToFastq_out9.fastq.gz -write_buffer_size 1");
		IS_TRUE(QFile::exists("out/BamToFastq_out8.fastq.gz"));
		IS_TRUE(QFile::exists("out/BamToFastq_out9.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out8.fastq.gz", TESTDATA("data_out/BamToFastq_out8.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out9.fastq.gz", TESTDATA("data_out/BamToFastq_out9.fastq.gz"));
	}
};


