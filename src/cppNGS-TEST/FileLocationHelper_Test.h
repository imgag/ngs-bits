#include "TestFramework.h"
#include "FileLocationHelper.h"

TEST_CLASS(FileLocationHelper_Test)
{
Q_OBJECT
private slots:
	void convert_pathTypeToString()
	{
		S_EQUAL(FileLocation::typeToString(PathType::PROJECT_FOLDER), "PROJECT_FOLDER");

		S_EQUAL(FileLocation::typeToString(PathType::SAMPLE_FOLDER), "SAMPLE_FOLDER");

		S_EQUAL(FileLocation::typeToString(PathType::BAM), "BAM");

		S_EQUAL(FileLocation::typeToString(PathType::GSVAR), "GSVAR");

		S_EQUAL(FileLocation::typeToString(PathType::VCF), "VCF");

		S_EQUAL(FileLocation::typeToString(PathType::BAF), "BAF");

		S_EQUAL(FileLocation::typeToString(PathType::COPY_NUMBER_CALLS), "COPY_NUMBER_CALLS");

		S_EQUAL(FileLocation::typeToString(PathType::COPY_NUMBER_RAW_DATA), "COPY_NUMBER_RAW_DATA");

		S_EQUAL(FileLocation::typeToString(PathType::OTHER), "OTHER");
	}

	void bam_getEvidenceFile()
	{
		QString evidence = FileLocationHelper::getEvidenceFile("data_in/panel.bam");
		IS_TRUE(evidence.endsWith("panel_manta_evidence.bam"));

		IS_THROWN(ArgumentException, FileLocationHelper::getEvidenceFile("blbbla"));
	}

};
