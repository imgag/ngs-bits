#include "TestFramework.h"
#include "SvList.h"

TEST_CLASS(SvList_Test)
{
Q_OBJECT
private slots:

	//First: Cases for the class StructuralVariant

	void constructor()
	{
		QByteArrayList annotations_test = {"0","6","0.18518518518519","27","0","4","0","12"};
		StructuralVariant test_sv("DUP","chr1",152054,9712054,20,"PASS","chr13",45131012,"PASS",annotations_test);

		S_EQUAL(test_sv.type(),"DUP");
		S_EQUAL(test_sv.chr().str(),"chr1");
		I_EQUAL(test_sv.start(),152054);
		I_EQUAL(test_sv.end(),9712054);
		I_EQUAL(test_sv.score(),20);
		S_EQUAL(test_sv.getFilter(),"PASS");
		S_EQUAL(test_sv.mateChr().str(),"chr13");
		I_EQUAL(test_sv.matePos(),45131012);
		S_EQUAL(test_sv.getMateFilter(),"PASS");
		//anotation at position 2
		S_EQUAL(test_sv.annotations().at(2),"0.18518518518519");
	}

	//Test SV List
	void loadFromFile()
	{
		SvList svs1;
		svs1.load(TESTDATA("data_in/SvList_source_file.tsv"));

		svs1.checkValid();

		I_EQUAL(svs1.count(),8);
		I_EQUAL(svs1.comments().count(),2);
		I_EQUAL(svs1.annotationHeaders().count(),8);

		I_EQUAL(svs1[2].size(),117829);
		S_EQUAL(svs1[3].getFilter(),"PASS");


		I_EQUAL(svs1.annotationIndexByName("tumor_SR_freq"),3);
	}



};
