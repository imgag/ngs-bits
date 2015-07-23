#include "TestFramework.h"
#include "QCCollection.h"

TEST_CLASS(QCCollection_Test)
{
Q_OBJECT
private slots:

	void storeToQCML()
	{
		QCCollection col;
		col.insert(QCValue("integer", 4711, "description1", "A1"));
		col.insert(QCValue("string", "bla", "description2", "A2"));
		col.insert(QCValue("float", 47.11, "description3", "A3"));
		col.insert(QCValue::Image("image", TESTDATA("data_in/QCCollection_01.png"), "some plot", "A4"));
		col.storeToQCML("out/QCCollection_qcML_out01.qcML", QStringList() << "bli" << "bla" << "bluff", "some\"nasty parameters");

		REMOVE_LINES("out/QCCollection_qcML_out01.qcML", QRegExp("creation "));
		COMPARE_FILES("out/QCCollection_qcML_out01.qcML", TESTDATA("data_out/QCCollection_qcML_out01.qcML"));
	}
};
