#include "../TestFramework.h"
#include "BasicStatistics.h"

class BasicStatistics_Test
		: public QObject
{
	Q_OBJECT

private slots:

	void median()
	{
		QVector<double> data;
		data << 0.5 << 1.0 << 1.5;
		QCOMPARE(BasicStatistics::median(data), 1.0);

		data.clear();
		data << 0.5 << 0.8 << 1.0 << 1.5;
		QCOMPARE(BasicStatistics::median(data), 0.9);
	}

	void isSorted()
	{
		QVector<double> data;
		data << 0.5 << 1.0;
		QVERIFY(BasicStatistics::isSorted(data));

		data << 1.5;
		QVERIFY(BasicStatistics::isSorted(data));

		data << 1.5;
		QVERIFY(BasicStatistics::isSorted(data));

		data << 1.4;
		QVERIFY(!BasicStatistics::isSorted(data));
	}

	void getMinMax()
	{
		QVector<double> data;
		data << 0.5 << 1.0 << 1.5;

		QPair<double, double> output = BasicStatistics::getMinMax(data);
		QCOMPARE(output.first, 0.5);
		QCOMPARE(output.second, 1.5);

		data << 0.1;
		output = BasicStatistics::getMinMax(data);
		QCOMPARE(output.first, 0.1);
		QCOMPARE(output.second, 1.5);

		data << 1.6;
		output = BasicStatistics::getMinMax(data);
		QCOMPARE(output.first, 0.1);
		QCOMPARE(output.second, 1.6);
	}


	void range()
	{
		QVector<double> range = BasicStatistics::range(-1, 5.0, 1.0);
		QCOMPARE(range.count(), 0);

		range = BasicStatistics::range(0, 5.0, 1.0);
		QCOMPARE(range.count(), 0);

		range = BasicStatistics::range(1, 5.0, 1.0);
		QCOMPARE(range.count(), 1);
		QCOMPARE(range[0], 5.0);

		range = BasicStatistics::range(3, 5.0, 1.0);
		QCOMPARE(range.count(), 3);
		QCOMPARE(range[0], 5.0);
		QCOMPARE(range[1], 6.0);
		QCOMPARE(range[2], 7.0);

		range = BasicStatistics::range(3, 5.0, -1.0);
		QCOMPARE(range.count(), 3);
		QCOMPARE(range[0], 5.0);
		QCOMPARE(range[1], 4.0);
		QCOMPARE(range[2], 3.0);
	}

	void isValidFloat()
	{
		QCOMPARE(BasicStatistics::isValidFloat("bla"), false);
		QCOMPARE(BasicStatistics::isValidFloat("nan"), false);
		QCOMPARE(BasicStatistics::isValidFloat("inf"), false);
		QCOMPARE(BasicStatistics::isValidFloat("1"), true);
		QCOMPARE(BasicStatistics::isValidFloat("1.1"), true);
		QCOMPARE(BasicStatistics::isValidFloat("1e-4"), true);
	}
};

TFW_DECLARE(BasicStatistics_Test)
