#include "TestFramework.h"
#include "GeneSet.h"

TEST_CLASS(GeneSet_Test)
{
Q_OBJECT
private slots:

	void count()
	{
		GeneSet set;
		I_EQUAL(set.count(), 0);

		set.insert("");
		I_EQUAL(set.count(), 0);

		set.insert("A");
		I_EQUAL(set.count(), 1);

		set.insert("a");
		I_EQUAL(set.count(), 1);

		set.insert("C");
		I_EQUAL(set.count(), 2);

		set.insert("C");
		I_EQUAL(set.count(), 2);

		set.insert("B");
		I_EQUAL(set.count(), 3);

		set.insert("B");
		I_EQUAL(set.count(), 3);

		set.insert("A");
		I_EQUAL(set.count(), 3);

		set.insert("C");
		I_EQUAL(set.count(), 3);

		set.insert("B");
		I_EQUAL(set.count(), 3);

		set.insert(" ");
		I_EQUAL(set.count(), 3);

		set.insert("");
		I_EQUAL(set.count(), 3);

		S_EQUAL(set[0], "A");
		S_EQUAL(set[1], "B");
		S_EQUAL(set[2], "C");
	}

	void clear()
	{
		GeneSet set;
		I_EQUAL(set.count(), 0);

		set.insert("A");
		I_EQUAL(set.count(), 1);

		set.clear();
		I_EQUAL(set.count(), 0);
	}

	void contains()
	{
		GeneSet set;
		set.insert("A");

		IS_TRUE(set.contains("A"));
		IS_TRUE(set.contains("a"));

		IS_FALSE(set.contains("B"));
		IS_FALSE(set.contains("C"));
	}

	void intersectsWith()
	{
		GeneSet set;
		set << "A" << "B";

		GeneSet set2;
		set2 << "C" << "D";

		IS_FALSE(set.intersectsWith(set2));
		IS_FALSE(set2.intersectsWith(set));

		set2 << "B";
		IS_TRUE(set.intersectsWith(set2));
		IS_TRUE(set2.intersectsWith(set));
	}

	void insert_stream()
	{
		GeneSet set;

		set << "A" << "B" << "C";

		I_EQUAL(set.count(), 3);
		S_EQUAL(set[0], "A");
		S_EQUAL(set[1], "B");
		S_EQUAL(set[2], "C");
	}

	void insert_multi()
	{
		GeneSet set;
		set.insert("A");
		set.insert("B");
		GeneSet set2;
		set2.insert("A");
		set2.insert("C");

		set.insert(set2);

		I_EQUAL(set.count(), 3);
		S_EQUAL(set[0], "A");
		S_EQUAL(set[1], "B");
		S_EQUAL(set[2], "C");
	}

	void insert_multi_list()
	{
		GeneSet set;
		set.insert("A");
		set.insert("B");
		QByteArrayList set2;
		set2 << "A";
		set2 << "C";

		set.insert(set2);

		I_EQUAL(set.count(), 3);
		S_EQUAL(set[0], "A");
		S_EQUAL(set[1], "B");
		S_EQUAL(set[2], "C");
	}

	void createFromFile()
	{
		GeneSet set = GeneSet::createFromFile(TESTDATA("data_in/GeneSet_in1.tsv"));

		I_EQUAL(set.count(), 3);
		S_EQUAL(set[0], "A");
		S_EQUAL(set[1], "B");
		S_EQUAL(set[2], "C");
	}

	void createFromText()
	{
		GeneSet set = GeneSet::createFromText("#bla\nA\nC\nB");

		I_EQUAL(set.count(), 3);
		S_EQUAL(set[0], "A");
		S_EQUAL(set[1], "B");
		S_EQUAL(set[2], "C");
	}

	void createFromText_custom_separator()
	{
		GeneSet set = GeneSet::createFromText("#bla,A,C,B", ',');

		I_EQUAL(set.count(), 3);
		S_EQUAL(set[0], "A");
		S_EQUAL(set[1], "B");
		S_EQUAL(set[2], "C");
	}
	void store()
	{
		GeneSet set;
		set.insert("B");
		set.insert("A");
		set.insert("C");
		set.insert("a");

		set.store("out/GeneSet_store.tsv");

		QStringList file = Helper::loadTextFile("out/GeneSet_store.tsv");
		S_EQUAL(file[0], "A");
		S_EQUAL(file[1], "B");
		S_EQUAL(file[2], "C");
	}
};
