#include "TestFramework.h"
#include "TSVFileStream.h"

TEST_CLASS(TSVFileStream_Test)
{
private:

    TEST_METHOD(empty)
	{
		TSVFileStream stream(TESTDATA("data_in/tsv_empty.txt"));

		I_EQUAL(stream.header().count(), 1);
		S_EQUAL(stream.header()[0], QByteArray(""));
		I_EQUAL(stream.comments().count(), 0);
		I_EQUAL(stream.columns(), 1);
		IS_TRUE(stream.atEnd());
	}

	TEST_METHOD(only_header)
	{
		TSVFileStream stream(TESTDATA("data_in/tsv_only_header.txt"));

		I_EQUAL(stream.header().count(), 3);
		S_EQUAL(stream.header()[0], QByteArray("one"));
		S_EQUAL(stream.header()[1], QByteArray("two"));
		S_EQUAL(stream.header()[2], QByteArray("three"));
		I_EQUAL(stream.comments().count(), 3);
		S_EQUAL(stream.comments()[0], QByteArray("##bli"));
		S_EQUAL(stream.comments()[1], QByteArray("##bla"));
		S_EQUAL(stream.comments()[2], QByteArray("##bluff"));
		I_EQUAL(stream.columns(), 3);
		IS_TRUE(stream.atEnd());
	}

	TEST_METHOD(one_line)
	{
		TSVFileStream stream(TESTDATA("data_in/tsv_one_line.txt"));

		I_EQUAL(stream.header().count(), 3);
		S_EQUAL(stream.header()[0], QByteArray("one"));
		I_EQUAL(stream.colIndex("one", false), 0);
		S_EQUAL(stream.header()[1], QByteArray("two"));
		I_EQUAL(stream.colIndex("two", false), 1);
		S_EQUAL(stream.header()[2], QByteArray("three"));
		I_EQUAL(stream.colIndex("three", false), 2);
		I_EQUAL(stream.colIndex("four", false), -1);
		I_EQUAL(stream.comments().count(), 3);
		S_EQUAL(stream.comments()[0], QByteArray("##bli"));
		S_EQUAL(stream.comments()[1], QByteArray("##bla"));
		S_EQUAL(stream.comments()[2], QByteArray("##bluff"));
		I_EQUAL(stream.columns(), 3);
		IS_FALSE(stream.atEnd());

		QList<QByteArray> line = stream.readLine();
		I_EQUAL(line.count(), 3);
		S_EQUAL(line[0], QByteArray("1"));
		S_EQUAL(line[1], QByteArray("2"));
		S_EQUAL(line[2], QByteArray("3"));
		IS_TRUE(stream.atEnd());

		//test reset method actually works
		stream.reset();

		I_EQUAL(stream.header().count(), 3);
		S_EQUAL(stream.header()[0], QByteArray("one"));
		I_EQUAL(stream.colIndex("one", false), 0);
		S_EQUAL(stream.header()[1], QByteArray("two"));
		I_EQUAL(stream.colIndex("two", false), 1);
		S_EQUAL(stream.header()[2], QByteArray("three"));
		I_EQUAL(stream.colIndex("three", false), 2);
		I_EQUAL(stream.colIndex("four", false), -1);
		I_EQUAL(stream.comments().count(), 3);
		S_EQUAL(stream.comments()[0], QByteArray("##bli"));
		S_EQUAL(stream.comments()[1], QByteArray("##bla"));
		S_EQUAL(stream.comments()[2], QByteArray("##bluff"));
		I_EQUAL(stream.columns(), 3);
		IS_FALSE(stream.atEnd());

		line = stream.readLine();
		I_EQUAL(line.count(), 3);
		S_EQUAL(line[0], QByteArray("1"));
		S_EQUAL(line[1], QByteArray("2"));
		S_EQUAL(line[2], QByteArray("3"));
		IS_TRUE(stream.atEnd());
	}

	TEST_METHOD(many_lines)
	{
		TSVFileStream stream(TESTDATA("data_in/tsv_many_lines.txt"));

		I_EQUAL(stream.header().count(), 3);
		S_EQUAL(stream.header()[0], QByteArray("one"));
		S_EQUAL(stream.header()[1], QByteArray("two"));
		S_EQUAL(stream.header()[2], QByteArray("three"));
		I_EQUAL(stream.comments().count(), 3);
		S_EQUAL(stream.comments()[0], QByteArray("##bli"));
		S_EQUAL(stream.comments()[1], QByteArray("##bla"));
		S_EQUAL(stream.comments()[2], QByteArray("##bluff"));
		I_EQUAL(stream.columns(), 3);
		IS_FALSE(stream.atEnd());

		QList<QByteArray> line = stream.readLine();
		I_EQUAL(line.count(), 3);
		S_EQUAL(line[0], QByteArray("1"));
		S_EQUAL(line[1], QByteArray("2"));
		S_EQUAL(line[2], QByteArray("3"));
		IS_FALSE(stream.atEnd());

		line = stream.readLine();
		I_EQUAL(line.count(), 3);
		S_EQUAL(line[0], QByteArray("4"));
		S_EQUAL(line[1], QByteArray("5"));
		S_EQUAL(line[2], QByteArray("6"));
		IS_FALSE(stream.atEnd());

		line = stream.readLine();
		I_EQUAL(line.count(), 3);
		S_EQUAL(line[0], QByteArray("7"));
		S_EQUAL(line[1], QByteArray("8"));
		S_EQUAL(line[2], QByteArray("9"));
		IS_TRUE(stream.atEnd());
	}
};
