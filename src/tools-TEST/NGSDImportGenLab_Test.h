#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"
#include "GenLabDB.h"

TEST_CLASS(NGSDImportGenlab_Test)
{
Q_OBJECT
private:
        void tumor_normal_test(NGSD& db, QString sql, QString import, QString expected)
        {
            db.init();
            db.executeQueriesFromFile(sql);

			EXECUTE("NGSDImportGenlab", "-test -ps " + import + " -debug");
            QString s_id = db.sampleId(import);
            SampleData s_data = db.getSampleData(s_id);
            QSet<int> related_samples = db.relatedSamples(s_id.toInt(), "tumor-normal");

            ProcessedSampleData ps_data;
            if (s_data.is_tumor)
            {
                ps_data = db.getProcessedSampleData(db.processedSampleId(import));
                S_EQUAL(ps_data.normal_sample_name, expected);
            }
            else
            {
                ps_data = db.getProcessedSampleData(db.processedSampleId(expected));
                S_EQUAL(ps_data.normal_sample_name, import);
            }

            IS_TRUE(related_samples.contains(db.sampleId(expected).toInt()));

        }

        void same_sample(NGSD& db, QString sql, QString import, QString expected)
        {
            db.init();
            db.executeQueriesFromFile(sql);

			EXECUTE("NGSDImportGenlab", "-test -ps " + import + " -debug");
            QString s_id = db.sampleId(import);
            QSet<int> related_samples = db.relatedSamples(s_id.toInt(), "same sample");

            IS_TRUE(related_samples.contains(db.sampleId(expected).toInt()));
        }

private slots:

	void metadata_import()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGenlab_init1.sql"));

		//test sample 1
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest1_01 -no_relations -no_rna_tissue -debug");

		QString s_id = db.sampleId("DXtest1_01");
		SampleData s_data = db.getSampleData(s_id);
		S_EQUAL(s_data.gender, "male");
		S_EQUAL(s_data.disease_group, "Mental, behavioural or neurodevelopmental disorders");
		S_EQUAL(s_data.disease_status, "Affected");
		S_EQUAL(s_data.patient_identifier, "179158");
		S_EQUAL(s_data.year_of_birth, "2018");

		QList<SampleDiseaseInfo> infos = db.getSampleDiseaseInfo(s_id, "clinical phenotype (free text)");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "Entwicklungsverzoegerung");

		infos = db.getSampleDiseaseInfo(s_id, "Orpha number");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "ORPHA:73223");

		infos = db.getSampleDiseaseInfo(s_id, "ICD10 code");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "F89");

		infos = db.getSampleDiseaseInfo(s_id, "tumor fraction");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "50");

		infos = db.getSampleDiseaseInfo(s_id, "HPO term id");
		I_EQUAL(infos.count(), 2);
		S_EQUAL(infos[0].disease_info, "HP:0000750");
		S_EQUAL(infos[1].disease_info, "HP:0007281");

		//test sample 2
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest2_01 -no_relations -no_rna_tissue -debug");

		s_id = db.sampleId("DXtest2_01");
		s_data = db.getSampleData(s_id);
		S_EQUAL(s_data.gender, "female");
		S_EQUAL(s_data.disease_group, "Diseases of the nervous system");
		S_EQUAL(s_data.disease_status, "Affected");
		S_EQUAL(s_data.patient_identifier, "179159");

		infos = db.getSampleDiseaseInfo(s_id, "clinical phenotype (free text)");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "Ataxie");

		infos = db.getSampleDiseaseInfo(s_id, "Orpha number");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "ORPHA:99");

		infos = db.getSampleDiseaseInfo(s_id, "ICD10 code");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "G11.0");

		infos = db.getSampleDiseaseInfo(s_id, "tumor fraction");
		I_EQUAL(infos.count(), 0);

		infos = db.getSampleDiseaseInfo(s_id, "HPO term id");
		I_EQUAL(infos.count(), 2);
		S_EQUAL(infos[0].disease_info, "HP:0002066");
		S_EQUAL(infos[1].disease_info, "HP:0002070");

		//sample not in genlab -> no import
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest5_01 -no_relations -no_rna_tissue -debug");

		s_id = db.sampleId("DXtest5_01");
		s_data = db.getSampleData(s_id);
		S_EQUAL(s_data.gender, "n/a");
		S_EQUAL(s_data.disease_group, "n/a");
		S_EQUAL(s_data.disease_status, "n/a");
		S_EQUAL(s_data.patient_identifier, "");

		infos = db.getSampleDiseaseInfo(s_id, "clinical phenotype (free text)");
		I_EQUAL(infos.count(), 0);

		infos = db.getSampleDiseaseInfo(s_id, "Orpha number");
		I_EQUAL(infos.count(), 0);

		infos = db.getSampleDiseaseInfo(s_id, "ICD10 code");
		I_EQUAL(infos.count(), 0);

		infos = db.getSampleDiseaseInfo(s_id, "tumor fraction");
		I_EQUAL(infos.count(), 0);

		infos = db.getSampleDiseaseInfo(s_id, "HPO term id");
		I_EQUAL(infos.count(), 0);
	}

	void relations_import()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGenlab_init1.sql"));

		//same sample:
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest4_01 -debug");
		QString s_id = db.sampleId("DXtest4_01");
		QSet<int> related_samples = db.relatedSamples(s_id.toInt(), "same sample");
		I_EQUAL(related_samples.count(), 1);
		IS_TRUE(related_samples.contains(db.sampleId("DXtest2_01").toInt()));

		//tumor-normal simple case:
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGenlab_init1.sql"));
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest1_01 -debug");
		s_id = db.sampleId("DXtest1_01");
		related_samples = db.relatedSamples(s_id.toInt(), "tumor-normal");
		I_EQUAL(related_samples.count(), 1);
		IS_TRUE(related_samples.contains(db.sampleId("DXtest3_01").toInt()));
		ProcessedSampleData ps_data = db.getProcessedSampleData(db.processedSampleId("DXtest3_01"));
		S_EQUAL(ps_data.normal_sample_name, "DXtest1_01");

		related_samples = db.relatedSamples(s_id.toInt(), "siblings");
		I_EQUAL(related_samples.count(), 1);
		IS_TRUE(related_samples.contains(db.sampleId("DXtest2_01").toInt()));


		//tumor-normal simple case reversed:
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGenlab_init1.sql"));

		EXECUTE("NGSDImportGenlab", "-test -ps DXtest3_01 -debug");
		s_id = db.sampleId("DXtest3_01");
		related_samples = db.relatedSamples(s_id.toInt(), "tumor-normal");
		I_EQUAL(related_samples.count(), 1);
		IS_TRUE(related_samples.contains(db.sampleId("DXtest1_01").toInt()));
		ps_data = db.getProcessedSampleData(db.processedSampleId("DXtest3_01"));
		S_EQUAL(ps_data.normal_sample_name, "DXtest1_01");

		//test sample data changes (type - DNA/RNA/cfDNA, tumor status yes no)
		//multiple processed samples - find best one: relevant for normal_id in tumor sample
		tumor_normal_test(db, TESTDATA("data_in/NGSDImportGenlab_init3.sql"), "DXtest1_01", "DXtest3_01");

		tumor_normal_test(db, TESTDATA("data_in/NGSDImportGenlab_init3.sql"), "DXtest3_03", "DXtest1_02");
		tumor_normal_test(db, TESTDATA("data_in/NGSDImportGenlab_init3.sql"), "DXtest1_02", "DXtest3_03");

		tumor_normal_test(db, TESTDATA("data_in/NGSDImportGenlab_init3.sql"), "DXtest3_02", "DXtest1_02");

		tumor_normal_test(db, TESTDATA("data_in/NGSDImportGenlab_init3.sql"), "DXtest3_04", "DXtest1_04");
		tumor_normal_test(db, TESTDATA("data_in/NGSDImportGenlab_init3.sql"), "DXtest1_04", "DXtest3_04");

		//test if relation is already imported / a different relation of same type is already imported

		//same sample
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGenlab_init3.sql"));
		db.getQuery().exec("INSERT INTO sample_relations (sample1_id, relation, sample2_id) VALUES (" + db.sampleId("DXtest4_01") + ",'same sample'," + db.sampleId("DXtest1_01") + ")");


		EXECUTE("NGSDImportGenlab", "-test -ps DXtest4_01 -debug");
		s_id = db.sampleId("DXtest4_01");
		related_samples = db.relatedSamples(s_id.toInt(), "same sample");
		I_EQUAL(related_samples.count(), 1);
		IS_TRUE(related_samples.contains(db.sampleId("DXtest1_01").toInt()));


		//tumor normal:
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGenlab_init3.sql"));
		db.getQuery().exec("INSERT INTO sample_relations (sample1_id, relation, sample2_id) VALUES (" + db.sampleId("DXtest3_03") + ",'tumor-normal'," + db.sampleId("DXtest5_01") + ")");

		EXECUTE("NGSDImportGenlab", "-test -ps DXtest3_03 -debug");
		s_id = db.sampleId("DXtest3_03");
		related_samples = db.relatedSamples(s_id.toInt(), "tumor-normal");
		I_EQUAL(related_samples.count(), 1);
		IS_TRUE(related_samples.contains(db.sampleId("DXtest5_01").toInt()));
		ps_data = db.getProcessedSampleData(db.processedSampleId("DXtest3_03"));
		S_EQUAL(ps_data.normal_sample_name, "");
	}

	void rna_tissue_import()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGenlab_init1.sql"));

		//single HPO term tests
		QString s_id = db.sampleId("DXtest1_01");
		SampleDiseaseInfo hpo;
		hpo.date = QDateTime::currentDateTime();
		hpo.type = "HPO term id";
		hpo.user = "ahtest";
		hpo.disease_info = "HP:0012268";
		db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo});
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest1_01 -no_metadata -no_relations -debug");
		QList<SampleDiseaseInfo> infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "adipose tissue");

		hpo.disease_info = "HP:0100634";
		db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo});
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest1_01 -no_metadata -no_relations -debug");
		infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "lung");

		hpo.disease_info = "HP:0003002";
		db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo});
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest1_01 -no_metadata -no_relations -debug");
		infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "breast");

		hpo.disease_info = "HP:9999999"; // not mappable
		db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo});
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest1_01 -no_metadata -no_relations -debug");
		infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
		I_EQUAL(infos.count(), 0);

		//multiple HPO terms tests:
		//same reference tissue
		SampleDiseaseInfo hpo1 = hpo;
		SampleDiseaseInfo hpo2 = hpo;
		hpo1.disease_info = "HP:0100634"; //lung
		hpo2.disease_info = "HP:0030360"; //lung
		db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo1, hpo2});
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest1_01 -no_metadata -no_relations -debug");
		infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "lung");

		// one with a mapping and one without:
		hpo1.disease_info = "HP:0100634"; //lung
		hpo2.disease_info = "HP:9999999"; //none
		db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo1, hpo2});
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest1_01 -no_metadata -no_relations -debug");
		infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "lung");

		//different reference tissues
		hpo1.disease_info = "HP:0100634"; //lung
		hpo2.disease_info = "HP:0012056"; //skin
		db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo1, hpo2});
		EXECUTE_FAIL("NGSDImportGenlab", "-test -ps DXtest1_01 -no_metadata -no_relations -debug");
	}

	void add_information_to_existing_information()
	{
		if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGenlab_init2.sql"));

		//test sample 1
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest1_01 -no_relations -debug");

		QString s_id = db.sampleId("DXtest1_01");
		SampleData s_data = db.getSampleData(s_id);
		S_EQUAL(s_data.gender, "female");
		S_EQUAL(s_data.disease_group, "Neoplasms");
		S_EQUAL(s_data.disease_status, "Unaffected");
		S_EQUAL(s_data.patient_identifier, "9999999");

		QList<SampleDiseaseInfo> infos = db.getSampleDiseaseInfo(s_id, "clinical phenotype (free text)");
		I_EQUAL(infos.count(), 2);
		S_EQUAL(infos[0].disease_info, "Entwicklungsverzoegerung");
		S_EQUAL(infos[1].disease_info, "Is sick");

		infos = db.getSampleDiseaseInfo(s_id, "Orpha number");
		I_EQUAL(infos.count(), 2);
		S_EQUAL(infos[0].disease_info, "ORPHA:73223");
		S_EQUAL(infos[1].disease_info, "ORPHA:999");

		infos = db.getSampleDiseaseInfo(s_id, "ICD10 code");
		I_EQUAL(infos.count(), 2);
		S_EQUAL(infos[0].disease_info, "F89");
		S_EQUAL(infos[1].disease_info, "G99.9");

		infos = db.getSampleDiseaseInfo(s_id, "tumor fraction");
		I_EQUAL(infos.count(), 2);
		S_EQUAL(infos[0].disease_info, "111");
		S_EQUAL(infos[1].disease_info, "50");

		infos = db.getSampleDiseaseInfo(s_id, "HPO term id");
		I_EQUAL(infos.count(), 3);
		S_EQUAL(infos[0].disease_info, "HP:0000750");
		S_EQUAL(infos[1].disease_info, "HP:0007281");
		S_EQUAL(infos[2].disease_info, "HP:9999999");

		infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "tissue");

		//test sample 2
		EXECUTE("NGSDImportGenlab", "-test -ps DXtest2_01 -no_relations -debug");

		s_id = db.sampleId("DXtest2_01");
		s_data = db.getSampleData(s_id);
		S_EQUAL(s_data.gender, "male");
		S_EQUAL(s_data.disease_group, "Neoplasms");
		S_EQUAL(s_data.disease_status, "Unaffected");
		S_EQUAL(s_data.patient_identifier, "9999999");

		infos = db.getSampleDiseaseInfo(s_id, "clinical phenotype (free text)");
		I_EQUAL(infos.count(),2);
		S_EQUAL(infos[0].disease_info, "Ataxie");
		S_EQUAL(infos[1].disease_info, "Is sick");

		infos = db.getSampleDiseaseInfo(s_id, "Orpha number");
		I_EQUAL(infos.count(), 2);
		S_EQUAL(infos[0].disease_info, "ORPHA:99");
		S_EQUAL(infos[1].disease_info, "ORPHA:999");

		infos = db.getSampleDiseaseInfo(s_id, "ICD10 code");
		I_EQUAL(infos.count(), 2);
		S_EQUAL(infos[0].disease_info, "G11.0");
		S_EQUAL(infos[1].disease_info, "G99.9");

		infos = db.getSampleDiseaseInfo(s_id, "tumor fraction");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "111");

		infos = db.getSampleDiseaseInfo(s_id, "HPO term id");
		I_EQUAL(infos.count(), 3);
		S_EQUAL(infos[0].disease_info, "HP:0002066");
		S_EQUAL(infos[1].disease_info, "HP:0002070");
		S_EQUAL(infos[2].disease_info, "HP:9999999");

		infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
		I_EQUAL(infos.count(), 1);
		S_EQUAL(infos[0].disease_info, "tissue");
	}

};


