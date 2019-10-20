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
};
