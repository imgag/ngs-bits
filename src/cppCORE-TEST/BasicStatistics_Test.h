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

};
