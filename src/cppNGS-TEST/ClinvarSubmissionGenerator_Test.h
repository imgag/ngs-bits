#include "TestFramework.h"
#include "ClinvarSubmissionGenerator.h"

TEST_CLASS(ClinvarSubmissionGenerator_Test)
{
Q_OBJECT

private:
	ClinvarSubmissionData minimalData()
	{
		ClinvarSubmissionData data;

		data.local_key = "SomeLocalIdentifierOfTheSubmission";
		data.submitter_first_name = "Max";
		data.submitter_last_name = "Mustermann";
		data.submitter_email = "Max.Mustermann@med.uni-tuebingen.de";
		data.organization_id = "506385";
		data.variant = Variant("chr7",  140453136, 140453136, "T", "A");
		data.variant_classification = "Pathogenic";
		data.variant_inheritance = "Autosomal recessive inheritance";

		data.sample_name = "DX123456_01";

		return data;
	}

private slots:

	void generateXML_minimal()
	{
		ClinvarSubmissionData data = minimalData();

		QString xml = ClinvarSubmissionGenerator::generateXML(data);

		Helper::storeTextFile("out/ClinvarSubmissionData_generateXML_minimal.xml", xml.split("\n"));
		COMPARE_FILES("out/ClinvarSubmissionData_generateXML_minimal.xml", TESTDATA("data_out/ClinvarSubmissionData_generateXML_minimal.xml"));
	}

	void generateXML_full()
	{

		ClinvarSubmissionData data = minimalData();

		data.sample_gender = "female";
		data.sample_disease = "MIM:614381";
		data.sample_phenotypes << Phenotype("HP:0002066", "Gait ataxia");
		data.sample_phenotypes << Phenotype("HP:0001337", "Tremor");

		QString xml = ClinvarSubmissionGenerator::generateXML(data);

		Helper::storeTextFile("out/ClinvarSubmissionData_generateXML_full.xml", xml.split("\n"));
		COMPARE_FILES("out/ClinvarSubmissionData_generateXML_full.xml", TESTDATA("data_out/ClinvarSubmissionData_generateXML_full.xml"));
	}

};
