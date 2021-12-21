#ifndef BIGWIGREADER_TEST_H
#define BIGWIGREADER_TEST_H

#include "TestFramework.h"
#include "BigWigReader.h"
#include <iostream>

TEST_CLASS(BigWigReader_Test)
{
Q_OBJECT
private slots:
	// TODO move testing files to test folder.
	void testing()
	{
		BigWigReader r = BigWigReader(QString(TESTDATA("data_in/BigWigReader.bw")));

		BigWigHeader header = r.header();
		r.printHeader();
		r.printSummary();
		r.printZoomLevels();
		r.printChromHeader();
		r.printChromosomes();

		//Header
		I_EQUAL(header.version, 4);
		I_EQUAL(header.zoom_levels, 1);
		I_EQUAL(header.chromosome_tree_offset, 0x158);
		I_EQUAL(header.full_data_offset, 0x190);
		I_EQUAL(header.full_index_offset, 0x1e7);
		I_EQUAL(header.auto_sql_offset, 0x0);
		I_EQUAL(header.total_summary_offset, 0x130);
		I_EQUAL(header.uncompress_buf_size, 32768);

		Summary summary = r.summary();
		//Summary
		I_EQUAL(summary.bases_covered, 154);
		F_EQUAL(summary.min_val, 0.1);
		F_EQUAL(summary.max_val, 2.0);
		F_EQUAL2(summary.sum_data, 272.1, 0.000001);
		F_EQUAL2(summary.sum_squares, 500.389992, 0.000001);
	}
};



#endif // BIGWIGREADER_TEST_H
