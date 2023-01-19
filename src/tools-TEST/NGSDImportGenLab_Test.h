#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"
#include "GenLabDB.h"

TEST_CLASS(NGSDImportGenlab_Test)
{
Q_OBJECT
private slots:
	
        void metadata_import()
	{
            if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

            NGSD db(true);
            db.init();
            db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGenlab_init1.sql"));

            //test sample 1
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest1_01 -no_relations -no_rna_tissue -debug");

            QString s_id = db.sampleId("DXtest1_01");
            SampleData s_data = db.getSampleData(s_id);
            S_EQUAL(s_data.gender, "male");
            S_EQUAL(s_data.disease_group, "Mental, behavioural or neurodevelopmental disorders");
            S_EQUAL(s_data.disease_status, "Affected");
            S_EQUAL(s_data.patient_identifier, "179158");

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
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest2_01 -no_relations -no_rna_tissue -debug");

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
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest5_01 -no_relations -no_rna_tissue -debug");

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
            //TODO when additional test samples exist in GenLab!
            //each type: siblings, same sample, tumor-normal
            //with "distractions" processed samples with different processing systems/RNA/cfDNA...
        }

        void rna_tissue_import()
        {
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
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest1_01 -no_metadata -no_relations -debug");
            QList<SampleDiseaseInfo> infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "adipose tissue");

            hpo.disease_info = "HP:0100634";
            db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo});
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest1_01 -no_metadata -no_relations -debug");
            infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "lung");

            hpo.disease_info = "HP:0003002";
            db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo});
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest1_01 -no_metadata -no_relations -debug");
            infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "breast");

            hpo.disease_info = "HP:9999999"; // not mappable
            db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo});
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest1_01 -no_metadata -no_relations -debug");
            infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
            I_EQUAL(infos.count(), 0);

            //multiple HPO terms tests:
            //same reference tissue
            SampleDiseaseInfo hpo1 = hpo;
            SampleDiseaseInfo hpo2 = hpo;
            hpo1.disease_info = "HP:0100634"; //lung
            hpo2.disease_info = "HP:0100002"; //lung
            db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo1, hpo2});
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest1_01 -no_metadata -no_relations -debug");
            infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "lung");

            // one with a mapping and one without:
            hpo1.disease_info = "HP:0100634"; //lung
            hpo2.disease_info = "HP:9999999"; //none
            db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo1, hpo2});
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest1_01 -no_metadata -no_relations -debug");
            infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "lung");

            //different reference tissues
            hpo1.disease_info = "HP:0100634"; //lung
            hpo2.disease_info = "HP:0012056"; //skin
            db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo1, hpo2});
            EXECUTE_FAIL("NGSDImportGenlab", "-test -ps_id DXtest1_01 -no_metadata -no_relations -debug");

            // with ADD action:
            SampleDiseaseInfo existing_tissue;
            existing_tissue.date = QDateTime::currentDateTime();
            existing_tissue.type = "RNA reference tissue";
            existing_tissue.user = "ahtest";
            existing_tissue.disease_info = "tissue";
            hpo1.disease_info = "HP:0100634"; //lung
            hpo2.disease_info = "HP:0100002"; //lung
            db.setSampleDiseaseInfo(s_id, QList<SampleDiseaseInfo>{hpo1, hpo2, existing_tissue});
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest1_01 -no_metadata -no_relations -action ADD_REPLACE -debug");
            infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
            I_EQUAL(infos.count(), 2);
            S_EQUAL(infos[0].disease_info, "lung");
            S_EQUAL(infos[1].disease_info, "tissue");
        }

        void default_ignore()
        {
            //TODO also test relations
            if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

            NGSD db(true);
            db.init();
            db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGenlab_init2.sql"));

            //test sample 1
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest1_01 -no_relations -debug -action IGNORE");

            QString s_id = db.sampleId("DXtest1_01");
            SampleData s_data = db.getSampleData(s_id);
            S_EQUAL(s_data.gender, "female");
            S_EQUAL(s_data.disease_group, "Neoplasms");
            S_EQUAL(s_data.disease_status, "Unaffected");
            S_EQUAL(s_data.patient_identifier, "9999999");

            QList<SampleDiseaseInfo> infos = db.getSampleDiseaseInfo(s_id, "clinical phenotype (free text)");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "Is sick");

            infos = db.getSampleDiseaseInfo(s_id, "Orpha number");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "ORPHA:999");

            infos = db.getSampleDiseaseInfo(s_id, "ICD10 code");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "G99.9");

            infos = db.getSampleDiseaseInfo(s_id, "tumor fraction");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "111");

            infos = db.getSampleDiseaseInfo(s_id, "HPO term id");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "HP:9999999");

            infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "tissue");

            //test sample 2
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest2_01 -no_relations -debug -action IGNORE");

            s_id = db.sampleId("DXtest2_01");
            s_data = db.getSampleData(s_id);
            S_EQUAL(s_data.gender, "male");
            S_EQUAL(s_data.disease_group, "Neoplasms");
            S_EQUAL(s_data.disease_status, "Unaffected");
            S_EQUAL(s_data.patient_identifier, "9999999");

            infos = db.getSampleDiseaseInfo(s_id, "clinical phenotype (free text)");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "Is sick");

            infos = db.getSampleDiseaseInfo(s_id, "Orpha number");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "ORPHA:999");

            infos = db.getSampleDiseaseInfo(s_id, "ICD10 code");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "G99.9");

            infos = db.getSampleDiseaseInfo(s_id, "tumor fraction");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "111");

            infos = db.getSampleDiseaseInfo(s_id, "HPO term id");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "HP:9999999");

            infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "tissue");

        }

        void default_add_replace()
        {
            //TODO also test relations
            if (!GenLabDB::isAvailable()) SKIP("Test needs access to the GenLab Database!");

            NGSD db(true);
            db.init();
            db.executeQueriesFromFile(TESTDATA("data_in/NGSDImportGenlab_init2.sql"));

            //test sample 1
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest1_01 -no_relations -debug -action ADD_REPLACE");

            QString s_id = db.sampleId("DXtest1_01");
            SampleData s_data = db.getSampleData(s_id);
            S_EQUAL(s_data.gender, "male");
            S_EQUAL(s_data.disease_group, "Mental, behavioural or neurodevelopmental disorders");
            S_EQUAL(s_data.disease_status, "Affected");
            S_EQUAL(s_data.patient_identifier, "179158");

            QList<SampleDiseaseInfo> infos = db.getSampleDiseaseInfo(s_id, "clinical phenotype (free text)");
            I_EQUAL(infos.count(), 2);
            S_EQUAL(infos[1].disease_info, "Is sick");
            S_EQUAL(infos[0].disease_info, "Entwicklungsverzoegerung");

            infos = db.getSampleDiseaseInfo(s_id, "Orpha number");
            I_EQUAL(infos.count(), 2);
            S_EQUAL(infos[1].disease_info, "ORPHA:999");
            S_EQUAL(infos[0].disease_info, "ORPHA:73223");

            infos = db.getSampleDiseaseInfo(s_id, "ICD10 code");
            I_EQUAL(infos.count(), 2);
            S_EQUAL(infos[1].disease_info, "G99.9");
            S_EQUAL(infos[0].disease_info, "F89");

            infos = db.getSampleDiseaseInfo(s_id, "tumor fraction");
            I_EQUAL(infos.count(), 2);
            S_EQUAL(infos[0].disease_info, "111");
            S_EQUAL(infos[1].disease_info, "50");

            infos = db.getSampleDiseaseInfo(s_id, "HPO term id");
            I_EQUAL(infos.count(), 3);
            S_EQUAL(infos[2].disease_info, "HP:9999999");
            S_EQUAL(infos[0].disease_info, "HP:0000750");
            S_EQUAL(infos[1].disease_info, "HP:0007281");

            infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue"); // No mappable HPO terms
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "tissue");

            //test sample 2
            EXECUTE("NGSDImportGenlab", "-test -ps_id DXtest2_01 -no_relations -debug -action ADD_REPLACE");

            s_id = db.sampleId("DXtest2_01");
            s_data = db.getSampleData(s_id);
            S_EQUAL(s_data.gender, "female");
            S_EQUAL(s_data.disease_group, "Diseases of the nervous system");
            S_EQUAL(s_data.disease_status, "Affected");
            S_EQUAL(s_data.patient_identifier, "179159");

            infos = db.getSampleDiseaseInfo(s_id, "clinical phenotype (free text)");
            I_EQUAL(infos.count(), 2);
            S_EQUAL(infos[1].disease_info, "Is sick");
            S_EQUAL(infos[0].disease_info, "Ataxie");

            infos = db.getSampleDiseaseInfo(s_id, "Orpha number");
            I_EQUAL(infos.count(), 2);
            S_EQUAL(infos[1].disease_info, "ORPHA:999");
            S_EQUAL(infos[0].disease_info, "ORPHA:99");

            infos = db.getSampleDiseaseInfo(s_id, "ICD10 code");
            I_EQUAL(infos.count(), 2);
            S_EQUAL(infos[1].disease_info, "G99.9");
            S_EQUAL(infos[0].disease_info, "G11.0");

            infos = db.getSampleDiseaseInfo(s_id, "tumor fraction");
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "111");

            infos = db.getSampleDiseaseInfo(s_id, "HPO term id");
            I_EQUAL(infos.count(), 3);
            S_EQUAL(infos[2].disease_info, "HP:9999999");
            S_EQUAL(infos[0].disease_info, "HP:0002066");
            S_EQUAL(infos[1].disease_info, "HP:0002070");

            infos = db.getSampleDiseaseInfo(s_id, "RNA reference tissue"); // No mappable HPO terms
            I_EQUAL(infos.count(), 1);
            S_EQUAL(infos[0].disease_info, "tissue");
        }

};


