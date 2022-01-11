#ifndef BIGWIGREADER_TEST_H
#define BIGWIGREADER_TEST_H

#include "TestFramework.h"
#include "BigWigReader.h"
#include <iostream>
#include <math.h>

TEST_CLASS(BigWigReader_Test)
{
Q_OBJECT
private slots:
	// TODO move testing files to test folder.
	void testing()
	{
		//BigWigReader r = BigWigReader(QString(TESTDATA("data_in/BigWigReader.bw")));
//		BigWigReader r = BigWigReader(QString("W:/GRCh38/share/data/dbs/phyloP/hg38_phyloP100way_vertebrate.bw"));

//		r.printHeader();
//		r.printSummary();
//		r.printZoomLevels();
//		r.printChromHeader();
//		r.printChromosomes();
//		r.printIndexTree();
	}

	void read_local()
	{
		std::cout << "Size of float: " << sizeof(float) << std::endl;
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
		std::vector<float> intervals = r.readValues("1", 100, 150, 0);
		for (uint i=0; i<intervals.size(); i++)
		{
			F_EQUAL2(intervals[i], 1.4f, 0.000001);
		}
		I_EQUAL(intervals.size(), 50);

		// not in file
		intervals = r.readValues("1", 80, 90, 0);
		for (uint i=0; i<intervals.size(); i++)
		{
			F_EQUAL2(intervals[i], r.defaultValue(), 0.000001);
		}
		I_EQUAL(intervals.size(), 10);

		//test change default
		float new_default = -42;
		r.setDefault(new_default);

		F_EQUAL(new_default, r.defaultValue());

		intervals = r.readValues("1", 80, 90, 0);
		for (uint i=0; i<intervals.size(); i++)
		{
			F_EQUAL2(intervals[i], new_default, 0.000001);
		}
		I_EQUAL(intervals.size(), 10);

		result = r.readValue("1", 50, 0);
		F_EQUAL2(result, new_default, 0.000001);

	}
};



#endif // BIGWIGREADER_TEST_H
