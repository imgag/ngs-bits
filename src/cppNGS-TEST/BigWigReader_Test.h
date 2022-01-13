#ifndef BIGWIGREADER_TEST_H
#define BIGWIGREADER_TEST_H

#include "TestFramework.h"
#include "BigWigReader.h"
#include <iostream>
#include <math.h>
#include <VcfFile.h>

TEST_CLASS(BigWigReader_Test)
{
Q_OBJECT
private slots:

	void read_local()
	{
		BigWigReader r = BigWigReader(QString(TESTDATA("data_in/BigWigReader.bw")));

		//Header
		BigWigHeader header = r.header();
		I_EQUAL(header.version, 4);
		I_EQUAL(header.zoom_levels, 1);
		I_EQUAL(header.chromosome_tree_offset, 0x158);
		I_EQUAL(header.full_data_offset, 0x190);
		I_EQUAL(header.full_index_offset, 0x1e7);
		I_EQUAL(header.auto_sql_offset, 0x0);
		I_EQUAL(header.total_summary_offset, 0x130);
		I_EQUAL(header.uncompress_buf_size, 32768);

		//Summary
		Summary summary = r.summary();
		I_EQUAL(summary.bases_covered, 154);
		F_EQUAL(summary.min_val, 0.1);
		F_EQUAL(summary.max_val, 2.0);
		F_EQUAL2(summary.sum_data, 272.1, 0.000001);
		F_EQUAL2(summary.sum_squares, 500.389992, 0.000001);

		//Read single values
		float result;
		result = r.readValue("1", 0, 0);
		F_EQUAL2(result, 0.1f, 0.000001);

		result = r.readValue("1", 1, 0);
		F_EQUAL2(result, 0.2f, 0.000001);

		result = r.readValue("1", 100, 0);
		F_EQUAL2(result, 1.4f, 0.000001);

		result = r.readValue("1", 99, 0);
		F_EQUAL2(result, r.defaultValue(), 0.000001);

		//read multiple values
		// existing in file
		QVector<float> intervals = r.readValues("1", 100, 150, 0);
		for (int i=0; i<intervals.size(); i++)
		{
			F_EQUAL2(intervals[i], 1.4f, 0.000001);
		}
		I_EQUAL(intervals.size(), 50);

		// not in file
		intervals = r.readValues("1", 80, 90, 0);
		for (int i=0; i<intervals.size(); i++)
		{
			F_EQUAL2(intervals[i], r.defaultValue(), 0.000001);
		}
		I_EQUAL(intervals.size(), 10);

		//test change default
		float new_default = -42;
		r.setDefault(new_default);

		F_EQUAL(new_default, r.defaultValue());

		intervals = r.readValues("1", 80, 90, 0);
		for (int i=0; i<intervals.size(); i++)
		{
			F_EQUAL2(intervals[i], new_default, 0.000001);
		}
		I_EQUAL(intervals.size(), 10);

		result = r.readValue("1", 50, 0);
		F_EQUAL2(result, new_default, 0.000001);
	}

	void test_vep_annotation_file_test()
	{
		BigWigReader r = BigWigReader(TESTDATA("data_in/BigWigReader_phyloP_chr1_part.bw"));
		QString test_file = TESTDATA("data_in/BigWigReader_vep1.vcf");

		VcfFile vcf = VcfFile();
		vcf.load(test_file);

		int i_phylop = vcf.vcfHeader().vepIndexByName("PHYLOP", false);
		for (int i=0; i<vcf.count(); i++)
		{
			VcfLine v = vcf[i];
			int start = v.start();
			int end = v.end();
			QByteArray chr = v.chr().str();

			double expectedValue = v.vepAnnotations(i_phylop)[0].toDouble();
			//std::cout << "vcf mutation: " << i << " postition - " << chr.toStdString() << ":" << start << "-" << end << "\n";
			F_EQUAL(r.reproduceVepPhylopAnnotation(chr, start, end, QString(v.ref()), QString(v.altString())), expectedValue)
		}
	}
};



#endif // BIGWIGREADER_TEST_H
