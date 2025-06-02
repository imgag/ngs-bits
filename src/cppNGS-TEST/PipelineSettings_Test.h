#include "TestFramework.h"
#include "PipelineSettings.h"


TEST_CLASS(PipelineSettings_Test)
{
Q_OBJECT
private slots:

	void loadSettings()
	{
		//load
		IS_FALSE(PipelineSettings::isInitialized());
		PipelineSettings::loadSettings(TESTDATA("data_in/megSAP_settings.ini"));
		IS_TRUE(PipelineSettings::isInitialized());

		//check values
		IS_FALSE(PipelineSettings::rootDir().isEmpty());
		S_EQUAL(PipelineSettings::projectFolder("diagnostic"), "/mnt/storage4/projects/diagnostic/");
		S_EQUAL(PipelineSettings::projectFolder("research"), "/mnt/storage4/projects/research/");
		S_EQUAL(PipelineSettings::projectFolder("test"), "/mnt/storage4/projects/test/");
		S_EQUAL(PipelineSettings::projectFolder("external"), "/mnt/storage4/projects/external/");

		S_EQUAL(PipelineSettings::dataFolder(), "/mnt/storage4/megSAP/data/");
		IS_TRUE(PipelineSettings::queuesDefault().contains("default_srv010"));
		IS_TRUE(PipelineSettings::queuesResearch().contains("research_srv011"));
		IS_TRUE(PipelineSettings::queuesHighPriority().contains("priority_srv010"));
		IS_TRUE(PipelineSettings::queuesHighMemory().contains("highmem_srv010"));
		S_EQUAL(PipelineSettings::queuesDragen().at(0), "dragen_srv016");
	}
};
