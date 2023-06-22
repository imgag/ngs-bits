#include "TestFramework.h"
#include "BasicStatistics.h"

TEST_CLASS(BasicStatistics_Test)
{
Q_OBJECT
private slots:

	void median()
	{
		QVector<double> data;
		data << 0.5 << 1.0 << 1.5;
		F_EQUAL(BasicStatistics::median(data), 1.0);

		data.clear();
		data << 0.5 << 0.8 << 1.0 << 1.5;
		F_EQUAL(BasicStatistics::median(data), 0.9);
	}

	void q1()
	{
		QVector<double> data;
		data << 0.1 << 0.2 << 0.3;
		F_EQUAL(BasicStatistics::q1(data), 0.1);

		data.clear();
		data << 0.1 << 0.2 << 0.3 << 0.4;
		F_EQUAL(BasicStatistics::q1(data), 0.2);

		data.clear();
		data << 0.1 << 0.2 << 0.3 << 0.4 << 0.5;
		F_EQUAL(BasicStatistics::q1(data), 0.2);

		data.clear();
		data << 0.1 << 0.2 << 0.3 << 0.4 << 0.5 << 0.6;
		F_EQUAL(BasicStatistics::q1(data), 0.2);

		data.clear();
		data << 0.1 << 0.2 << 0.3 << 0.4 << 0.5 << 0.6 << 0.7;
		F_EQUAL(BasicStatistics::q1(data), 0.2);

		data.clear();
		data << 0.1 << 0.2 << 0.3 << 0.4 << 0.5 << 0.6 << 0.7 << 0.8;
		F_EQUAL(BasicStatistics::q1(data), 0.3);
	}

	void q3()
	{
		QVector<double> data;
		data << 0.1 << 0.2 << 0.3;
		F_EQUAL(BasicStatistics::q3(data), 0.3);

		data.clear();
		data << 0.1 << 0.2 << 0.3 << 0.4;
		F_EQUAL(BasicStatistics::q3(data), 0.4);

		data.clear();
		data << 0.1 << 0.2 << 0.3 << 0.4 << 0.5;
		F_EQUAL(BasicStatistics::q3(data), 0.4);

		data.clear();
		data << 0.1 << 0.2 << 0.3 << 0.4 << 0.5 << 0.6;
		F_EQUAL(BasicStatistics::q3(data), 0.5);

		data.clear();
		data << 0.1 << 0.2 << 0.3 << 0.4 << 0.5 << 0.6 << 0.7;
		F_EQUAL(BasicStatistics::q3(data), 0.6);

		data.clear();
		data << 0.1 << 0.2 << 0.3 << 0.4 << 0.5 << 0.6 << 0.7 << 0.8;
		F_EQUAL(BasicStatistics::q3(data), 0.7);
	}

	void isSorted()
	{
		QVector<double> data;
		data << 0.5 << 1.0;
		IS_TRUE(BasicStatistics::isSorted(data));

		data << 1.5;
		IS_TRUE(BasicStatistics::isSorted(data));

		data << 1.5;
		IS_TRUE(BasicStatistics::isSorted(data));

		data << 1.4;
		IS_TRUE(!BasicStatistics::isSorted(data));
	}

	void getMinMax()
	{
		QVector<double> data;
		data << 0.5 << 1.0 << 1.5;

		QPair<double, double> output = BasicStatistics::getMinMax(data);
		F_EQUAL(output.first, 0.5);
		F_EQUAL(output.second, 1.5);

		data << 0.1;
		output = BasicStatistics::getMinMax(data);
		F_EQUAL(output.first, 0.1);
		F_EQUAL(output.second, 1.5);

		data << 1.6;
		output = BasicStatistics::getMinMax(data);
		F_EQUAL(output.first, 0.1);
		F_EQUAL(output.second, 1.6);
	}


	void range()
	{
		QVector<double> range = BasicStatistics::range(-1, 5.0, 1.0);
		I_EQUAL(range.count(), 0);

		range = BasicStatistics::range(0, 5.0, 1.0);
		I_EQUAL(range.count(), 0);

		range = BasicStatistics::range(1, 5.0, 1.0);
		I_EQUAL(range.count(), 1);
		F_EQUAL(range[0], 5.0);

		range = BasicStatistics::range(3, 5.0, 1.0);
		I_EQUAL(range.count(), 3);
		F_EQUAL(range[0], 5.0);
		F_EQUAL(range[1], 6.0);
		F_EQUAL(range[2], 7.0);

		range = BasicStatistics::range(3, 5.0, -1.0);
		I_EQUAL(range.count(), 3);
		F_EQUAL(range[0], 5.0);
		F_EQUAL(range[1], 4.0);
		F_EQUAL(range[2], 3.0);
	}

	void isValidFloat()
	{
		IS_FALSE(BasicStatistics::isValidFloat("bla"))
		IS_FALSE(BasicStatistics::isValidFloat("nan"))
		IS_FALSE(BasicStatistics::isValidFloat("inf"))
		IS_TRUE(BasicStatistics::isValidFloat("1"))
		IS_TRUE(BasicStatistics::isValidFloat("1.1"))
		IS_TRUE(BasicStatistics::isValidFloat("1e-4"))
	}

	void factorial()
	{
		BasicStatistics::precalculateFactorials();
		F_EQUAL(BasicStatistics::factorial(0), 1.0);
		F_EQUAL(BasicStatistics::factorial(1), 1.0);
		F_EQUAL(BasicStatistics::factorial(2), 2.0);
		F_EQUAL(BasicStatistics::factorial(3), 6.0);
		F_EQUAL(BasicStatistics::factorial(4), 24.0);
	}

	void matchProbability()
	{
		BasicStatistics::precalculateFactorials();
		F_EQUAL(BasicStatistics::matchProbability(0.1, 1, 1), 0.100);
		F_EQUAL(BasicStatistics::matchProbability(0.1, 1, 2), 0.190);
		F_EQUAL(BasicStatistics::matchProbability(0.1, 1, 3), 0.271);
		F_EQUAL(BasicStatistics::matchProbability(0.1, 1, 5), 0.40951);
		F_EQUAL(BasicStatistics::matchProbability(0.1, 5, 5), 0.00001);
	}

	void rangeOverlaps()
	{
		//no overlap
		IS_FALSE(BasicStatistics::rangeOverlaps(1, 4, 5, 9));
		IS_FALSE(BasicStatistics::rangeOverlaps(5, 9, 1, 4));

		//partial overlap
		IS_TRUE(BasicStatistics::rangeOverlaps(1, 4, 4, 9));
		IS_TRUE(BasicStatistics::rangeOverlaps(4, 9, 1, 4));

		//full overlap
		IS_TRUE(BasicStatistics::rangeOverlaps(1, 10, 4, 4));
		IS_TRUE(BasicStatistics::rangeOverlaps(4, 4, 1, 10));
	}

//	void hypergeometricLogProbability()
//	{
//		BasicStatistics::precalculateLogFactorials();
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(17297, 22673, 14929, 19669), 0.00557, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(7676, 10497, 17362, 22846), 0.00075, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(5508, 2745, 16140, 8681), 0.00019, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(17239, 14348, 24974, 21334), 0.00121, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(11559, 19230, 13393, 22749), 0.00276, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(16392, 19185, 12936, 15172), 0.00633, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(15536, 10603, 14255, 10201), 0.00023, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(11032, 24378, 6835, 16102), 0.00002, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(13540, 23204, 14082, 24675), 0.00205, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(15648, 3393, 3297, 704), 0.01720, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(6142, 2628, 6563, 2546), 0.00016, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(7653, 17779, 4985, 10863), 0.00012, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(14459, 15205, 7616, 8000), 0.00788, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(17802, 4888, 17605, 5055), 0.00130, 0.00001);
//		F_EQUAL2(BasicStatistics::hypergeometricLogProbability(21206, 13006, 12571, 8025), 0.00063, 0.00001);
//	}

	void fishersExactTest_twosided()
	{
		F_EQUAL2(BasicStatistics::fishersExactTest(1, 2, 3, 0, "two-sided"), 0.4, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2, 7, 8, 2, "two-sided"), 0.023014, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1, 9, 11, 3, "two-sided"), 0.00276, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13, 0, 19, 25, "two-sided"), 0.00021, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16, 21, 15, 8, "two-sided"), 0.11752, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22, 21, 6, 12, "two-sided"), 0.26430, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(18, 12, 25, 24, "two-sided"), 0.49074, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 1, 17, 24, "two-sided"), 0.30870, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13, 2, 25, 10, "two-sided"), 0.30439, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 8, 5, 22, "two-sided"), 0.00002, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15, 7, 23, 21, "two-sided"), 0.29288, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13, 23, 17, 12, "two-sided"), 0.08461, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1, 4, 12, 14, "two-sided"), 0.36787, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9, 25, 5, 7, "two-sided"), 0.46702, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7, 8, 12, 12, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9, 8, 7, 14, "two-sided"), 0.32426, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 2, 21, 25, "two-sided"), 0.00384, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 4, 17, 11, "two-sided"), 0.33644, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22, 17, 4, 9, "two-sided"), 0.19935, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22, 14, 5, 10, "two-sided"), 0.12254, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(18, 6, 6, 22, "two-sided"), 0.00021, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21, 15, 25, 17, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 23, 5, 19, "two-sided"), 0.45612, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(18, 7, 3, 18, "two-sided"), 0.00011, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4, 4, 15, 21, "two-sided"), 0.70952, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 22, 17, 9, "two-sided"), 0.32212, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20, 19, 17, 4, "two-sided"), 0.02891, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 3, 3, 25, "two-sided"), 0.05306, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13, 20, 5, 0, "two-sided"), 0.01707, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(25, 8, 20, 15, "two-sided"), 0.12876, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16, 11, 12, 12, "two-sided"), 0.57960, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(25, 13, 20, 21, "two-sided"), 0.17303, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12, 19, 16, 11, "two-sided"), 0.18752, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 15, 22, 0, "two-sided"), 0.00003, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7, 10, 10, 0, "two-sided"), 0.00305, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 7, 14, 21, "two-sided"), 0.75023, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7, 24, 25, 16, "two-sided"), 0.00172, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1, 8, 7, 9, "two-sided"), 0.18219, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4, 13, 24, 16, "two-sided"), 0.01953, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 21, 7, 21, "two-sided"), 0.57381, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8, 0, 15, 18, "two-sided"), 0.00559, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20, 22, 12, 8, "two-sided"), 0.42253, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 15, 20, 14, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15, 13, 14, 19, "two-sided"), 0.44634, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21, 11, 15, 2, "two-sided"), 0.10513, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10, 9, 5, 25, "two-sided"), 0.01166, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(0, 17, 1, 10, "two-sided"), 0.39286, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 25, 25, 1, "two-sided"), 0.00001, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 11, 14, 9, "two-sided"), 0.77785, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13, 24, 4, 13, "two-sided"), 0.53249, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 8, 12, 4, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 3, 14, 16, "two-sided"), 0.00083, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 12, 16, 10, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 9, 0, 7, "two-sided"), 0.12065, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1, 21, 1, 9, "two-sided"), 0.53427, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10, 1, 4, 25, "two-sided"), 0.00001, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10, 3, 11, 25, "two-sided"), 0.00752, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5, 7, 24, 11, "two-sided"), 0.16754, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5, 24, 10, 10, "two-sided"), 0.02599, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 9, 6, 23, "two-sided"), 0.01737, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 5, 18, 2, "two-sided"), 0.06650, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1, 7, 9, 3, "two-sided"), 0.01977, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8, 17, 13, 15, "two-sided"), 0.39985, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7, 5, 4, 1, "two-sided"), 0.60003, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(0, 22, 25, 8, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 15, 24, 22, "two-sided"), 0.51096, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 12, 3, 8, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10, 19, 22, 21, "two-sided"), 0.22719, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17, 13, 23, 8, "two-sided"), 0.18393, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7, 1, 23, 18, "two-sided"), 0.12844, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22, 13, 18, 3, "two-sided"), 0.07748, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 18, 20, 13, "two-sided"), 0.12608, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13, 4, 14, 19, "two-sided"), 0.03551, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17, 22, 3, 5, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16, 0, 4, 20, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 0, 13, 9, "two-sided"), 0.13579, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9, 2, 24, 25, "two-sided"), 0.09077, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9, 8, 20, 15, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20, 21, 11, 7, "two-sided"), 0.41219, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13, 6, 23, 13, "two-sided"), 0.77521, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10, 9, 10, 11, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(25, 15, 9, 12, "two-sided"), 0.17959, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2, 6, 19, 10, "two-sided"), 0.05538, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7, 3, 5, 1, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 4, 14, 23, "two-sided"), 0.00901, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5, 19, 7, 11, "two-sided"), 0.30236, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12, 5, 12, 15, "two-sided"), 0.12441, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16, 21, 8, 15, "two-sided"), 0.59395, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10, 16, 24, 12, "two-sided"), 0.03919, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 4, 20, 8, "two-sided"), 0.69387, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9, 21, 0, 15, "two-sided"), 0.01953, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9, 20, 7, 2, "two-sided"), 0.02107, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4, 1, 8, 24, "two-sided"), 0.03021, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 3, 10, 0, "two-sided"), 0.54482, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8, 9, 24, 5, "two-sided"), 0.01929, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 7, 25, 16, "two-sided"), 0.42944, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 0, 12, 25, "two-sided"), 0.00305, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 23, 0, 0, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17, 23, 21, 18, "two-sided"), 0.37109, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8, 0, 0, 0, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20, 13, 9, 10, "two-sided"), 0.39688, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 17, 8, 6, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(25, 8, 15, 15, "two-sided"), 0.04039, 0.00001);

		//up to 30:
		F_EQUAL2(BasicStatistics::fishersExactTest(28, 30, 15, 26, "two-sided"), 0.30506, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9, 26, 12, 9, "two-sided"), 0.02478, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 13, 11, 19, "two-sided"), 0.04709, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9, 27, 2, 26, "two-sided"), 0.09460, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 20, 20, 10, "two-sided"), 0.02143, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 13, 26, 13, "two-sided"), 0.81330, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7, 2, 19, 30, "two-sided"), 0.06394, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20, 12, 25, 12, "two-sided"), 0.80064, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22, 2, 3, 10, "two-sided"), 0.00004, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10, 29, 4, 8, "two-sided"), 0.71476, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(28, 5, 30, 24, "two-sided"), 0.00528, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(25, 28, 21, 7, "two-sided"), 0.01947, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 19, 15, 10, "two-sided"), 0.45360, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13, 13, 17, 11, "two-sided"), 0.58430, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16, 24, 2, 18, "two-sided"), 0.01892, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 11, 14, 20, "two-sided"), 0.76724, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 19, 12, 25, "two-sided"), 0.57398, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(29, 17, 17, 25, "two-sided"), 0.05394, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(25, 14, 22, 13, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 29, 27, 23, "two-sided"), 0.05884, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(18, 30, 18, 1, "two-sided"), 0.00002, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9, 10, 11, 5, "two-sided"), 0.30636, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 5, 2, 0, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 10, 30, 6, "two-sided"), 0.00238, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(29, 14, 0, 18, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 5, 21, 14, "two-sided"), 0.16060, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 20, 0, 4, "two-sided"), 0.55687, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(30, 14, 4, 8, "two-sided"), 0.04484, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(30, 11, 26, 26, "two-sided"), 0.03272, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 28, 8, 20, "two-sided"), 0.10043, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20, 14, 4, 7, "two-sided"), 0.29888, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(28, 26, 27, 22, "two-sided"), 0.84370, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 16, 19, 28, "two-sided"), 0.03794, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 7, 16, 27, "two-sided"), 0.00171, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 0, 10, 21, "two-sided"), 0.00016, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21, 14, 0, 0, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 24, 3, 30, "two-sided"), 0.00010, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 15, 27, 24, "two-sided"), 0.81664, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 15, 2, 11, "two-sided"), 0.08355, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 25, 19, 12, "two-sided"), 0.05340, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2, 26, 15, 2, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 10, 10, 7, "two-sided"), 0.07112, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 20, 16, 17, "two-sided"), 0.32217, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 24, 6, 16, "two-sided"), 0.28137, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 1, 1, 27, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16, 30, 21, 3, "two-sided"), 0.00003, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(26, 0, 29, 22, "two-sided"), 0.00003, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 24, 7, 27, "two-sided"), 0.05117, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 26, 9, 14, "two-sided"), 0.79017, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 16, 27, 9, "two-sided"), 0.08497, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1, 19, 13, 25, "two-sided"), 0.02144, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9, 0, 19, 22, "two-sided"), 0.00296, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 22, 25, 24, "two-sided"), 0.00101, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 16, 23, 16, "two-sided"), 0.33971, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 21, 19, 27, "two-sided"), 0.63819, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 1, 23, 7, "two-sided"), 0.40235, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17, 27, 29, 19, "two-sided"), 0.05975, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4, 24, 14, 10, "two-sided"), 0.00126, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 15, 14, 17, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(26, 13, 4, 18, "two-sided"), 0.00042, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5, 20, 16, 13, "two-sided"), 0.01186, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 16, 28, 30, "two-sided"), 0.12878, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(18, 8, 15, 19, "two-sided"), 0.06925, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21, 12, 8, 29, "two-sided"), 0.00058, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 10, 7, 14, "two-sided"), 0.00525, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 20, 2, 0, "two-sided"), 0.50680, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 11, 19, 29, "two-sided"), 0.44571, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 2, 5, 28, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22, 23, 13, 10, "two-sided"), 0.61402, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(25, 13, 7, 19, "two-sided"), 0.00466, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8, 19, 22, 27, "two-sided"), 0.22698, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 7, 28, 13, "two-sided"), 0.30728, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 19, 20, 13, "two-sided"), 0.07874, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22, 27, 28, 28, "two-sided"), 0.69614, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(25, 6, 7, 13, "two-sided"), 0.00245, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(28, 30, 12, 29, "two-sided"), 0.06485, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(26, 5, 28, 4, "two-sided"), 0.73228, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 14, 22, 13, "two-sided"), 0.80469, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 15, 0, 28, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 3, 1, 5, "two-sided"), 0.00305, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21, 1, 12, 23, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2, 7, 0, 12, "two-sided"), 0.17143, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 27, 3, 28, "two-sided"), 0.00401, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(0, 27, 11, 3, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(30, 0, 26, 9, "two-sided"), 0.00266, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 6, 26, 6, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(29, 23, 13, 25, "two-sided"), 0.05513, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 21, 3, 13, "two-sided"), 0.01072, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2, 4, 21, 0, "two-sided"), 0.00085, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10, 2, 30, 1, "two-sided"), 0.18362, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7, 10, 10, 10, "two-sided"), 0.74329, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12, 24, 26, 16, "two-sided"), 0.01389, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 24, 28, 18, "two-sided"), 0.13957, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 27, 13, 5, "two-sided"), 0.00023, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 20, 21, 15, "two-sided"), 0.23162, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 12, 6, 27, "two-sided"), 0.00002, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5, 5, 8, 28, "two-sided"), 0.11733, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8, 9, 21, 21, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 25, 14, 7, "two-sided"), 0.00007, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5, 7, 4, 30, "two-sided"), 0.03878, 0.00001);

		//up to 40:
		F_EQUAL2(BasicStatistics::fishersExactTest(2, 5, 17, 10, "two-sided"), 0.19935, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 10, 7, 14, "two-sided"), 0.34989, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10, 5, 38, 9, "two-sided"), 0.29574, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10, 20, 29, 33, "two-sided"), 0.26467, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17, 8, 25, 22, "two-sided"), 0.31595, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(39, 2, 20, 32, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(35, 22, 25, 7, "two-sided"), 0.15699, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(38, 0, 26, 28, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 9, 3, 1, "two-sided"), 0.11813, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(38, 25, 9, 15, "two-sided"), 0.09096, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4, 39, 16, 18, "two-sided"), 0.00022, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(0, 38, 21, 17, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15, 9, 7, 15, "two-sided"), 0.04541, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(31, 10, 6, 7, "two-sided"), 0.08359, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 22, 23, 29, "two-sided"), 0.32205, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 33, 2, 33, "two-sided"), 0.00005, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12, 38, 12, 25, "two-sided"), 0.46886, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(39, 6, 27, 18, "two-sided"), 0.00795, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(33, 21, 18, 10, "two-sided"), 0.81482, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(37, 19, 29, 28, "two-sided"), 0.12758, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 12, 31, 22, "two-sided"), 0.82231, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 8, 11, 37, "two-sided"), 0.00001, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(18, 11, 19, 16, "two-sided"), 0.61495, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 7, 20, 8, "two-sided"), 0.55662, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2, 39, 40, 15, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 19, 2, 19, "two-sided"), 0.01383, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(36, 21, 0, 28, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15, 39, 17, 7, "two-sided"), 0.00050, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(34, 20, 3, 7, "two-sided"), 0.08099, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 10, 35, 26, "two-sided"), 0.13591, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(18, 35, 24, 14, "two-sided"), 0.01008, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 35, 37, 38, "two-sided"), 0.00001, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20, 40, 27, 22, "two-sided"), 0.03219, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 27, 18, 17, "two-sided"), 0.05926, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 24, 23, 16, "two-sided"), 0.02120, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(35, 3, 31, 33, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(39, 35, 9, 19, "two-sided"), 0.07740, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 21, 0, 24, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4, 25, 14, 0, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21, 34, 28, 6, "two-sided"), 0.00006, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20, 5, 35, 38, "two-sided"), 0.00558, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(0, 9, 1, 7, "two-sided"), 0.47059, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5, 34, 19, 17, "two-sided"), 0.00039, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(38, 23, 37, 40, "two-sided"), 0.12168, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(26, 34, 35, 12, "two-sided"), 0.00161, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15, 8, 3, 38, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(32, 12, 34, 40, "two-sided"), 0.00696, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12, 7, 22, 4, "two-sided"), 0.16011, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1, 6, 23, 31, "two-sided"), 0.22858, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(29, 19, 5, 35, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20, 40, 18, 22, "two-sided"), 0.29451, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(40, 14, 11, 0, "two-sided"), 0.10240, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(38, 38, 6, 9, "two-sided"), 0.57697, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17, 30, 1, 29, "two-sided"), 0.00071, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(25, 25, 0, 4, "two-sided"), 0.11510, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(35, 8, 28, 3, "two-sided"), 0.34004, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(35, 13, 33, 7, "two-sided"), 0.31825, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17, 0, 25, 12, "two-sided"), 0.01067, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21, 12, 24, 22, "two-sided"), 0.36168, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(35, 5, 36, 24, "two-sided"), 0.00335, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(35, 1, 27, 18, "two-sided"), 0.00005, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(25, 25, 11, 15, "two-sided"), 0.63015, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12, 36, 36, 6, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20, 33, 0, 7, "two-sided"), 0.08380, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2, 17, 20, 4, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8, 11, 32, 9, "two-sided"), 0.00880, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(0, 39, 35, 11, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7, 40, 8, 5, "two-sided"), 0.00167, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15, 20, 3, 16, "two-sided"), 0.06919, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15, 3, 39, 4, "two-sided"), 0.41088, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 38, 30, 38, "two-sided"), 0.47912, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8, 12, 37, 23, "two-sided"), 0.12009, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 7, 19, 33, "two-sided"), 0.54053, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5, 24, 25, 27, "two-sided"), 0.00794, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(31, 7, 31, 19, "two-sided"), 0.06007, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7, 17, 20, 19, "two-sided"), 0.11727, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20, 37, 18, 9, "two-sided"), 0.00960, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(33, 13, 2, 25, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 2, 18, 20, "two-sided"), 0.00157, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21, 2, 9, 2, "two-sided"), 0.57993, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9, 2, 17, 37, "two-sided"), 0.00470, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(35, 8, 10, 39, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5, 20, 37, 34, "two-sided"), 0.00908, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(30, 30, 15, 16, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 4, 33, 17, "two-sided"), 0.04029, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 18, 31, 0, "two-sided"), 0.00002, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(33, 6, 26, 8, "two-sided"), 0.55241, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22, 7, 29, 8, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16, 40, 21, 2, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 25, 30, 5, "two-sided"), 0.00001, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(26, 38, 2, 20, "two-sided"), 0.00757, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(32, 15, 34, 7, "two-sided"), 0.14088, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(40, 38, 20, 16, "two-sided"), 0.69182, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 7, 16, 17, "two-sided"), 0.26336, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(26, 16, 26, 34, "two-sided"), 0.07343, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 34, 31, 10, "two-sided"), 0.00002, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(28, 2, 11, 9, "two-sided"), 0.00357, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(29, 29, 39, 29, "two-sided"), 0.47449, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 25, 31, 4, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(29, 7, 34, 9, "two-sided"), 1.00000, 0.00001);

		//up to 50:
		F_EQUAL2(BasicStatistics::fishersExactTest(48, 39, 18, 21, "two-sided"), 0.44067, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(26, 24, 42, 31, "two-sided"), 0.58305, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(41, 30, 11, 50, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(47, 43, 1, 38, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(38, 37, 2, 27, "two-sided"), 0.00003, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 8, 19, 30, "two-sided"), 0.00161, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 4, 38, 14, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(48, 2, 13, 30, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(0, 13, 27, 42, "two-sided"), 0.00379, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(49, 23, 21, 8, "two-sided"), 0.81254, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 38, 10, 35, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(29, 9, 25, 21, "two-sided"), 0.04239, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(49, 32, 44, 11, "two-sided"), 0.02358, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(31, 23, 42, 43, "two-sided"), 0.38755, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 38, 24, 19, "two-sided"), 0.11124, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 30, 31, 12, "two-sided"), 0.00007, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(41, 8, 41, 31, "two-sided"), 0.00273, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14, 5, 43, 33, "two-sided"), 0.20066, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2, 47, 19, 14, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(18, 47, 34, 2, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(43, 33, 12, 50, "two-sided"), 0.00001, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22, 9, 10, 0, "two-sided"), 0.08303, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16, 35, 46, 17, "two-sided"), 0.00001, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(43, 18, 18, 17, "two-sided"), 0.07910, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(32, 13, 41, 42, "two-sided"), 0.02451, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(28, 38, 19, 1, "two-sided"), 0.00002, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(42, 1, 42, 3, "two-sided"), 0.61663, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(42, 37, 25, 46, "two-sided"), 0.03291, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17, 41, 35, 23, "two-sided"), 0.00139, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(31, 46, 32, 16, "two-sided"), 0.00566, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(42, 8, 15, 29, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(39, 31, 29, 11, "two-sided"), 0.10356, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7, 7, 39, 11, "two-sided"), 0.05072, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(35, 7, 34, 50, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8, 16, 49, 22, "two-sided"), 0.00339, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(44, 19, 24, 40, "two-sided"), 0.00035, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1, 35, 12, 3, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(48, 11, 45, 12, "two-sided"), 0.81795, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(30, 23, 3, 33, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 5, 45, 11, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19, 24, 1, 1, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(36, 50, 30, 22, "two-sided"), 0.08076, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2, 47, 43, 3, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1, 7, 15, 2, "two-sided"), 0.00054, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(28, 2, 30, 26, "two-sided"), 0.00011, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(50, 35, 47, 2, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(43, 13, 35, 12, "two-sided"), 0.82067, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1, 32, 17, 39, "two-sided"), 0.00197, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 48, 32, 42, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17, 16, 24, 35, "two-sided"), 0.38365, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23, 10, 23, 29, "two-sided"), 0.02667, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(40, 14, 11, 41, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(39, 21, 39, 28, "two-sided"), 0.46915, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(28, 44, 19, 11, "two-sided"), 0.03007, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10, 6, 19, 15, "two-sided"), 0.76348, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(35, 30, 9, 30, "two-sided"), 0.00228, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(29, 39, 12, 34, "two-sided"), 0.07748, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(50, 10, 37, 50, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(44, 36, 29, 9, "two-sided"), 0.02776, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1, 21, 50, 38, "two-sided"), 0.00001, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(35, 31, 4, 36, "two-sided"), 0.00001, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(30, 37, 12, 44, "two-sided"), 0.00771, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(34, 45, 32, 42, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(36, 12, 31, 25, "two-sided"), 0.04221, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(35, 22, 45, 4, "two-sided"), 0.00026, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(42, 22, 43, 25, "two-sided"), 0.85626, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12, 2, 31, 14, "two-sided"), 0.31016, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(25, 37, 17, 20, "two-sided"), 0.67541, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 6, 36, 22, "two-sided"), 0.14929, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(28, 43, 34, 34, "two-sided"), 0.23505, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(26, 7, 28, 15, "two-sided"), 0.21393, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11, 0, 11, 19, "two-sided"), 0.00025, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12, 8, 30, 49, "two-sided"), 0.08350, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(28, 6, 44, 2, "two-sided"), 0.06618, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(39, 32, 26, 26, "two-sided"), 0.71483, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13, 31, 19, 41, "two-sided"), 0.83364, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10, 19, 21, 22, "two-sided"), 0.33190, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 47, 8, 7, "two-sided"), 0.25558, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(27, 16, 10, 32, "two-sided"), 0.00042, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(32, 27, 31, 37, "two-sided"), 0.37591, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(29, 15, 18, 10, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15, 19, 9, 12, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(30, 18, 21, 7, "two-sided"), 0.31769, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13, 39, 28, 39, "two-sided"), 0.07962, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 20, 27, 5, "two-sided"), 0.00714, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 26, 33, 14, "two-sided"), 0.03867, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(44, 42, 46, 15, "two-sided"), 0.00349, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(30, 36, 13, 17, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21, 38, 8, 41, "two-sided"), 0.02990, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(48, 44, 46, 16, "two-sided"), 0.00708, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22, 42, 27, 35, "two-sided"), 0.36128, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15, 18, 39, 7, "two-sided"), 0.00045, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(36, 2, 41, 21, "two-sided"), 0.00110, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(42, 26, 43, 23, "two-sided"), 0.72228, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3, 6, 23, 44, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(47, 43, 4, 26, "two-sided"), 0.00021, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 42, 5, 49, "two-sided"), 0.00055, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(39, 1, 30, 31, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24, 34, 23, 34, "two-sided"), 1.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5, 40, 4, 32, "two-sided"), 1.00000, 0.00001);

		//up to 500
		F_EQUAL2(BasicStatistics::fishersExactTest(75, 96, 382, 176, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(247, 32, 64, 296, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(312, 162, 274, 469, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(265, 386, 228, 40, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(148, 42, 347, 207, "two-sided"), 0.00012, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(74, 325, 137, 165, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(297, 94, 142, 363, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(412, 417, 453, 352, "two-sided"), 0.00860, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(363, 123, 415, 448, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(449, 248, 414, 321, "two-sided"), 0.00206, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(470, 350, 323, 159, "two-sided"), 0.00052, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(218, 405, 360, 340, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(433, 368, 170, 295, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(326, 339, 85, 436, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(451, 346, 294, 441, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(500, 439, 118, 176, "two-sided"), 0.00010, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(326, 184, 234, 28, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(248, 247, 394, 191, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(259, 10, 120, 199, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(380, 187, 54, 113, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(353, 252, 422, 93, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(104, 306, 69, 275, "two-sided"), 0.09840, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(193, 198, 300, 482, "two-sided"), 0.00035, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(439, 474, 368, 277, "two-sided"), 0.00056, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(189, 470, 21, 40, "two-sided"), 0.37728, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(26, 239, 372, 382, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(28, 360, 326, 78, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(198, 125, 14, 158, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(76, 378, 124, 483, "two-sided"), 0.13253, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(107, 313, 404, 358, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(191, 383, 214, 356, "two-sided"), 0.13807, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(368, 442, 180, 352, "two-sided"), 0.00003, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(315, 188, 325, 410, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(217, 286, 125, 263, "two-sided"), 0.00108, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(410, 229, 438, 428, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(242, 79, 196, 299, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(471, 42, 223, 163, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(233, 303, 213, 236, "two-sided"), 0.22242, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(337, 412, 333, 77, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(476, 446, 64, 102, "two-sided"), 0.00233, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(361, 386, 128, 378, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(166, 158, 168, 317, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(379, 70, 78, 448, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(181, 106, 3, 458, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(473, 282, 272, 63, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(239, 17, 139, 459, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(444, 186, 476, 493, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(357, 16, 366, 115, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(122, 360, 397, 192, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(263, 415, 189, 218, "two-sided"), 0.01561, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(201, 318, 135, 233, "two-sided"), 0.57424, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(464, 464, 375, 51, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(205, 71, 170, 196, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(493, 284, 294, 15, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(477, 237, 344, 229, "two-sided"), 0.01220, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(175, 384, 211, 181, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(174, 1, 123, 490, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(452, 500, 57, 456, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(419, 150, 274, 135, "two-sided"), 0.02699, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(321, 3, 20, 360, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(299, 382, 8, 219, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(0, 405, 435, 489, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(290, 376, 142, 432, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(365, 71, 250, 77, "two-sided"), 0.01266, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(148, 294, 171, 12, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(234, 251, 466, 131, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(169, 6, 421, 136, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(238, 94, 234, 288, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(188, 441, 267, 217, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(41, 406, 140, 102, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(484, 375, 126, 421, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(57, 280, 376, 163, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(58, 197, 124, 349, "two-sided"), 0.32442, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(420, 320, 180, 432, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(194, 249, 438, 91, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(76, 407, 447, 174, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(299, 75, 129, 282, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(150, 66, 358, 480, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(421, 321, 475, 87, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(241, 468, 5, 157, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(345, 56, 418, 313, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(347, 231, 362, 217, "two-sided"), 0.39847, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(79, 100, 179, 415, "two-sided"), 0.00079, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(416, 310, 449, 174, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(59, 134, 403, 344, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6, 374, 64, 466, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(426, 67, 140, 319, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(463, 387, 267, 497, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(75, 295, 399, 296, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(283, 199, 253, 99, "two-sided"), 0.00010, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(35, 320, 452, 147, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(194, 75, 496, 377, "two-sided"), 0.00001, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(82, 324, 202, 24, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(456, 240, 479, 260, "two-sided"), 0.78201, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(428, 365, 442, 297, "two-sided"), 0.02315, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(224, 325, 458, 287, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(428, 34, 494, 475, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(406, 66, 182, 59, "two-sided"), 0.00081, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(123, 87, 196, 490, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(361, 75, 377, 106, "two-sided"), 0.08102, 0.00001);

		//up to 5000
		F_EQUAL2(BasicStatistics::fishersExactTest(347, 3862, 1716, 4507, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2114, 4370, 3419, 3310, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4126, 14, 2753, 2288, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(597, 714, 790, 381, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(57, 244, 2139, 1300, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4250, 353, 1653, 4938, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4098, 470, 1928, 992, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(636, 2945, 4779, 4323, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(529, 599, 1405, 1100, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2800, 2460, 3497, 4180, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(176, 2244, 2914, 284, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4105, 2165, 3857, 3355, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3327, 3015, 1490, 3451, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1225, 4358, 400, 3459, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2502, 4166, 678, 4800, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2320, 4414, 970, 4600, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3950, 549, 34, 3269, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3835, 918, 4312, 566, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2587, 2875, 2266, 4508, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2116, 2501, 3516, 2332, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1423, 4158, 4116, 1763, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4855, 3832, 2673, 1402, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(966, 4110, 1231, 1547, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3878, 834, 1350, 302, "two-sided"), 0.60127, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2301, 2526, 2333, 372, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(608, 3034, 116, 1744, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2704, 3049, 781, 3461, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4200, 364, 2209, 211, "two-sided"), 0.29279, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4093, 4200, 4750, 3885, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3197, 4818, 4382, 4807, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2021, 2743, 4717, 3439, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1840, 3773, 4888, 4412, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(986, 3746, 3695, 241, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(587, 2654, 184, 2720, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3919, 3234, 1108, 2012, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3507, 4430, 4282, 2023, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1131, 308, 564, 4927, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4458, 1616, 1980, 2154, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4138, 4229, 2954, 1382, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1046, 3611, 1261, 2841, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2277, 331, 1019, 1695, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3524, 2335, 4313, 4584, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(892, 3427, 1321, 3489, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1027, 1619, 3939, 3225, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2367, 1515, 4644, 4841, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(774, 3739, 2194, 681, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2489, 1832, 686, 4203, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4838, 3971, 1330, 4535, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3866, 3904, 4922, 888, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3203, 3416, 689, 2018, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3760, 2798, 2588, 952, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3806, 3531, 48, 3285, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2046, 1670, 773, 3761, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4114, 4782, 173, 2599, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3833, 597, 3163, 3669, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1980, 2278, 2374, 4669, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(743, 1948, 3698, 2889, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1575, 1607, 2726, 748, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1252, 4067, 243, 801, "two-sided"), 0.87319, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3737, 1942, 3145, 2265, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2438, 1295, 622, 2125, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(975, 3314, 3640, 3459, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(217, 1255, 1044, 573, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3897, 4181, 3648, 2071, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2281, 2170, 4578, 2780, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1043, 3938, 763, 1407, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(652, 3827, 4048, 4536, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(40, 383, 1331, 4397, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4756, 3818, 4423, 4323, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1361, 3217, 2073, 696, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3658, 1755, 3755, 3297, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3588, 4226, 1010, 2903, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1919, 3524, 1305, 4148, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3901, 1394, 3701, 436, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2649, 2983, 4489, 4245, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1068, 937, 2899, 203, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3162, 1226, 4777, 3283, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(224, 294, 4298, 3105, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3315, 4637, 4145, 836, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1600, 2630, 2612, 2989, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3347, 3654, 3, 3766, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3446, 296, 4813, 4630, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4366, 3247, 3650, 1073, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(915, 1382, 2417, 2089, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3415, 2060, 687, 2677, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1541, 57, 887, 1398, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1457, 3640, 3453, 4554, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4343, 3676, 3093, 3568, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2848, 2798, 3176, 2509, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1213, 4769, 145, 3245, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(370, 4099, 1849, 4890, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3970, 2575, 381, 262, "two-sided"), 0.49886, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3517, 4216, 1831, 1413, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3425, 3175, 4628, 2314, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(153, 3486, 2474, 1759, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4747, 384, 1726, 3831, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3019, 1472, 4460, 1764, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3821, 2294, 276, 1222, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2354, 4099, 4546, 2867, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(662, 2519, 4575, 1179, "two-sided"), 0.00000, 0.00001);

		//up to 25000
		F_EQUAL2(BasicStatistics::fishersExactTest(16324, 19998, 1701, 6661, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5277, 20165, 15056, 16452, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7724, 22225, 2987, 4434, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16621, 7885, 8986, 9708, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23541, 13479, 20091, 18140, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21834, 22642, 10960, 5072, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9203, 2823, 19448, 2245, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(18597, 24193, 20560, 7284, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5013, 23656, 455, 16489, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23799, 3780, 4197, 7106, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21810, 22604, 20144, 10547, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21148, 22696, 11898, 23943, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6140, 4287, 16059, 16328, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17977, 11853, 14023, 11467, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23342, 19268, 6340, 11690, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3559, 17137, 22446, 15849, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10162, 20999, 11246, 5394, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9865, 13150, 12632, 10261, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5685, 14905, 2535, 16486, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3763, 14416, 15240, 2588, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16500, 9390, 17983, 1780, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14387, 22865, 4213, 13807, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2924, 6612, 23929, 15168, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11261, 12928, 5461, 6498, "two-sided"), 0.11143, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12369, 22767, 11478, 24345, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(376, 19084, 6444, 5851, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4613, 4175, 14430, 8912, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8786, 20896, 23435, 10641, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9298, 989, 23297, 7583, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19694, 17033, 5172, 2276, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3544, 6908, 2231, 18626, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8416, 24237, 19203, 22429, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13041, 17785, 228, 16449, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19680, 749, 9096, 393, "two-sided"), 0.04796, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(105, 11182, 13628, 4157, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21797, 24733, 4712, 13537, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21913, 4332, 17603, 23409, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19022, 19500, 24454, 1768, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21224, 13434, 7251, 7233, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15335, 19032, 18319, 15353, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3029, 12069, 5053, 23503, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3692, 4559, 925, 13436, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20542, 14542, 7084, 7508, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5687, 1587, 23284, 12373, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11280, 9259, 8217, 1030, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4623, 19841, 6539, 17854, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19529, 2842, 9269, 1364, "two-sided"), 0.75058, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15072, 150, 9162, 469, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23019, 13172, 10644, 799, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22769, 20121, 1321, 6239, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13776, 15339, 8822, 15172, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24562, 11546, 7807, 5299, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6194, 8164, 18315, 11737, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(18924, 7235, 9845, 8779, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19454, 3632, 13927, 7128, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2097, 4267, 18237, 15211, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8428, 20162, 8470, 12924, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8271, 15884, 9056, 20132, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16717, 6152, 19784, 17782, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11601, 16472, 21013, 4239, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21733, 7351, 16762, 3690, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15951, 16394, 24094, 5675, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16847, 14412, 24782, 17855, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15670, 15986, 8488, 11305, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22246, 12726, 7787, 10214, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24692, 13672, 5393, 19059, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7758, 3613, 2806, 6756, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7472, 6848, 9719, 20648, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16944, 14171, 18087, 10636, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9734, 19078, 12937, 281, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12883, 6321, 23726, 12464, "two-sided"), 0.00030, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5271, 4990, 24679, 11299, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5914, 19901, 7697, 7009, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16943, 18550, 19031, 15574, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4702, 15376, 14539, 14469, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3144, 22774, 2431, 3582, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11074, 5882, 24858, 13574, "two-sided"), 0.15297, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(413, 22483, 8056, 7096, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3595, 21296, 21232, 8161, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13998, 9006, 5987, 18114, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(588, 1130, 10797, 24475, "two-sided"), 0.00173, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(76, 10594, 18436, 15025, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20803, 6506, 5491, 1847, "two-sided"), 0.01720, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19326, 9627, 1799, 5001, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24561, 13874, 8384, 11106, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(601, 10182, 4031, 14328, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19422, 2519, 8923, 16144, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17661, 4239, 14611, 13835, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15233, 7487, 1722, 4570, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17046, 14375, 8988, 8612, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3956, 12409, 24451, 15838, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21327, 7500, 16955, 22482, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9898, 9276, 18930, 12202, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7752, 15823, 16899, 4814, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6787, 11596, 16915, 19444, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(18387, 5141, 113, 21326, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12953, 6509, 17322, 14539, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22868, 7730, 8253, 21311, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8642, 9167, 2359, 9293, "two-sided"), 0.00000, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16644, 10197, 24920, 21279, "two-sided"), 0.00000, 0.00001);
	}

	void fishersExactTest_less()
	{
		F_EQUAL2(BasicStatistics::fishersExactTest(14892, 8676, 23997, 14842, "less"), 0.99978, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9727, 4322, 2233, 991, "less"), 0.49781, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19060, 12176, 11391, 7656, "less"), 0.99665, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14001, 22638, 8157, 12414, "less"), 0.00036, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24685, 17347, 12412, 8726, "less"), 0.51325, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10722, 9894, 4438, 4232, "less"), 0.90234, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(5311, 21872, 5662, 22244, "less"), 0.01398, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13635, 14681, 1587, 1795, "less"), 0.91458, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20774, 10137, 12866, 6605, "less"), 0.99568, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3318, 1844, 13384, 8306, "less"), 0.99972, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4633, 4553, 17292, 16034, "less"), 0.00708, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(15595, 15985, 1709, 1961, "less"), 0.99942, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14817, 19554, 16085, 20369, "less"), 0.00331, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23924, 23136, 5326, 5411, "less"), 0.98974, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17172, 16441, 18964, 18193, "less"), 0.55571, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(26, 51, 11684, 19732, "less"), 0.31028, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(23782, 21676, 15949, 13646, "less"), 0.00001, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8337, 6428, 236, 244, "less"), 0.99933, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8368, 14584, 8031, 13971, "less"), 0.46658, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19800, 18192, 1128, 891, "less"), 0.00054, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7761, 10115, 6385, 7844, "less"), 0.00464, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19991, 16920, 24468, 21192, "less"), 0.95033, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19730, 19636, 20364, 19281, "less"), 0.00024, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6170, 15151, 7658, 18307, "less"), 0.09510, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16740, 10265, 20434, 12075, "less"), 0.01507, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20158, 12028, 18059, 10108, "less"), 0.00008, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(24672, 13939, 11990, 6315, "less"), 0.00010, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13442, 12778, 15554, 13797, "less"), 0.00002, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12277, 17078, 8597, 11688, "less"), 0.10935, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10929, 10150, 18874, 17221, "less"), 0.15578, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6355, 2592, 18505, 7784, "less"), 0.87670, 0.00001);
	}

	void fishersExactTest_greater()
	{
		F_EQUAL2(BasicStatistics::fishersExactTest(20327, 15152, 8743, 6525, "greater"), 0.47927, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14209, 7305, 11114, 5664, "greater"), 0.66025, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(18691, 22828, 7383, 9147, "greater"), 0.22247, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(7257, 8129, 19655, 23569, "greater"), 0.00015, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11193, 11095, 12625, 12581, "greater"), 0.39006, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(78, 5353, 302, 19574, "greater"), 0.69186, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(21376, 14011, 21353, 14100, "greater"), 0.31751, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3613, 1117, 22453, 7070, "greater"), 0.31661, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2391, 1933, 14122, 12384, "greater"), 0.00711, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13536, 18990, 7052, 9671, "greater"), 0.88277, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6834, 1943, 427, 88, "greater"), 0.99758, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11911, 12742, 12758, 13699, "greater"), 0.42023, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(11231, 11349, 6759, 7264, "greater"), 0.00216, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9707, 11851, 10652, 12784, "greater"), 0.81920, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17606, 5220, 22865, 6948, "greater"), 0.12156, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(22710, 15419, 8157, 5249, "greater"), 0.99564, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8834, 3198, 21038, 7032, "greater"), 0.99936, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8595, 16029, 12040, 23473, "greater"), 0.00562, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2691, 3502, 15861, 21065, "greater"), 0.23575, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(9474, 13875, 5834, 8053, "greater"), 0.99685, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14330, 18312, 15966, 19534, "greater"), 0.99765, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3011, 15756, 2773, 15080, "greater"), 0.09211, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(12845, 15176, 12784, 14220, "greater"), 0.99980, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1993, 338, 21004, 4493, "greater"), 0.00006, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13599, 14338, 766, 851, "greater"), 0.15968, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8474, 8380, 18531, 19529, "greater"), 0.00030, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17642, 18402, 17311, 18488, "greater"), 0.05780, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(2883, 3801, 11002, 16145, "greater"), 0.00006, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(8928, 9679, 20030, 22484, "greater"), 0.02449, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14221, 13965, 21134, 20909, "greater"), 0.31670, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(16508, 21909, 5970, 7494, "greater"), 0.99719, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(20818, 17133, 15213, 12494, "greater"), 0.55548, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19946, 18949, 6303, 5632, "greater"), 0.99834, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14594, 13421, 21449, 19846, "greater"), 0.34942, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13129, 5222, 9747, 3968, "greater"), 0.17894, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(13855, 15567, 18439, 20948, "greater"), 0.23912, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14307, 20207, 11750, 15631, "greater"), 0.99987, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6436, 24625, 5591, 20064, "greater"), 0.99909, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4382, 7573, 13200, 23161, "greater"), 0.24754, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(19863, 22048, 19283, 22277, "greater"), 0.00203, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10551, 18823, 9922, 16955, "greater"), 0.99311, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(17999, 8140, 7167, 3395, "greater"), 0.03154, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(759, 1914, 7454, 17574, "greater"), 0.93557, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(4684, 3380, 10376, 8097, "greater"), 0.00194, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(3664, 6531, 8128, 13430, "greater"), 0.99887, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14378, 15176, 20090, 22477, "greater"), 0.00006, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(6340, 23369, 5851, 22095, "greater"), 0.11986, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(10602, 9491, 13294, 12661, "greater"), 0.00052, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14006, 12925, 23349, 22577, "greater"), 0.00121, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(1794, 2423, 19559, 23883, "greater"), 0.99908, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(739, 824, 17489, 24140, "greater"), 0.00002, 0.00001);
		F_EQUAL2(BasicStatistics::fishersExactTest(14593, 10268, 7481, 5349, "greater"), 0.23674, 0.00001);
	}
};
