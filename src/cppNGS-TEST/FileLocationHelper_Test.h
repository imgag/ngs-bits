#include "TestFramework.h"
#include "FileLocationHelper.h"

TEST_CLASS(FileLocationHelper_Test)
{
Q_OBJECT
private slots:
	void convert_pathTypeToString()
	{
		QString text = FileLocationHelper::pathTypeToString(PathType::PROJECT_FOLDER);
		S_EQUAL(text, "PROJECT_FOLDER");

		text = FileLocationHelper::pathTypeToString(PathType::SAMPLE_FOLDER);
		S_EQUAL(text, "SAMPLE_FOLDER");

		text = FileLocationHelper::pathTypeToString(PathType::BAM);
		S_EQUAL(text, "BAM");

		text = FileLocationHelper::pathTypeToString(PathType::GSVAR);
		S_EQUAL(text, "GSVAR");

		text = FileLocationHelper::pathTypeToString(PathType::VCF);
		S_EQUAL(text, "VCF");

		text = FileLocationHelper::pathTypeToString(PathType::BAF);
		S_EQUAL(text, "BAF");

		text = FileLocationHelper::pathTypeToString(PathType::CNV_CALLS);
		S_EQUAL(text, "CNV_CALLS");

		text = FileLocationHelper::pathTypeToString(PathType::CNV_ESTIMATES);
		S_EQUAL(text, "CNV_ESTIMATES");

		text = FileLocationHelper::pathTypeToString(PathType::OTHER);
		S_EQUAL(text, "OTHER");
	}

	void bam_getEvidenceFile()
	{
		QString evidence = FileLocationHelper::getEvidenceFile("data_in/panel.bam");
		IS_TRUE(evidence.endsWith("panel_manta_evidence.bam"));

		IS_THROWN(ArgumentException, FileLocationHelper::getEvidenceFile("blbbla"));
	}

};
