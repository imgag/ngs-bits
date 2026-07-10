#include "TestFramework.h"
#include "QCCollection.h"
#include "QcRuleMatcher.h"

TEST_CLASS(QcRuleMatcher_Test)
{
	private:

	TEST_METHOD(checkEvaluation)
	{
		IS_THROWN(FileParseException, QcRuleMatcher(TESTDATA("data_in/test_qc_cutoffs_invalid.xml")));
		QcRuleMatcher qc_matcher = QcRuleMatcher(TESTDATA("data_in/test_qc_cutoffs.xml"));

		QString name_short = "system";
		QString sys_type = "WGS";

		// QCs are present, bad quality
		QCCollection qc_data;
		qc_data.insert(QCValue("target region 20x percentage", 20.0, "", "QC:2000027"));
		qc_data.insert(QCValue("target region read depth", 30.0, "", "QC:2000025"));
		QString quality = qc_matcher.evaluate(qc_data, name_short, sys_type, false);
		S_EQUAL(quality, "bad");

		// QCs are present, medium quality
		qc_data.clear();
		qc_data.insert(QCValue("target region 20x percentage", 98.0, "", "QC:2000027"));
		qc_data.insert(QCValue("target region read depth", 30.0, "", "QC:2000025"));
		quality = qc_matcher.evaluate(qc_data, name_short, sys_type, false);
		S_EQUAL(quality, "medium");

		// QCs are present, good quality
		qc_data.clear();
		qc_data.insert(QCValue("target region 20x percentage", 99.0, "", "QC:2000027"));
		qc_data.insert(QCValue("target region read depth", 31.0, "", "QC:2000025"));
		quality = qc_matcher.evaluate(qc_data, name_short, sys_type, false);
		S_EQUAL(quality, "good");

		// One extra QC has been added, this QC is not in the XML rules
		qc_data.clear();
		qc_data.insert(QCValue("target region 20x percentage", 99.0, "", "QC:2000027"));
		qc_data.insert(QCValue("target region read depth", 31.0, "", "QC:2000025"));
		qc_data.insert(QCValue("mapped read percentage", 96.0, "", "QC:2000020"));
		quality = qc_matcher.evaluate(qc_data, name_short, sys_type, false);
		S_EQUAL(quality, "good");

		// One QC is missing from the XML rules
		qc_data.clear();
		qc_data.insert(QCValue("target region 20x percentage", 99.0, "", "QC:2000027"));
		quality = qc_matcher.evaluate(qc_data, name_short, sys_type, false);
		S_EQUAL(quality, "n/a");

		// QCs which are not present in the XML rules
		qc_data.clear();
		qc_data.insert(QCValue("insert size", 363.22, "", "QC:2000023"));
		qc_data.insert(QCValue("duplicate read percentage", 11.66, "", "QC:2000024"));
		quality = qc_matcher.evaluate(qc_data, name_short, sys_type, false);
		S_EQUAL(quality, "n/a");

		// Single QC value - good
		quality = qc_matcher.evaluate("target region 20x percentage", 99.0, name_short, sys_type, false);
		S_EQUAL(quality, "good");

		// Single QC value - medium
		quality = qc_matcher.evaluate("target region 20x percentage", 95.0, name_short, sys_type, false);
		S_EQUAL(quality, "medium");

		// Single QC value - bad
		quality = qc_matcher.evaluate("target region 20x percentage", 89.0, name_short, sys_type, false);
		S_EQUAL(quality, "bad");

		// Single QC value - good ("name short" instead of type)
		name_short = "RPGR-Ex15";
		quality = qc_matcher.evaluate("target region read depth", 1100.0, name_short, sys_type, false);
		S_EQUAL(quality, "good");

		// Single QC value - medium  ("name short" instead of type)
		quality = qc_matcher.evaluate("target region read depth", 450.0, name_short, sys_type, false);
		S_EQUAL(quality, "medium");
	}
};