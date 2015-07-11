#include "TestFramework_NGS.h"

TEST_CLASS(SeqPurge_Test)
{
Q_OBJECT
private slots:

	//MiSeq 151 cycles - hpHSPv2 panel - with qc
	void test_01()
	{
		TFW_EXEC("SeqPurge", "-in1 " + QFINDTESTDATA("data_in/SeqPurge_in1.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/SeqPurge_in2.fastq.gz") + " -out1 out/SeqPurge_out1.fastq.gz -out2 out/SeqPurge_out2.fastq.gz -ncut 0 -qcut 0 -qc out/SeqPurge_out1.qcML");
		QVERIFY(QFile::exists("out/SeqPurge_out1.fastq.gz"));
		QVERIFY(QFile::exists("out/SeqPurge_out2.fastq.gz"));
		TFW::comareFilesGZ("out/SeqPurge_out1.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out1.fastq.gz"));
		TFW::comareFilesGZ("out/SeqPurge_out2.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out2.fastq.gz"));

		//qc comparison
		TFW::removeLinesContaining("out/SeqPurge_out1.qcML", "creation ");
		TFW::removeLinesContaining("out/SeqPurge_out1.qcML", "source file");
		TFW::removeLinesContaining("out/SeqPurge_out1.qcML", "<binary>");
		TFW::comareFiles("out/SeqPurge_out1.qcML", QFINDTESTDATA("data_out/SeqPurge_out1.qcML"));
	}
	
	//MiSeq 151 cycles - test data where homopolymers mess up one read direction
	void test_02()
	{
	    TFW_EXEC("SeqPurge", "-in1 " + QFINDTESTDATA("data_in/SeqPurge_in3.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/SeqPurge_in4.fastq.gz") + " -out1 out/SeqPurge_out3.fastq.gz -out2 out/SeqPurge_out4.fastq.gz -ncut 0 -qcut 0");
		QVERIFY(QFile::exists("out/SeqPurge_out3.fastq.gz"));
		QVERIFY(QFile::exists("out/SeqPurge_out4.fastq.gz"));
		TFW::comareFilesGZ("out/SeqPurge_out3.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out3.fastq.gz"));
		TFW::comareFilesGZ("out/SeqPurge_out4.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out4.fastq.gz"));
	}
	
	//HiSeq 101 cycles - test data where the adapter sequences are not obvious for sequence hits
	void test_03()
	{
	    TFW_EXEC("SeqPurge", "-in1 " + QFINDTESTDATA("data_in/SeqPurge_in5.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/SeqPurge_in6.fastq.gz") + " -out1 out/SeqPurge_out5.fastq.gz -out2 out/SeqPurge_out6.fastq.gz -ncut 0 -qcut 0");
		QVERIFY(QFile::exists("out/SeqPurge_out5.fastq.gz"));
		QVERIFY(QFile::exists("out/SeqPurge_out6.fastq.gz"));
		TFW::comareFilesGZ("out/SeqPurge_out5.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out5.fastq.gz"));
		TFW::comareFilesGZ("out/SeqPurge_out6.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out6.fastq.gz"));
	}
	
	//HiSeq 151 cycles - test data where reads have different lengths due to trimming by Illumina
	void test_04()
	{
	    TFW_EXEC("SeqPurge", "-in1 " + QFINDTESTDATA("data_in/SeqPurge_in7.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/SeqPurge_in8.fastq.gz") + " -out1 out/SeqPurge_out7.fastq.gz -out2 out/SeqPurge_out8.fastq.gz -a1 CTGTCTCTTATACACATCT -a2 CTGTCTCTTATACACATCT -ncut 0 -qcut 0");
		QVERIFY(QFile::exists("out/SeqPurge_out7.fastq.gz"));
		QVERIFY(QFile::exists("out/SeqPurge_out8.fastq.gz"));
		TFW::comareFilesGZ("out/SeqPurge_out7.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out7.fastq.gz"));
		TFW::comareFilesGZ("out/SeqPurge_out8.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out8.fastq.gz"));
	}
	
	//MiSeq 151 cycles - test data where the adapter sequences are not obvious for sequence hits - with quality trimming
	void test_05()
	{
	    TFW_EXEC("SeqPurge", "-in1 " + QFINDTESTDATA("data_in/SeqPurge_in5.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/SeqPurge_in6.fastq.gz") + " -out1 out/SeqPurge_out9.fastq.gz -out2 out/SeqPurge_out10.fastq.gz -qcut 15 -ncut 0");
	    QVERIFY(QFile::exists("out/SeqPurge_out9.fastq.gz"));
	    QVERIFY(QFile::exists("out/SeqPurge_out10.fastq.gz"));
	    TFW::comareFilesGZ("out/SeqPurge_out9.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out9.fastq.gz"));
	    TFW::comareFilesGZ("out/SeqPurge_out10.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out10.fastq.gz"));
	}
	
	//MiSeq 151 cycles - test data where the adapter sequences are not obvious for sequence hits - with N trimming
	void test_06()
	{
		TFW_EXEC("SeqPurge", "-in1 " + QFINDTESTDATA("data_in/SeqPurge_in5.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/SeqPurge_in6.fastq.gz") + " -out1 out/SeqPurge_out11.fastq.gz -out2 out/SeqPurge_out12.fastq.gz -ncut 7 -qcut 0");
	    QVERIFY(QFile::exists("out/SeqPurge_out11.fastq.gz"));
	    QVERIFY(QFile::exists("out/SeqPurge_out12.fastq.gz"));
	    TFW::comareFilesGZ("out/SeqPurge_out11.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out11.fastq.gz"));
	    TFW::comareFilesGZ("out/SeqPurge_out12.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out12.fastq.gz"));
	}

	//MiSeq 151 cycles - test data where the adapter sequences are not obvious for sequence hits - with quality trimming and N trimming - with singleton output
	void test_07()
	{
		TFW_EXEC("SeqPurge", "-in1 " + QFINDTESTDATA("data_in/SeqPurge_in5.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/SeqPurge_in6.fastq.gz") + " -out1 out/SeqPurge_out13.fastq.gz -out2 out/SeqPurge_out14.fastq.gz -out3 out/SeqPurge_out15 -qcut 25 -min_len_s 30 -min_len_p 80");
	    QVERIFY(QFile::exists("out/SeqPurge_out13.fastq.gz"));
	    QVERIFY(QFile::exists("out/SeqPurge_out14.fastq.gz"));
	    TFW::comareFilesGZ("out/SeqPurge_out13.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out13.fastq.gz"));
	    TFW::comareFilesGZ("out/SeqPurge_out14.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out14.fastq.gz"));
	
	    QVERIFY(QFile::exists("out/SeqPurge_out15_R1.fastq.gz"));
	    QVERIFY(QFile::exists("out/SeqPurge_out15_R2.fastq.gz"));
	    TFW::comareFilesGZ("out/SeqPurge_out15_R1.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out15_R1.fastq.gz"));
	    TFW::comareFilesGZ("out/SeqPurge_out15_R2.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out15_R2.fastq.gz"));
	}

	//MiSeq 300 cycles - test with very long reads (factorial overflow bug)
	void test_08()
	{
		TFW_EXEC("SeqPurge", "-in1 " + QFINDTESTDATA("data_in/SeqPurge_in9.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/SeqPurge_in10.fastq.gz") + " -out1 out/SeqPurge_out16.fastq.gz -out2 out/SeqPurge_out17.fastq.gz");
		QVERIFY(QFile::exists("out/SeqPurge_out16.fastq.gz"));
		QVERIFY(QFile::exists("out/SeqPurge_out17.fastq.gz"));
		TFW::comareFilesGZ("out/SeqPurge_out16.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out16.fastq.gz"));
		TFW::comareFilesGZ("out/SeqPurge_out17.fastq.gz", QFINDTESTDATA("data_out/SeqPurge_out17.fastq.gz"));
	}

	//multi-thread test
	void test_multithread()
	{
		for (int i=1; i<=8; ++i)
		{
			QString suffix = QString::number(i) + "threads";
			QString out1 = "out/SeqPurge_"+suffix+"_R1.fastq.gz";
			QString out2 = "out/SeqPurge_"+suffix+"_R2.fastq.gz";
			int threads = std::max(i, 8);
			TFW_EXEC("SeqPurge", "-in1 " + QFINDTESTDATA("data_in/SeqPurge_in1.fastq.gz") + " -in2 " + QFINDTESTDATA("data_in/SeqPurge_in2.fastq.gz") + " -out1 "+out1+" -out2 "+out2+" -ncut 0 -qcut 0 -qc out/SeqPurge_"+suffix+".qcML -summary out/SeqPurge_"+suffix+".log -threads " + QString::number(threads));

			//compare fastq statistics
			QString out1_stats = "out/SeqPurge_"+suffix+"_R1.stats";
			TFW::fastqStatistics(out1, out1_stats);
			TFW::comareFiles(out1_stats, "out/SeqPurge_1threads_R1.stats");
			QString out2_stats = "out/SeqPurge_"+suffix+"_R2.stats";
			TFW::fastqStatistics(out2, out2_stats);
			TFW::comareFiles(out2_stats, "out/SeqPurge_1threads_R2.stats");

			//check that reads are correctly paired in forward/reverse file
			TFW::fastqCheckPair(out1, out2);

			//qc comparison
			TFW::removeLinesContaining("out/SeqPurge_"+suffix+".qcML", "creation ");
			TFW::removeLinesContaining("out/SeqPurge_"+suffix+".qcML", "source file");
			TFW::removeLinesContaining("out/SeqPurge_"+suffix+".qcML", "<binary>");
			TFW::comareFiles("out/SeqPurge_"+suffix+".qcML", "out/SeqPurge_1threads.qcML");
		}
	}
};


