#include "TestFramework.h"
#include "ExportCBioPortalStudy.h"


TEST_CLASS(ExportcBioPortal_Test)
{
Q_OBJECT
private slots:

	void test()
	{
		ExportCBioPortalStudy test;

		StudyData study;
		study.name = "Test_study";
		study.description = "A study to test the export";
		study.identifier = "TEST_STUDY_1";
		study.reference_genome = "hg38";
		study.cancer_type = "breast";

		CancerData cancer;
		cancer.description = "Cancer of the breast";
		cancer.color = "hot_pink";
		cancer.parent = "";

		test.setStudy(study);
		test.setCancer(cancer);

		test.exportStudy("C:/local_dev/data/cBioPortaltest/");
	}


};

