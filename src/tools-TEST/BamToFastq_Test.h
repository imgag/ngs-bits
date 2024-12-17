#include "TestFramework.h"
#include "FastqFileStream.h"
#include "BamReader.h"
#include "BamWriter.h"

TEST_CLASS(BamToFastq_Test)
{
Q_OBJECT
private slots:

	void test_default_parameter()
	{
		EXECUTE("BamToFastq", "-in " + TESTDATA("data_in/BamToFastq_in1.bam") + " -out1 out/BamToFastq_out1.fastq.gz -out2 out/BamToFastq_out2.fastq.gz -write_buffer_size 1");
		IS_TRUE(QFile::exists("out/BamToFastq_out1.fastq.gz"));
		IS_TRUE(QFile::exists("out/BamToFastq_out2.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out1.fastq.gz", TESTDATA("data_out/BamToFastq_out1.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out2.fastq.gz", TESTDATA("data_out/BamToFastq_out2.fastq.gz"));
	}

	void test_fix() //uses data and results from first test, but duplicates the reads
	{
		//create BAM with each read duplicated
		QString bam_temp = Helper::tempFileName(".bam");
		BamWriter writer(bam_temp);
		QString bam = TESTDATA("data_in/BamToFastq_in1.bam");
		for (int i=0; i<2; ++i)
		{
			BamReader reader(bam);
			if (i==0) writer.writeHeader(reader);
			BamAlignment al;
			while(reader.getNextAlignment(al))
			{
				writer.writeAlignment(al);
			}
		}
		writer.close();

		EXECUTE("BamToFastq", "-in " + bam_temp + " -out1 out/BamToFastq_out1_fix.fastq.gz -out2 out/BamToFastq_out2_fix.fastq.gz -write_buffer_size 1 -fix");
		COMPARE_GZ_FILES("out/BamToFastq_out1_fix.fastq.gz", TESTDATA("data_out/BamToFastq_out1.fastq.gz"));
		COMPARE_GZ_FILES("out/BamToFastq_out2_fix.fastq.gz", TESTDATA("data_out/BamToFastq_out2.fastq.gz"));
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
		EXECUTE("BamToFastq", "-in " + TESTDATA("data_in/BamToFastq_in3.bam") + " -out1 out/BamToFastq_out7.fastq.gz -write_buffer_size 1");
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


