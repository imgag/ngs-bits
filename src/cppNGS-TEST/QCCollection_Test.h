#include "TestFramework.h"
#include "QCCollection.h"

TEST_CLASS(QCCollection_Test)
{
Q_OBJECT
private slots:

	void QCValue_string_constructor()
	{
		QCValue value("name", "bla", "desc", "QC:???????");
		I_EQUAL(value.type(), QCValueType::STRING);
		S_EQUAL(value.asString(), "bla");
		S_EQUAL(value.toString(), "bla");
	}

	void QCValue_double_constructor()
	{
		QCValue value("name", 14.56, "desc", "QC:???????");
		I_EQUAL(value.type(), QCValueType::DOUBLE);
		F_EQUAL(value.asDouble(), 14.56);
		S_EQUAL(value.toString(), "14.56");
	}

	void QCValue_long_constructor()
	{
		QCValue value("name", 5147483647ll, "desc", "QC:???????");
		I_EQUAL(value.type(), QCValueType::INT);
		I_EQUAL(value.asInt(), 5147483647);
		S_EQUAL(value.toString(), "5147483647");
	}

	void QCValue_long_constructor_from_int()
	{
		QCValue value("name", 4711, "desc", "QC:???????");
		I_EQUAL(value.type(), QCValueType::INT);
		I_EQUAL(value.asInt(), 4711);
		S_EQUAL(value.toString(), "4711");
	}

	void storeToQCML()
	{
		QCCollection col;
		col.insert(QCValue("read count", 4711, "description1", "QC:2000005"));
		col.insert(QCValue("read length", "bla", "description2", "QC:2000006"));
		col.insert(QCValue("Q20 read percentage", 47.11, "description3", "QC:2000007"));
		col.insert(QCValue::Image("base distribution plot", TESTDATA("data_in/QCCollection_01.png"), "some plot", "QC:2000011"));
		col.storeToQCML("out/QCCollection_qcML_out01.qcML", QStringList() << "bli" << "bla" << "bluff", "some\"nasty parameters");
		REMOVE_LINES("out/QCCollection_qcML_out01.qcML", QRegExp("creation "));
		COMPARE_FILES("out/QCCollection_qcML_out01.qcML", TESTDATA("data_out/QCCollection_qcML_out01.qcML"));
	}

	void loadFromQCML()
	{
		QCCollection col = QCCollection::fromQCML(TESTDATA("data_in/qcML_infile_test.qcML"));
		F_EQUAL(col.value("QC:2000040",true).asDouble(),5.0);
		F_EQUAL(col.value("sample correlation",false).asDouble(),5.0);
		S_EQUAL(col.value("QC:1000002",true).asString(),"TestSoftware1");
		S_EQUAL(col.value("creation software",false).asString(),"TestSoftware1");
		S_EQUAL(col[1].accession(),"QC:1000002");
		S_EQUAL(col[2].accession(),"QC:2000040");
	}
};
