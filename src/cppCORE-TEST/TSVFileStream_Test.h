#include "TestFramework.h"
#include "TSVFileStream.h"

TEST_CLASS(TSVFileStream_Test)
{
Q_OBJECT
private slots:

	void empty()
	{
		TSVFileStream stream(QFINDTESTDATA("data_in/tsv_empty.txt"));

		QCOMPARE(stream.header().count(), 1);
		QCOMPARE(stream.header()[0], QByteArray(""));
		QCOMPARE(stream.comments().count(), 0);
		QCOMPARE(stream.columns(), 1);
		QCOMPARE(stream.atEnd(), true);
	}

	void only_header()
	{
		TSVFileStream stream(QFINDTESTDATA("data_in/tsv_only_header.txt"));

		QCOMPARE(stream.header().count(), 3);
		QCOMPARE(stream.header()[0], QByteArray("one"));
		QCOMPARE(stream.header()[1], QByteArray("two"));
		QCOMPARE(stream.header()[2], QByteArray("three"));
		QCOMPARE(stream.comments().count(), 3);
		QCOMPARE(stream.comments()[0], QByteArray("##bli"));
		QCOMPARE(stream.comments()[1], QByteArray("##bla"));
		QCOMPARE(stream.comments()[2], QByteArray("##bluff"));
		QCOMPARE(stream.columns(), 3);
		QCOMPARE(stream.atEnd(), true);
	}

	void one_line()
	{
		TSVFileStream stream(QFINDTESTDATA("data_in/tsv_one_line.txt"));

		QCOMPARE(stream.header().count(), 3);
		QCOMPARE(stream.header()[0], QByteArray("one"));
		QCOMPARE(stream.header()[1], QByteArray("two"));
		QCOMPARE(stream.header()[2], QByteArray("three"));
		QCOMPARE(stream.comments().count(), 3);
		QCOMPARE(stream.comments()[0], QByteArray("##bli"));
		QCOMPARE(stream.comments()[1], QByteArray("##bla"));
		QCOMPARE(stream.comments()[2], QByteArray("##bluff"));
		QCOMPARE(stream.columns(), 3);
		QCOMPARE(stream.atEnd(), false);

		QList<QByteArray> line = stream.readLine();
		QCOMPARE(line.count(), 3);
		QCOMPARE(line[0], QByteArray("1"));
		QCOMPARE(line[1], QByteArray("2"));
		QCOMPARE(line[2], QByteArray("3"));
		QCOMPARE(stream.atEnd(), true);
	}

	void many_lines()
	{
		TSVFileStream stream(QFINDTESTDATA("data_in/tsv_many_lines.txt"));

		QCOMPARE(stream.header().count(), 3);
		QCOMPARE(stream.header()[0], QByteArray("one"));
		QCOMPARE(stream.header()[1], QByteArray("two"));
		QCOMPARE(stream.header()[2], QByteArray("three"));
		QCOMPARE(stream.comments().count(), 3);
		QCOMPARE(stream.comments()[0], QByteArray("##bli"));
		QCOMPARE(stream.comments()[1], QByteArray("##bla"));
		QCOMPARE(stream.comments()[2], QByteArray("##bluff"));
		QCOMPARE(stream.columns(), 3);
		QCOMPARE(stream.atEnd(), false);

		QList<QByteArray> line = stream.readLine();
		QCOMPARE(line.count(), 3);
		QCOMPARE(line[0], QByteArray("1"));
		QCOMPARE(line[1], QByteArray("2"));
		QCOMPARE(line[2], QByteArray("3"));
		QCOMPARE(stream.atEnd(), false);

		line = stream.readLine();
		QCOMPARE(line.count(), 3);
		QCOMPARE(line[0], QByteArray("4"));
		QCOMPARE(line[1], QByteArray("5"));
		QCOMPARE(line[2], QByteArray("6"));
		QCOMPARE(stream.atEnd(), false);

		line = stream.readLine();
		QCOMPARE(line.count(), 3);
		QCOMPARE(line[0], QByteArray("7"));
		QCOMPARE(line[1], QByteArray("8"));
		QCOMPARE(line[2], QByteArray("9"));
		QCOMPARE(stream.atEnd(), true);
	}
};
