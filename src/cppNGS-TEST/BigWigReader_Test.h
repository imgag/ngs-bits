#ifndef BIGWIGREADER_TEST_H
#define BIGWIGREADER_TEST_H

#include "TestFramework.h"
#include "BigWigReader.h"
#include <iostream>
#include <math.h>
#include <VcfFile.h>
#include <iostream>

TEST_CLASS(BigWigReader_Test)
{
private:

    void read_local_values()
	{
		BigWigReader r = BigWigReader(QString(TESTDATA("data_in/BigWigReader.bw")));

		//Header
		BigWigReader::Header header = r.header();
		I_EQUAL(header.version, 4);
		I_EQUAL(header.zoom_levels, 1);
		I_EQUAL(header.chromosome_tree_offset, 0x158);
		I_EQUAL(header.full_data_offset, 0x190);
		I_EQUAL(header.full_index_offset, 0x1e7);
		I_EQUAL(header.auto_sql_offset, 0x0);
		I_EQUAL(header.total_summary_offset, 0x130);
		I_EQUAL(header.uncompress_buf_size, 32768);

		//Summary
		BigWigReader::Summary summary = r.summary();
		I_EQUAL(summary.bases_covered, 154);
		F_EQUAL(summary.min_val, 0.1);
		F_EQUAL(summary.max_val, 2.0);
		F_EQUAL2(summary.sum_data, 272.1, 0.000001);
		F_EQUAL2(summary.sum_squares, 500.389992, 0.000001);


		// Make sure exceptions are thrown if default value is not set:
		IS_THROWN(ProgrammingException, r.readValue("chr1", 0, 0));
		IS_THROWN(ProgrammingException, r.readValues("chr1", 100, 150, 0));
		IS_THROWN(ProgrammingException, r.readValues("chr1:0-1", 0));

		//Read single values
		r.setDefaultValue(-50);
		F_EQUAL(r.defaultValue(), (-50));

		float result;
		result = r.readValue("chr1", 0, 0);
		F_EQUAL2(result, 0.1f, 0.000001);

		result = r.readValue("chr1", 1, 0);
		F_EQUAL2(result, 0.2f, 0.000001);

		result = r.readValue("chr1", 100, 0);
		F_EQUAL2(result, 1.4f, 0.000001);

		result = r.readValue("chr1", 99, 0);
		F_EQUAL2(result, r.defaultValue(), 0.000001);

		//read multiple values
		//existing in file
		QVector<float> intervals = r.readValues("chr1", 100, 150, 0);
		for (int i=0; i<intervals.size(); i++)
		{
			F_EQUAL2(intervals[i], 1.4f, 0.000001);
		}
		I_EQUAL(intervals.size(), 50);

		intervals = r.readValues("chr1:100-110", 0);
		for (int i=0; i<intervals.size(); i++)
		{
			F_EQUAL2(intervals[i], 1.4f, 0.000001);
		}
		I_EQUAL(intervals.size(), 10);

		// not in file
		intervals = r.readValues("chr1", 80, 90, 0);
		for (int i=0; i<intervals.size(); i++)
		{
			F_EQUAL2(intervals[i], r.defaultValue(), 0.000001);
		}
		I_EQUAL(intervals.size(), 10);

		//half half:
		intervals = r.readValues("chr1", 90, 110, 0);
		for (int i=0; i<intervals.size(); i++)
		{
			if (i  <  10)
			{
				F_EQUAL2(intervals[i], r.defaultValue(), 0.000001);
			}
			else
			{
				F_EQUAL2(intervals[i], 1.4f, 0.000001);
			}

		}
		I_EQUAL(intervals.size(), 20);

		//test change default
		float new_default = -42;
		r.setDefaultValue(new_default);

		F_EQUAL(new_default, r.defaultValue());

		intervals = r.readValues("chr1", 80, 90, 0);
		for (int i=0; i<intervals.size(); i++)
		{
			F_EQUAL2(intervals[i], new_default, 0.000001);
		}
		I_EQUAL(intervals.size(), 10);

		result = r.readValue("chr1", 50, 0);
		F_EQUAL2(result, new_default, 0.000001);
	}

    void read_local_intervals()
    {
        BigWigReader r = BigWigReader(QString(TESTDATA("data_in/BigWigReader.bw")));

        // Overlapping single value intervals
		QList<BigWigReader::OverlappingInterval> intervals = r.getOverlappingIntervals("chr1", 0, 1, 0);
        I_EQUAL(intervals.length(), 1);
        I_EQUAL(intervals[0].start, 0);
        I_EQUAL(intervals[0].end, 1);
        F_EQUAL2(intervals[0].value, 0.1f, 0.000001);

		intervals = r.getOverlappingIntervals("chr1", 1, 2, 0);
        I_EQUAL(intervals.length(), 1);
        I_EQUAL(intervals[0].start, 1);
        I_EQUAL(intervals[0].end, 2);
        F_EQUAL2(intervals[0].value, 0.2f, 0.000001);

        // range with multiple one value intervals
		intervals = r.getOverlappingIntervals("chr1", 0, 3, 0);
        I_EQUAL(intervals.length(), 3);

        // single value of large interval?
		intervals = r.getOverlappingIntervals("chr1", 100, 101, 0);
        I_EQUAL(intervals.length(), 1);
        I_EQUAL(intervals[0].start, 100);
        I_EQUAL(intervals[0].end, 150);
        F_EQUAL2(intervals[0].value, 1.4f, 0.000001);

        // no value saved so no overlapping interval
		intervals = r.getOverlappingIntervals("chr1", 99, 100, 0);
        I_EQUAL(intervals.length(), 0)
    }
};



#endif // BIGWIGREADER_TEST_H
