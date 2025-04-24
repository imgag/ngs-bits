#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"
#include "LoginManager.h"
#include "SomaticXmlReportGenerator.h"
#include "SomaticReportSettings.h"
#include "SomaticReportHelper.h"
#include "SomaticVariantInterpreter.h"
#include "GermlineReportGenerator.h"
#include "TumorOnlyReportWorker.h"
#include "StatisticsServiceLocal.h"
#include "FileLocationProviderLocal.h"
#include "VariantHgvsAnnotator.h"
#include "OntologyTermCollection.h"
#include "RepeatLocusList.h"
#include <QThread>

TEST_CLASS(NGSD_Test)
{
Q_OBJECT
private slots:

	//Normally, one member is tested in one QT slot.
	//Because initializing the database takes very long, all NGSD functionality is tested in one slot.
	void main_tests()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in1.sql"));

		//isOpen
		IS_TRUE(db.isOpen());

		//isProductionDb
		IS_FALSE(db.isProductionDb());

		//log in user
		LoginManager::login("ahmustm1", "", true);

		//escapeText
		S_EQUAL(db.escapeText("; '"), "'; '''");

		//tableExists
		IS_TRUE(db.tableExists("user", false));
		IS_FALSE(db.tableExists("user_missing", false));

		//tableEmpty
		IS_FALSE(db.tableEmpty("user"));
		IS_TRUE(db.tableEmpty("gaps"));

		//rowExists
		IS_TRUE(db.rowExists("user", 99)); //ahmustm1
		IS_TRUE(db.rowExists("user", 101)); //ahkerra1
		IS_FALSE(db.rowExists("user", 666));

		//getEnum
		QStringList enum_values = db.getEnum("sample", "disease_group");
		S_EQUAL(enum_values.join(", "), "n/a, Neoplasms, Diseases of the blood or blood-forming organs, Diseases of the immune system, Endocrine, nutritional or metabolic diseases, Mental, behavioural or neurodevelopmental disorders, Sleep-wake disorders, Diseases of the nervous system, Diseases of the visual system, Diseases of the ear or mastoid process, Diseases of the circulatory system, Diseases of the respiratory system, Diseases of the digestive system, Diseases of the skin, Diseases of the musculoskeletal system or connective tissue, Diseases of the genitourinary system, Developmental anomalies, Other diseases");
		I_EQUAL(enum_values.count(), 18);
		S_EQUAL(enum_values[4], "Endocrine, nutritional or metabolic diseases");
		enum_values = db.getEnum("sample_disease_info", "type");
		S_EQUAL(enum_values.join(", "), "HPO term id, ICD10 code, OMIM disease/phenotype identifier, Orpha number, CGI cancer type, tumor fraction, age of onset, clinical phenotype (free text), RNA reference tissue, Oncotree code")
		I_EQUAL(enum_values.count(), 10);

		//getEnum on Set type:
		//setInfo:
		enum_values = db.getEnum("somatic_report_configuration", "quality");
		I_EQUAL(enum_values.count(), 6);
		S_EQUAL(enum_values.join(", "), "no abnormalities, tumor cell content too low, quality of tumor DNA too low, DNA quantity too low, heterogeneous sample, contamination");

		//processedSampleName
		QString ps_name = db.processedSampleName(db.processedSampleId("NA12878_03"), false);
		S_EQUAL(ps_name, "NA12878_03");

		//processingSystemIdFromSample
		int sys_id = db.processingSystemIdFromProcessedSample(ps_name);
        I_EQUAL(sys_id, 1);

		//getProcessingSystemData
		ProcessingSystemData system_data = db.getProcessingSystemData(sys_id);
		S_EQUAL(system_data.name, "HaloPlex HBOC v5");
		S_EQUAL(system_data.name_short, "hpHBOCv5");
		S_EQUAL(system_data.adapter1_p5, "AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC");
		S_EQUAL(system_data.adapter2_p7, "AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT");
		S_EQUAL(system_data.type, "Panel Haloplex");
		IS_FALSE(system_data.shotgun);
		S_EQUAL(system_data.umi_type, "n/a");
		S_EQUAL(system_data.genome, "GRCh37");

		//normalSample
		S_EQUAL(db.normalSample(db.processedSampleId("NA12345_01")), "NA12878_03")

		//nextProcessingId
		S_EQUAL(db.nextProcessingId(db.sampleId("NA12878")), "5");

		//processedSamplePath
		QString gsvar_path = db.processedSamplePath(db.processedSampleId("NA12878_03"), PathType::GSVAR);
		IS_TRUE(gsvar_path.endsWith("test/KontrollDNACoriell/Sample_NA12878_03/NA12878_03.GSvar"));

		//geneToApproved
		QByteArray gene_app = db.geneToApproved("BRCA1");
		S_EQUAL(gene_app, "BRCA1");
		gene_app = db.geneToApproved("BLABLA");
		S_EQUAL(gene_app, "");
		gene_app = db.geneToApproved("BLABLA", true);
		S_EQUAL(gene_app, "BLABLA");
		gene_app = db.geneToApproved("BLABLA2", true);
		S_EQUAL(gene_app, "BLABLA2");

		//geneToApprovedWithMessage
		auto gene_app2 = db.geneToApprovedWithMessage("BRCA1");
		S_EQUAL(gene_app2.first, "BRCA1");
		S_EQUAL(gene_app2.second, "KEPT: BRCA1 is an approved symbol");
		gene_app2 = db.geneToApprovedWithMessage("BLABLA");
		S_EQUAL(gene_app2.first, "BLABLA");
		S_EQUAL(gene_app2.second, "ERROR: BLABLA is unknown symbol");
		gene_app2 = db.geneToApprovedWithMessage("COX2");
		S_EQUAL(gene_app2.first, "COX2");
		S_EQUAL(gene_app2.second, "ERROR: COX2 is a synonymous symbol of the genes MT-CO2, PTGS2");
		gene_app2 = db.geneToApprovedWithMessage("QARS");
		S_EQUAL(gene_app2.first, "QARS");
		S_EQUAL(gene_app2.second, "ERROR: QARS is a previous symbol of the genes EPRS, QARS1");

		//geneToApprovedWithMessageAndAmbiguous
		auto gene_app3 = db.geneToApprovedWithMessageAndAmbiguous("BRCA1");
		I_EQUAL(gene_app3.count(), 1);
		S_EQUAL(gene_app3[0].first, "BRCA1");
		S_EQUAL(gene_app3[0].second, "KEPT: BRCA1 is an approved symbol");
		gene_app3 = db.geneToApprovedWithMessageAndAmbiguous("BLABLA");
		I_EQUAL(gene_app3.count(), 1);
		S_EQUAL(gene_app3[0].first, "BLABLA");
		S_EQUAL(gene_app3[0].second, "ERROR: BLABLA is an unknown symbol");
		gene_app3 = db.geneToApprovedWithMessageAndAmbiguous("COX2");
		I_EQUAL(gene_app3.count(), 2);
		S_EQUAL(gene_app3[0].first, "MT-CO2");
		S_EQUAL(gene_app3[0].second, "REPLACED: COX2 is a synonymous symbol");
		S_EQUAL(gene_app3[1].first, "PTGS2");
		S_EQUAL(gene_app3[1].second, "REPLACED: COX2 is a synonymous symbol");
		gene_app3 = db.geneToApprovedWithMessageAndAmbiguous("QARS");
		I_EQUAL(gene_app3.count(), 2);
		S_EQUAL(gene_app3[0].first, "EPRS");
		S_EQUAL(gene_app3[0].second, "REPLACED: QARS is a previous symbol");
		S_EQUAL(gene_app3[1].first, "QARS1");
		S_EQUAL(gene_app3[1].second, "REPLACED: QARS is a previous symbol");

		//geneID
		int gene_app_id = db.geneId("BRCA1");
		I_EQUAL(gene_app_id, 1);
		gene_app_id = db.geneId("BLABLA");
		I_EQUAL(gene_app_id, -1);

		//genesOverlapping
		GeneSet genes = db.genesOverlapping("chr13", 90, 95, 0); //nearby left
		I_EQUAL(genes.count(), 0);
		genes = db.genesOverlapping("chr13", 205, 210, 0); //nearby right
		I_EQUAL(genes.count(), 0);
		genes = db.genesOverlapping("chr13", 100, 200, 0); //overlapping exons
		I_EQUAL(genes.count(), 1);
		S_EQUAL(genes[0], "BRCA2");
		genes = db.genesOverlapping("chr13", 140, 160, 0); //overlapping intron
		I_EQUAL(genes.count(), 1);
		S_EQUAL(genes[0], "BRCA2");
		genes = db.genesOverlapping("chr13", 90, 95, 6); //extend left
		I_EQUAL(genes.count(), 1);
		S_EQUAL(genes[0], "BRCA2");
		genes = db.genesOverlapping("chr13", 205, 210, 6); //extend right
		I_EQUAL(genes.count(), 1);
		S_EQUAL(genes[0], "BRCA2");
		genes = db.genesOverlapping("chr22", 80, 110, 0); //non-coding
		I_EQUAL(genes.count(), 1);
		S_EQUAL(genes[0], "NON-CODING");

		//genesOverlappingByExon
		genes = db.genesOverlappingByExon("chr13", 90, 95, 0); //nearby left
		I_EQUAL(genes.count(), 0);
		genes = db.genesOverlappingByExon("chr13", 205, 210, 0); //nearby right
		I_EQUAL(genes.count(), 0);
		genes = db.genesOverlappingByExon("chr13", 100, 200, 0); //overlapping exons
		I_EQUAL(genes.count(), 1);
		S_EQUAL(genes[0], "BRCA2");
		genes = db.genesOverlappingByExon("chr13", 140, 160, 0); //overlapping intron
		I_EQUAL(genes.count(), 0);
		genes = db.genesOverlappingByExon("chr13", 90, 95, 6); //extend left
		I_EQUAL(genes.count(), 1);
		S_EQUAL(genes[0], "BRCA2");
		genes = db.genesOverlappingByExon("chr13", 205, 210, 6); //extend right
		I_EQUAL(genes.count(), 1);
		S_EQUAL(genes[0], "BRCA2");
		genes = db.genesOverlappingByExon("chr22", 110, 190, 0); //non-coding
		I_EQUAL(genes.count(), 1);
		S_EQUAL(genes[0], "NON-CODING");

		//getSampleData
		QString sample_id = db.sampleId("NA12878");
		SampleData sample_data = db.getSampleData(sample_id);
		S_EQUAL(sample_data.name, "NA12878");
		S_EQUAL(sample_data.name_external, "ex1");
		S_EQUAL(sample_data.patient_identifier, "pat1");
		S_EQUAL(sample_data.year_of_birth, "1977");
		S_EQUAL(sample_data.quality, "good");
		S_EQUAL(sample_data.comments, "comment_s1");
		S_EQUAL(sample_data.disease_group, "Diseases of the blood or blood-forming organs");
		S_EQUAL(sample_data.disease_status, "Unaffected");
		I_EQUAL(sample_data.phenotypes.count(), 0);
		IS_FALSE(sample_data.is_tumor);
		IS_FALSE(sample_data.is_ffpe);
		//second sample (tumor)
		sample_id = db.sampleId("NA12345_01");
		sample_data = db.getSampleData(sample_id);
		S_EQUAL(sample_data.name, "NA12345");
		S_EQUAL(sample_data.name_external, "ex3");
		S_EQUAL(sample_data.patient_identifier, "pat3");
		S_EQUAL(sample_data.year_of_birth, "");
		S_EQUAL(sample_data.quality, "bad");
		S_EQUAL(sample_data.comments, "comment_s3");
		S_EQUAL(sample_data.disease_group, "Diseases of the immune system");
		S_EQUAL(sample_data.disease_status, "Affected");
		I_EQUAL(sample_data.phenotypes.count(), 1);
		S_EQUAL(sample_data.phenotypes[0].toString(), "HP:0001251 - Ataxia");
		IS_TRUE(sample_data.is_tumor);
		IS_TRUE(sample_data.is_ffpe);

		//getProcessedSampleData
		QString processed_sample_id = db.processedSampleId("NA12878_03");
		ProcessedSampleData processed_sample_data = db.getProcessedSampleData(processed_sample_id);
		S_EQUAL(processed_sample_data.name, "NA12878_03");
		S_EQUAL(processed_sample_data.quality, "medium");
		S_EQUAL(processed_sample_data.gender, "female");
		S_EQUAL(processed_sample_data.comments, "comment_ps1");
		S_EQUAL(processed_sample_data.project_name, "KontrollDNACoriell");
		S_EQUAL(processed_sample_data.project_type, "test");
		S_EQUAL(processed_sample_data.run_name, "#00372");
		S_EQUAL(processed_sample_data.normal_sample_name, "");
		S_EQUAL(processed_sample_data.processing_system, "HaloPlex HBOC v5");
		S_EQUAL(processed_sample_data.processing_system_type, "Panel Haloplex");
		S_EQUAL(processed_sample_data.processing_modus, "manual");
		S_EQUAL(processed_sample_data.batch_number, "batch 17");
		//second sample (tumor)
		processed_sample_id = db.processedSampleId("NA12345_01");
		processed_sample_data = db.getProcessedSampleData(processed_sample_id);
		S_EQUAL(processed_sample_data.name, "NA12345_01");
		S_EQUAL(processed_sample_data.quality, "good");
		S_EQUAL(processed_sample_data.gender, "male");
		S_EQUAL(processed_sample_data.comments, "comment_ps4");
		S_EQUAL(processed_sample_data.project_name, "KontrollDNACoriell");
		S_EQUAL(processed_sample_data.run_name, "#00372");
		S_EQUAL(processed_sample_data.normal_sample_name, "NA12878_03");
		S_EQUAL(processed_sample_data.processing_modus, "n/a");
		S_EQUAL(processed_sample_data.batch_number, "");

		//genesToRegions
		QString messages;
		QTextStream stream(&messages);
		BedFile regions;

		messages.clear();
		regions = db.genesToRegions(GeneSet() << "BRCA1", Transcript::CCDS, "gene", false, false, &stream); //gene mode, hit
		I_EQUAL(regions.count(), 1);
		S_EQUAL(regions[0].annotations()[0], "BRCA1");
		I_EQUAL(regions.baseCount(), 101);
		IS_TRUE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(GeneSet() << "NIPA1", Transcript::ENSEMBL, "gene", false, true, &stream); //gene mode, two hits, annotate_transcripts
		I_EQUAL(regions.count(), 2);
		S_EQUAL(regions[0].annotations()[0], "NIPA1 NIPA1_TR2.5");
		S_EQUAL(regions[1].annotations()[0], "NIPA1 NIPA1_TR1.4");
		I_EQUAL(regions.baseCount(), 642);
		regions.merge(); //overlapping regions
		I_EQUAL(regions.count(), 1);
		I_EQUAL(regions.baseCount(), 341);
		IS_TRUE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(GeneSet() << "NIPA1", Transcript::CCDS, "gene", false, false, &stream); //gene mode, no hit
		I_EQUAL(regions.baseCount(), 0);
		IS_FALSE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(GeneSet() << "NIPA1", Transcript::CCDS, "gene", true, false, &stream); //gene mode, no hit, fallback
		I_EQUAL(regions.count(), 2);
		I_EQUAL(regions.baseCount(), 642);
		regions.merge(); //overlapping regions
		I_EQUAL(regions.count(), 1);
		I_EQUAL(regions.baseCount(), 341);
		IS_TRUE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(GeneSet() << "BRCA1", Transcript::CCDS, "exon", false, false, &stream); //exon mode, hit
		I_EQUAL(regions.count(), 4);
		S_EQUAL(regions[0].annotations()[0], "BRCA1");
		I_EQUAL(regions.baseCount(), 44);
		IS_TRUE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(GeneSet() << "NIPA1", Transcript::CCDS, "exon", false, false, &stream); //exon mode, no hit
		I_EQUAL(regions.baseCount(), 0);
		IS_FALSE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(GeneSet() << "NIPA1", Transcript::CCDS, "exon", true, false, &stream); //exon mode, no hit, fallback
		I_EQUAL(regions.count(), 4);
		I_EQUAL(regions.baseCount(), 304);
		regions.merge(); //overlapping regions
		I_EQUAL(regions.count(), 2);
		I_EQUAL(regions.baseCount(), 202);
		IS_TRUE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(GeneSet() << "NIPA1", Transcript::ENSEMBL, "exon", false, true, &stream); //exon mode, two hits, annotate_transcripts
		I_EQUAL(regions.count(), 4);
		S_EQUAL(regions[0].annotations()[0], "NIPA1 NIPA1_TR1.4");
		S_EQUAL(regions[1].annotations()[0], "NIPA1 NIPA1_TR2.5");
		S_EQUAL(regions[2].annotations()[0], "NIPA1 NIPA1_TR2.5");
		S_EQUAL(regions[3].annotations()[0], "NIPA1 NIPA1_TR1.4");
		regions.merge(); //overlapping regions
		I_EQUAL(regions.count(), 2);
		I_EQUAL(regions.baseCount(), 202);
		IS_TRUE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(GeneSet() << "NON-CODING", Transcript::ENSEMBL, "exon", false, true, &stream); //exon mode, non-coding, annotate_transcripts
		I_EQUAL(regions.count(), 2);
		S_EQUAL(regions[0].annotations()[0], "NON-CODING NON-CODING_TR1.6");
		S_EQUAL(regions[1].annotations()[0], "NON-CODING NON-CODING_TR1.6");
		I_EQUAL(regions.count(), 2);
		I_EQUAL(regions.baseCount(), 202);
		IS_TRUE(messages.isEmpty());

		//transcriptId
		I_EQUAL(db.transcriptId("NIPA1_TR2"), 4);
		I_EQUAL(db.transcriptId("NIPA1_TR2_FAIL", false), -1);
		I_EQUAL(db.transcriptId("NIPA1_TR2.3", false), 4);

		//transcript
		Transcript transcript = db.transcript(4);
		S_EQUAL(transcript.name(), "NIPA1_TR2");
		I_EQUAL(transcript.strand(), Transcript::MINUS);
		I_EQUAL(transcript.source(), Transcript::ENSEMBL);
		I_EQUAL(transcript.regions().count(), 4);
		I_EQUAL(transcript.regions().baseCount(), 224);
		I_EQUAL(transcript.codingRegions().count(), 2);
		I_EQUAL(transcript.codingRegions().baseCount(), 102);

		//transcripts
		TranscriptList transcripts = db.transcripts(1, Transcript::CCDS, true); //BRCA1, CCDS, coding
		I_EQUAL(transcripts.count(), 1);
		S_EQUAL(transcripts[0].gene(), "BRCA1");
		S_EQUAL(transcripts[0].name(), "BRCA1_TR1");
		I_EQUAL(transcripts[0].strand(), Transcript::PLUS);
		I_EQUAL(transcripts[0].source(), Transcript::CCDS);
		I_EQUAL(transcripts[0].regions().count(), 4);
		I_EQUAL(transcripts[0].regions().baseCount(), 44);
		I_EQUAL(transcripts[0].codingRegions().count(), 4);
		I_EQUAL(transcripts[0].codingRegions().baseCount(), 44);
		IS_TRUE(transcripts[0].isGencodeBasicTranscript());
		IS_FALSE(transcripts[0].isEnsemblCanonicalTranscript());
		IS_TRUE(transcripts[0].isManeSelectTranscript());
		IS_FALSE(transcripts[0].isManePlusClinicalTranscript());

		transcripts = db.transcripts(3, Transcript::ENSEMBL, true); //NIPA1, Ensembl, coding
		I_EQUAL(transcripts.count(), 2);
		S_EQUAL(transcripts[0].name(), "NIPA1_TR2");
		I_EQUAL(transcripts[0].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[0].source(), Transcript::ENSEMBL);
		I_EQUAL(transcripts[0].regions().count(), 4);
		I_EQUAL(transcripts[0].regions().baseCount(), 224);
		I_EQUAL(transcripts[0].codingRegions().count(), 2);
		I_EQUAL(transcripts[0].codingRegions().baseCount(), 102);
		IS_FALSE(transcripts[0].isGencodeBasicTranscript());
		IS_TRUE(transcripts[0].isEnsemblCanonicalTranscript());
		IS_FALSE(transcripts[0].isManeSelectTranscript());
		IS_FALSE(transcripts[0].isManePlusClinicalTranscript());
		S_EQUAL(transcripts[1].gene(), "NIPA1");
		S_EQUAL(transcripts[1].name(), "NIPA1_TR1");
		I_EQUAL(transcripts[1].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[1].source(), Transcript::ENSEMBL);
		I_EQUAL(transcripts[1].regions().count(), 2);
		I_EQUAL(transcripts[1].regions().baseCount(), 202);
		I_EQUAL(transcripts[1].codingRegions().count(), 2);
		I_EQUAL(transcripts[1].codingRegions().baseCount(), 202);
		IS_FALSE(transcripts[1].isGencodeBasicTranscript());
		IS_FALSE(transcripts[1].isEnsemblCanonicalTranscript());
		IS_TRUE(transcripts[1].isManeSelectTranscript());
		IS_TRUE(transcripts[1].isManePlusClinicalTranscript());

		transcripts = db.transcripts(3, Transcript::ENSEMBL, false); //NIPA1, Ensembl, non-coding
		I_EQUAL(transcripts.count(), 2);
		S_EQUAL(transcripts[0].name(), "NIPA1_TR2");
		I_EQUAL(transcripts[0].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[0].source(), Transcript::ENSEMBL);
		I_EQUAL(transcripts[0].regions().count(), 4);
		I_EQUAL(transcripts[0].regions().baseCount(), 224);
		I_EQUAL(transcripts[0].codingRegions().count(), 2);
		I_EQUAL(transcripts[0].codingRegions().baseCount(), 102);
		S_EQUAL(transcripts[1].name(), "NIPA1_TR1");
		I_EQUAL(transcripts[1].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[1].source(), Transcript::ENSEMBL);
		I_EQUAL(transcripts[1].regions().count(), 2);
		I_EQUAL(transcripts[1].regions().baseCount(), 202);
		I_EQUAL(transcripts[1].codingRegions().count(), 2);
		I_EQUAL(transcripts[1].codingRegions().baseCount(), 202);

		transcripts = db.transcripts(4, Transcript::ENSEMBL, true); //NON-CODING, Ensembl, coding
		I_EQUAL(transcripts.count(), 0);

		transcripts = db.transcripts(4, Transcript::ENSEMBL, false); //NON-CODING, Ensembl, non-coding
		I_EQUAL(transcripts.count(), 1);
		S_EQUAL(transcripts[0].name(), "NON-CODING_TR1");
		I_EQUAL(transcripts[0].regions().count(), 2);
		I_EQUAL(transcripts[0].regions().baseCount(), 202);
		I_EQUAL(transcripts[0].codingRegions().count(), 0);
		I_EQUAL(transcripts[0].codingRegions().baseCount(), 0);

		//transcriptsOverlapping
		transcripts = db.transcriptsOverlapping("chr15", 70, 70, 0); //nearby left of NIPA1
		I_EQUAL(transcripts.count(), 0);
		transcripts = db.transcriptsOverlapping("chr15", 425, 425, 0); //nearby right of NIPA1
		I_EQUAL(transcripts.count(), 0);
		transcripts = db.transcriptsOverlapping("chr15", 95, 95, 0); //overlapping only NIPA1_TR2
		I_EQUAL(transcripts.count(), 1);
		S_EQUAL(transcripts[0].name(), "NIPA1_TR2");
		transcripts = db.transcriptsOverlapping("chr15", 95, 95, 10); //overlapping both because of extension
		I_EQUAL(transcripts.count(), 2);
		S_EQUAL(transcripts[0].name(), "NIPA1_TR2");
		S_EQUAL(transcripts[1].name(), "NIPA1_TR1");

		//longestCodingTranscript
		transcript = db.longestCodingTranscript(4, Transcript::ENSEMBL); //NON-CODING, zero CCDS transcripts
		IS_FALSE(transcript.isValid());

		transcript = db.longestCodingTranscript(1, Transcript::CCDS); //BRCA1, one CCDS transcript
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "BRCA1_TR1");
		I_EQUAL(transcript.regions().count(), 4);
		I_EQUAL(transcript.regions().baseCount(), 44);
		I_EQUAL(transcript.codingRegions().count(), 4);
		I_EQUAL(transcript.codingRegions().baseCount(), 44);

		transcript = db.longestCodingTranscript(3, Transcript::ENSEMBL); //NIPA1, two Ensembl transcripts
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "NIPA1_TR1");
		I_EQUAL(transcript.regions().count(), 2);
		I_EQUAL(transcript.regions().baseCount(), 202);
		I_EQUAL(transcript.codingRegions().count(), 2);
		I_EQUAL(transcript.codingRegions().baseCount(), 202);

		//bestTranscript
		transcript = db.bestTranscript(4); //NON-CODING, one non-coding Ensembl transcript
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "NON-CODING_TR1");

		transcript = db.bestTranscript(3); //NIPA1, two coding Ensembl transcripts
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "NIPA1_TR1");

		transcript = db.bestTranscript(652410); //SPG7, two coding Ensembl transcripts (shorter one is preferred)
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "ENST00000341316");

		transcript = db.bestTranscript(1); //BRCA1, only CCDS transcript > invalid
		IS_FALSE(transcript.isValid());

		transcript = db.bestTranscript(415153); // EPRS 3 transcripts: 2 mane+clinical, 1 gencode-basic
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "EPRS_TR2");

		transcript = db.bestTranscript(427667); // MT-CO2 4 transcripts: 3 mane+clinical (2-4), 1 normal (1), 2 of mane+clinical as preffered (3+4)
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "MT-CO2_TR3");

		//bestTranscript with impact:
		VariantTranscript t1;
		t1.id = "EPRS_TR2.1";
		t1.impact = VariantImpact::LOW;

		VariantTranscript t2;
		t2.id = "EPRS_TR3.1";
		t2.impact = VariantImpact::HIGH;

		QList<VariantTranscript> var_trans;
		var_trans.append(t1);
		var_trans.append(t2);

		transcript = db.bestTranscript(415153, var_trans);
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "EPRS_TR3");

		t1.id = "MT-CO2_TR3.1";
		t1.impact = VariantImpact::LOW;

		t2.id = "MT-CO2_TR4.1";
		t2.impact = VariantImpact::MODERATE;

		VariantTranscript t3;
		t3.id = "MT-CO2_TR2.1"; //not preferred
		t3.impact = VariantImpact::HIGH;

		var_trans.clear();
		var_trans.append(t1);
		var_trans.append(t2);
		var_trans.append(t3);

		transcript = db.bestTranscript(427667, var_trans);
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "MT-CO2_TR4");

		t1.id = "MT-CO2_TR3.1";
		t1.impact = VariantImpact::LOW;

		t2.id = "MT-CO2_TR4.1";
		t2.impact = VariantImpact::LOW;

		t3.id = "MT-CO2_TR2.1"; //not preferred
		t3.impact = VariantImpact::HIGH;

		var_trans.clear();
		var_trans.append(t1);
		var_trans.append(t2);
		var_trans.append(t3);

		transcript = db.bestTranscript(427667, var_trans);
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "MT-CO2_TR3");


		//relevantTranscripts
		transcripts = db.relevantTranscripts(3); //NIPA1 (only best)
		I_EQUAL(transcripts.count(), 2);
		S_EQUAL(transcripts[0].name(), "NIPA1_TR1");
		S_EQUAL(transcripts[1].name(), "NIPA1_TR2");
		transcripts = db.relevantTranscripts(652410); //SPG7 (best plus MANE select)
		I_EQUAL(transcripts.count(), 2);
		S_EQUAL(transcripts[0].name(), "ENST00000341316");
		S_EQUAL(transcripts[1].name(), "ENST00000268704");
		transcripts = db.relevantTranscripts(1); //BRCA1 (only CCDS transcript)
		I_EQUAL(transcripts.count(), 0);

		//geneIdOfTranscript
		I_EQUAL(db.geneIdOfTranscript("BRCA1_TR1"), 1);
		I_EQUAL(db.geneIdOfTranscript("BRCA2_TR1"), 2);
		I_EQUAL(db.geneIdOfTranscript("NIPA1_TR1"), 3);
		I_EQUAL(db.geneIdOfTranscript("NIPA1_TR2"), 3);
		I_EQUAL(db.geneIdOfTranscript("NON-CODING_TR1"), 4);
		I_EQUAL(db.geneIdOfTranscript("HARSTEM_ROX", false), -1); //not present

		//transcriptToRegions
		regions = db.transcriptToRegions("NIPA1_TR2", "gene");
		I_EQUAL(regions.count(), 1);
		S_EQUAL(regions[0].annotations()[0], "NIPA1 NIPA1_TR2.5");
		I_EQUAL(regions.baseCount(), 341);

		regions = db.transcriptToRegions("NIPA1_TR2.5", "exon");
		I_EQUAL(regions.count(), 2);
		S_EQUAL(regions[0].annotations()[0], "NIPA1 NIPA1_TR2.5");
		I_EQUAL(regions.baseCount(), 102);

		regions = db.transcriptToRegions("NON-CODING_TR1", "exon");
		I_EQUAL(regions.count(), 2);
		S_EQUAL(regions[0].annotations()[0], "NON-CODING NON-CODING_TR1.6");
		I_EQUAL(regions.baseCount(), 202);

		//geneInfo
		GeneInfo ginfo = db.geneInfo("BRCA1");
		S_EQUAL(ginfo.symbol, "BRCA1");
		S_EQUAL(ginfo.name, "Breast cancer associated gene 1");
		S_EQUAL(ginfo.oe_syn, "0.77");
		S_EQUAL(ginfo.oe_mis, "0.88");
		S_EQUAL(ginfo.oe_lof, "0.99");
		S_EQUAL(ginfo.inheritance, "AD");
		S_EQUAL(ginfo.comments, "");

		ginfo = db.geneInfo("NIPA1");
		S_EQUAL(ginfo.symbol, "NIPA1");
		S_EQUAL(ginfo.oe_syn, "n/a");
		S_EQUAL(ginfo.oe_mis, "n/a");
		S_EQUAL(ginfo.oe_lof, "n/a");
		S_EQUAL(ginfo.inheritance, "n/a");
		S_EQUAL(ginfo.comments, "");

		//setGeneInfo (existing gene)
		S_EQUAL(ginfo.symbol, "NIPA1");
		S_EQUAL(ginfo.oe_syn, "n/a");
		S_EQUAL(ginfo.oe_mis, "n/a");
		S_EQUAL(ginfo.oe_lof, "n/a");
		ginfo.inheritance = "AD";
		ginfo.comments = "comment";
		ginfo.oe_syn = "0.11";
		ginfo.oe_mis = "0.22";
		ginfo.oe_lof = "0.33";
		db.setGeneInfo(ginfo);
		ginfo = db.geneInfo("NIPA1");
		S_EQUAL(ginfo.symbol, "NIPA1");
		S_EQUAL(ginfo.oe_syn, "n/a");
		S_EQUAL(ginfo.oe_mis, "n/a");
		S_EQUAL(ginfo.oe_lof, "n/a");
		S_EQUAL(ginfo.inheritance, "AD");
		S_EQUAL(ginfo.comments, "comment");

		//setGeneInfo (new gene)
		ginfo.symbol = "NEWGENE";
		ginfo.oe_syn = "0.11";
		ginfo.oe_mis = "0.22";
		ginfo.oe_lof = "0.33";
		db.setGeneInfo(ginfo);
		ginfo = db.geneInfo("NEWGENE");
		S_EQUAL(ginfo.symbol, "NEWGENE");
		S_EQUAL(ginfo.oe_syn, "n/a");
		S_EQUAL(ginfo.oe_mis, "n/a");
		S_EQUAL(ginfo.oe_lof, "n/a");
		S_EQUAL(ginfo.inheritance, "AD");
		S_EQUAL(ginfo.comments, "comment");

		//approvedGeneNames
		GeneSet approved = db.approvedGeneNames();
		I_EQUAL(approved.count(), 20);

		//phenotypes
		PhenotypeList phenos = db.phenotypes(QStringList() << "aBNOrmality");
		I_EQUAL(phenos.count(), 1);
		IS_TRUE(phenos.containsAccession("HP:0000118")); //Phenotypic abnormality
		//synonyms
		phenos = db.phenotypes(QStringList() << "sYNonym");
		I_EQUAL(phenos.count(), 2);
		IS_TRUE(phenos.containsAccession("HP:0012823")); //Clinical modifier
		IS_TRUE(phenos.containsAccession("HP:0040279")); //Frequency
		//phenotypeIdByName / phenotypeIdByAccession
		int hpo_id1 = db.phenotypeIdByName("Frequency");
		int hpo_id2 = db.phenotypeIdByAccession("HP:0040279");
		I_EQUAL(hpo_id1, hpo_id2);

		//phenotypeChildTems
		phenos = db.phenotypeChildTerms(db.phenotypeIdByName("All"), true);
		I_EQUAL(phenos.count(), 10);
		phenos = db.phenotypeChildTerms(db.phenotypeIdByName("All"), false);
		I_EQUAL(phenos.count(), 4);
		IS_TRUE(phenos.containsAccession("HP:0000005")); //Mode of inheritance
		IS_TRUE(phenos.containsAccession("HP:0000118")); //Phenotypic abnormality
		IS_TRUE(phenos.containsAccession("HP:0012823")); //Clinical modifier
		IS_TRUE(phenos.containsAccession("HP:0040279")); //"Frequency"
		//inner node
		phenos = db.phenotypeChildTerms(db.phenotypeIdByName("Mode of inheritance"), true);
		I_EQUAL(phenos.count(), 6);
		IS_TRUE(phenos.containsAccession("HP:0001419")); //X-linked recessive inheritance
		phenos = db.phenotypeChildTerms(db.phenotypeIdByName("Mode of inheritance"), false);
		I_EQUAL(phenos.count(), 4);
		IS_FALSE(phenos.containsAccession("HP:0001419")); //X-linked recessive inheritance
		//leaf
		phenos = db.phenotypeChildTerms(db.phenotypeIdByName("Mitochondrial inheritance"), true);
		I_EQUAL(phenos.count(), 0);
		phenos = db.phenotypeChildTerms(db.phenotypeIdByName("Mitochondrial inheritance"), false);
		I_EQUAL(phenos.count(), 0);

		//phenotypeParentTerms
		phenos = db.phenotypeParentTerms(db.phenotypeIdByName("All"), false);
		I_EQUAL(phenos.count(), 0);
		phenos = db.phenotypeParentTerms(db.phenotypeIdByName("All"), true);
		I_EQUAL(phenos.count(), 0);
		phenos = db.phenotypeParentTerms(db.phenotypeIdByName("X-linked recessive inheritance"), false);
		I_EQUAL(phenos.count(), 1);
		IS_TRUE(phenos.containsAccession("HP:0001417")); //X-linked inheritance
		phenos = db.phenotypeParentTerms(db.phenotypeIdByName("X-linked recessive inheritance"), true);
		I_EQUAL(phenos.count(), 3);
		IS_TRUE(phenos.containsAccession("HP:0001417")); //X-linked inheritance
		IS_TRUE(phenos.containsAccession("HP:0000005")); //Mode of inheritance
		IS_TRUE(phenos.containsAccession("HP:0000001")); //All

		//getDiagnosticStatus
		DiagnosticStatusData diag_status = db.getDiagnosticStatus(db.processedSampleId("NA12878_03"));
		S_EQUAL(diag_status.date.toString(Qt::ISODate), "2014-07-29T09:40:49");
		S_EQUAL(diag_status.user, "Max Mustermann");
		S_EQUAL(diag_status.dagnostic_status, "done");
		S_EQUAL(diag_status.outcome, "no significant findings");
		S_EQUAL(diag_status.comments, "free text");
		//no entry in DB
		diag_status = db.getDiagnosticStatus(db.processedSampleId("NA12878_04"));
		S_EQUAL(diag_status.user, "");
		IS_FALSE(diag_status.date.isValid());
		S_EQUAL(diag_status.dagnostic_status, "");
		S_EQUAL(diag_status.outcome, "n/a");
		S_EQUAL(diag_status.comments, "");

		//setDiagnosticStatus
		//create new entry
		diag_status.dagnostic_status = "done";
		diag_status.outcome = "significant findings";
		diag_status.comments = "comment1";
		db.setDiagnosticStatus(db.processedSampleId("NA12878_04"), diag_status);
		diag_status = db.getDiagnosticStatus(db.processedSampleId("NA12878_04"));
		S_EQUAL(diag_status.user, "Max Mustermann");
		IS_TRUE(diag_status.date.isValid());
		S_EQUAL(diag_status.dagnostic_status, "done");
		S_EQUAL(diag_status.outcome, "significant findings");
		S_EQUAL(diag_status.comments, "comment1");
		//update existing entry
		diag_status = db.getDiagnosticStatus(db.processedSampleId("NA12878_03"));
		diag_status.comments = "comment2";
		db.setDiagnosticStatus(db.processedSampleId("NA12878_03"), diag_status);
		diag_status = db.getDiagnosticStatus(db.processedSampleId("NA12878_03"));
		IS_TRUE(diag_status.date.isValid());
		S_EQUAL(diag_status.user, "Max Mustermann");
		S_EQUAL(diag_status.dagnostic_status, "done");
		S_EQUAL(diag_status.outcome, "no significant findings");
		S_EQUAL(diag_status.comments, "comment2");

		//setSampleDiseaseData
		sample_id = db.sampleId("NA12878");
		db.setSampleDiseaseData(sample_id, "Neoplasms", "Affected");
		sample_data = db.getSampleData(sample_id);
		S_EQUAL(sample_data.disease_group, "Neoplasms");
		S_EQUAL(sample_data.disease_status, "Affected");

		//getQCData
		QCCollection qc_data = db.getQCData(db.processedSampleId("NA12878_03"));
		I_EQUAL(qc_data.count(), 2);
		S_EQUAL(qc_data.value("target region 20x percentage").toString(2), "95.96");
		I_EQUAL(qc_data.value("target region 20x percentage").type(), QCValueType::DOUBLE);
		S_EQUAL(qc_data.value("target region read depth").toString(2), "103.24");
		I_EQUAL(qc_data.value("target region 20x percentage").type(), QCValueType::DOUBLE);

		//getQCValues
		QVector<double> qc_values = db.getQCValues("QC:2000025", db.processedSampleId("NA12878_03"));
		I_EQUAL(qc_values.count(), 2);
		std::sort(qc_values.begin(), qc_values.end());
		F_EQUAL(qc_values[0], 103.24);
		F_EQUAL(qc_values[1], 132.24);

		//comment
		Variant variant("chr10", 43613843, 43613843, "G", "T");
		S_EQUAL(db.comment(variant), "");

		//setComment
		db.setComment(variant, "var_comm1");
		S_EQUAL(db.comment(variant), "var_comm1");

		//getClassification
		ClassificationInfo class_info = db.getClassification(variant);
		S_EQUAL(class_info.classification, "");
		S_EQUAL(class_info.comments, "");

		//setClassification
		class_info.classification = "2";
		class_info.comments = "class_comm1";
		db.setClassification(variant, VariantList(), class_info); //Variant list can be empty because variant is alread in NGSD
		class_info = db.getClassification(variant);
		S_EQUAL(class_info.classification, "2");
		S_EQUAL(class_info.comments, "class_comm1");
		//update existing entry
		class_info.classification = "5";
		class_info.comments = "class_comm2";
		db.setClassification(variant, VariantList(), class_info); //Variant list can be empty because variant is alread in NGSD
		class_info = db.getClassification(variant);
		S_EQUAL(class_info.classification, "5");
		S_EQUAL(class_info.comments, "class_comm2");

		//getSomaticClassification
		Variant som_variant("chr7",  140453136, 140453136, "T", "A");
		ClassificationInfo som_class_info = db.getSomaticClassification(variant);
		S_EQUAL(som_class_info.classification, "");
		S_EQUAL(som_class_info.comments, "");

		som_class_info.classification = "activating";
		som_class_info.comments = "som_class_comm1";
		db.setSomaticClassification(som_variant, som_class_info);
		som_class_info = db.getSomaticClassification(som_variant);
		S_EQUAL(som_class_info.classification, "activating");
		S_EQUAL(som_class_info.comments, "som_class_comm1");

		som_class_info.classification = "inactivating";
		som_class_info.comments = "som_class_comm2";
		db.setSomaticClassification(som_variant, som_class_info);
		S_EQUAL(som_class_info.classification, "inactivating");
		S_EQUAL(som_class_info.comments, "som_class_comm2");

		//addPubmedId
		db.addPubmedId(199844, "12345678");
		db.addPubmedId(199844, "87654321");
		QStringList pubmed_ids = db.pubmedIds("199844");
		pubmed_ids.sort();
		S_EQUAL(pubmed_ids.at(0), "12345678");
		S_EQUAL(pubmed_ids.at(1), "87654321");
		//check that duplicate entries are ignored
		db.addPubmedId(199844, "12345678");
		I_EQUAL(db.pubmedIds("199844").count(), 2);

		//analysisInfo
		AnalysisJob analysis_job = db.analysisInfo(-1, false);
		S_EQUAL(analysis_job.type, "");

		analysis_job = db.analysisInfo(1);
		S_EQUAL(analysis_job.type, "single sample");
		I_EQUAL(analysis_job.high_priority, false);
		S_EQUAL(analysis_job.args, "");
		S_EQUAL(analysis_job.sge_id, "4711");
		S_EQUAL(analysis_job.sge_queue, "default_srv018");
		I_EQUAL(analysis_job.samples.count(), 1);
		S_EQUAL(analysis_job.samples[0].name, "NA12878_03");
		S_EQUAL(analysis_job.samples[0].info, "");
		I_EQUAL(analysis_job.history.count(), 3);
		S_EQUAL(analysis_job.history[0].status, "queued");
		S_EQUAL(analysis_job.history[0].user, "ahmustm1");
		S_EQUAL(analysis_job.history[0].timeAsString(), "2018-02-12 10:20:00");
		S_EQUAL(analysis_job.history[0].output.join("\n"), "");
		S_EQUAL(analysis_job.history[1].status, "started");
		S_EQUAL(analysis_job.history[1].user, "");
		S_EQUAL(analysis_job.history[1].timeAsString(), "2018-02-12 10:20:45");
		S_EQUAL(analysis_job.history[1].output.join("\n"), "");
		S_EQUAL(analysis_job.history[2].status, "finished");
		S_EQUAL(analysis_job.history[2].user, "");
		S_EQUAL(analysis_job.history[2].timeAsString(), "2018-02-12 10:34:09");
		S_EQUAL(analysis_job.history[2].output.join("\n"), "warning: bla bla bla");

		//queueAnalysis
		db.queueAnalysis("single sample", true, QStringList() << "-steps ma,vc,an", QList<AnalysisJobSample>() << AnalysisJobSample { "NA12878_03", "index" });
		analysis_job = db.analysisInfo(2);
		S_EQUAL(analysis_job.type, "single sample");
		I_EQUAL(analysis_job.high_priority, true);
		S_EQUAL(analysis_job.args, "-steps ma,vc,an");
		S_EQUAL(analysis_job.sge_id, "");
		S_EQUAL(analysis_job.sge_queue, "");
		I_EQUAL(analysis_job.samples.count(), 1);
		S_EQUAL(analysis_job.samples[0].name, "NA12878_03");
		S_EQUAL(analysis_job.samples[0].info, "index");
		I_EQUAL(analysis_job.history.count(), 1);
		S_EQUAL(analysis_job.history[0].status, "queued");
		S_EQUAL(analysis_job.history[0].user, "ahmustm1");
		IS_TRUE(analysis_job.history[0].timeAsString().startsWith(QDate::currentDate().toString(Qt::ISODate)));
		S_EQUAL(analysis_job.history[0].output.join("\n"), "");

		//cancelAnalysis
		bool canceled = db.cancelAnalysis(2);
		I_EQUAL(canceled, true);
		analysis_job = db.analysisInfo(2);
		I_EQUAL(analysis_job.history.count(), 2);
		S_EQUAL(analysis_job.history[0].status, "queued");
		S_EQUAL(analysis_job.history[0].user, "ahmustm1");
		S_EQUAL(analysis_job.history[1].status, "cancel");
		S_EQUAL(analysis_job.history[1].user, "ahmustm1");
		IS_TRUE(analysis_job.history[1].timeAsString().startsWith(QDate::currentDate().toString(Qt::ISODate)));
		S_EQUAL(analysis_job.history[1].output.join("\n"), "");

		canceled = db.cancelAnalysis(2);
		I_EQUAL(canceled, false);

		//lastAnalysisOf
		int job_id = db.lastAnalysisOf(db.processedSampleId("NA12878_03"));
		I_EQUAL(job_id, 2);

		//deleteAnalysis
		bool deleted = db.deleteAnalysis(2);
		I_EQUAL(deleted, true);
		analysis_job = db.analysisInfo(2, false);
		S_EQUAL(analysis_job.type, "");
		deleted = db.deleteAnalysis(2);
		I_EQUAL(deleted, false);

		//analysisJobFolder
		QString folder = db.analysisJobFolder(1);
		IS_TRUE(folder.endsWith("test/KontrollDNACoriell/Sample_NA12878_03/"));
		db.queueAnalysis("somatic", false, QStringList(), QList<AnalysisJobSample>() << AnalysisJobSample{"NA12345_01", "tumor"} << AnalysisJobSample{"NA12878_03", "normal"});
		folder = db.analysisJobFolder(3);
		IS_TRUE(folder.endsWith("test/KontrollDNACoriell/Somatic_NA12345_01-NA12878_03/"));
		db.queueAnalysis("trio", false, QStringList(), QList<AnalysisJobSample>() << AnalysisJobSample{"NA12878_03", "child"} << AnalysisJobSample{"NA12123_04", "father"} << AnalysisJobSample{"NA12345_01", "mother"});
		folder = db.analysisJobFolder(4);
		IS_TRUE(folder.endsWith("test/KontrollDNACoriell/Trio_NA12878_03_NA12123_04_NA12345_01/"));
		db.queueAnalysis("multi sample", false, QStringList(), QList<AnalysisJobSample>() << AnalysisJobSample{"NA12123_04", "affected"} << AnalysisJobSample{"NA12345_01", "affected"});
		folder = db.analysisJobFolder(5);
		IS_TRUE(folder.endsWith("test/KontrollDNACoriell/Multi_NA12123_04_NA12345_01/"));

		//analysisJobGSvarFile
		QString gsvar = db.analysisJobGSvarFile(1);
		IS_TRUE(gsvar.endsWith("test/KontrollDNACoriell/Sample_NA12878_03/NA12878_03.GSvar"));
		gsvar = db.analysisJobGSvarFile(3);
		IS_TRUE(gsvar.endsWith("test/KontrollDNACoriell/Somatic_NA12345_01-NA12878_03/NA12345_01-NA12878_03.GSvar"));
		gsvar = db.analysisJobGSvarFile(4);
		IS_TRUE(gsvar.endsWith("test/KontrollDNACoriell/Trio_NA12878_03_NA12123_04_NA12345_01/trio.GSvar"));
		gsvar = db.analysisJobGSvarFile(5);
		IS_TRUE(gsvar.endsWith("test/KontrollDNACoriell/Multi_NA12123_04_NA12345_01/multi.GSvar"));

		//updateQC
		db.updateQC(TESTDATA("data_in/qcml.obo"), false);
		I_EQUAL(db.getValue("SELECT count(*) FROM qc_terms").toInt(), 43);
		I_EQUAL(db.getValue("SELECT count(*) FROM qc_terms WHERE obsolete=0").toInt(), 39);
		//second import to test update functionality
		db.updateQC(TESTDATA("data_in/qcml.obo"), false);
		I_EQUAL(db.getValue("SELECT count(*) FROM qc_terms").toInt(), 43);
		I_EQUAL(db.getValue("SELECT count(*) FROM qc_terms WHERE obsolete=0").toInt(), 39);

		//addVariant
		VariantList vl;
		vl.load(TESTDATA("../cppNGS-TEST/data_in/panel_vep.GSvar"));
		I_EQUAL(vl.count(), 329);
		QString var_id = db.addVariant(vl[0], vl);

		//variant
		IS_TRUE(db.variant(var_id)==vl[0]);

		//genotypeCounts
		QString variant_id = db.variantId(Variant("chr10",43613843,43613843,"G","T")); //hom
		GenotypeCounts ngsd_counts = db.genotypeCounts(variant_id);
		I_EQUAL(ngsd_counts.hom, 1);
		I_EQUAL(ngsd_counts.het, 0);
		I_EQUAL(ngsd_counts.mosaic, 0);

		variant_id = db.variantId(Variant("chr17",7579472,7579472,"G","C")); //het
		ngsd_counts = db.genotypeCounts(variant_id);
		I_EQUAL(ngsd_counts.hom, 0);
		I_EQUAL(ngsd_counts.het, 1);
		I_EQUAL(ngsd_counts.mosaic, 0);

		//genotypeCountsCached
		ngsd_counts = db.genotypeCountsCached(variant_id);
		I_EQUAL(ngsd_counts.hom, 0);
		I_EQUAL(ngsd_counts.het, 0);
		I_EQUAL(ngsd_counts.mosaic, 0);

		db.getQuery().exec("UPDATE variant SET germline_het=17, germline_hom=7 WHERE id=" + variant_id);
		ngsd_counts = db.genotypeCountsCached(variant_id);
		I_EQUAL(ngsd_counts.hom, 7);
		I_EQUAL(ngsd_counts.het, 17);
		I_EQUAL(ngsd_counts.mosaic, 0);

		//getSampleDiseaseInfo
		sample_id = db.sampleId("NA12878");
		QList<SampleDiseaseInfo> disease_info = db.getSampleDiseaseInfo(sample_id);
		I_EQUAL(disease_info.count(), 0);

		//setSampleDiseaseInfo
		disease_info << SampleDiseaseInfo{"HP:0001251", "HPO term id", "ahmustm1", QDateTime::currentDateTime()};
		disease_info << SampleDiseaseInfo{"G11.9", "ICD10 code", "ahmustm1", QDateTime::currentDateTime()};
		db.setSampleDiseaseInfo(sample_id, disease_info);
		QList<SampleDiseaseInfo> disease_info2 = db.getSampleDiseaseInfo(sample_id);
		I_EQUAL(disease_info2.count(), 2);
		S_EQUAL(disease_info2[0].disease_info, "HP:0001251");
		S_EQUAL(disease_info2[0].type, "HPO term id");
		QList<SampleDiseaseInfo> disease_info3 = db.getSampleDiseaseInfo(sample_id, "ICD10 code");
		I_EQUAL(disease_info3.count(), 1);
		S_EQUAL(disease_info3[0].disease_info, "G11.9");
		S_EQUAL(disease_info3[0].type, "ICD10 code");

		//processedSampleSearch
		ProcessedSampleSearchParameters params;
		DBTable ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 10);
		I_EQUAL(ps_table.columnCount(), 20);
		//add path
		params.add_path = "SAMPLE_FOLDER";
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 10);
		I_EQUAL(ps_table.columnCount(), 21);
		//add outcome
		params.add_outcome = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 10);
		I_EQUAL(ps_table.columnCount(), 23);
		//add disease details
		params.add_disease_details = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 10);
		I_EQUAL(ps_table.columnCount(), 33);
		//add QC
		params.add_qc = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 10);
		I_EQUAL(ps_table.columnCount(), 74);
		S_EQUAL(ps_table.headers().at(33), "sample_quality");
		S_EQUAL(ps_table.headers().at(34), "processed_sample_quality");
		S_EQUAL(ps_table.row(0).value(33), "good");
		S_EQUAL(ps_table.row(0).value(34), "good");
		//add report config
		params.add_report_config = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 10);
		I_EQUAL(ps_table.columnCount(), 75);
		S_EQUAL(ps_table.row(0).value(74), "");
		S_EQUAL(ps_table.row(4).value(74), "exists, causal variant: chr9:98232224-98232224 A>- (genotype:het genes:PTCH1), causal CNV: chr1:3000-4000 (cn:1 classification:4), causal uncalled CNV: chr2:123456-789012 (genes: EPRS)");
		//add comments
		params.add_comments = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 10);
		I_EQUAL(ps_table.columnCount(), 77);
		S_EQUAL(ps_table.headers().at(20), "comment_sample");
		S_EQUAL(ps_table.headers().at(21), "comment_processed_sample");
		S_EQUAL(ps_table.row(0).value(20), "comment_s6");
		S_EQUAL(ps_table.row(0).value(21), "comment_ps7");
		//add normal sample
		params.add_normal_sample = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 10);
		I_EQUAL(ps_table.columnCount(), 78);
		I_EQUAL(ps_table.headers().count(), 78);
		I_EQUAL(ps_table.columnIndex("normal_sample"), 77);
		S_EQUAL(ps_table.row(5).value(0), "NA12345_01");
		S_EQUAL(ps_table.row(5).value(77), "NA12878_03");
		//apply all search parameters
		params.s_name = "NA12878";
		params.s_species = "human";
		params.s_patient_identifier = "pat1";
		params.s_type = "DNA";
		params.s_sender = "Coriell";
		params.s_study = "SomeStudy";
		params.s_tissue = "blood";
		params.include_bad_quality_samples = false;
		params.include_tumor_samples = false;
		params.include_ffpe_samples = false;
		params.p_name = "KontrollDNACoriell";
		params.p_type = "test";
		params.include_archived_projects = false;
		params.sys_name = "hpHBOCv5";
		params.sys_type = "Panel Haloplex";
		params.r_name = "#00372";
		params.r_device_name = "Neo";
		params.include_bad_quality_runs = false;
		params.run_finished = true;
		params.r_before = QDate::fromString("2021-02-19", Qt::ISODate);
		params.r_after = QDate::fromString("1900-02-19", Qt::ISODate);
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 0);
		I_EQUAL(ps_table.columnCount(), 78);
		//filter based on access rights (restricted user)
		params = ProcessedSampleSearchParameters();
		params.restricted_user = "ahkerra1";
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 5);

		//reportConfigId
		QString ps_id = db.processedSampleId("NA12878_03");
		I_EQUAL(db.reportConfigId(ps_id), -1);

		//setReportConfig
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/cnvs_clincnv.tsv"));
		BedpeFile svs;
		svs.load(TESTDATA("data_in/sv_manta.bedpe"));
		RepeatLocusList res;
		res.load(TESTDATA("data_in/re_calls.vcf"));

		QSharedPointer<ReportConfiguration> report_conf = QSharedPointer<ReportConfiguration>(new ReportConfiguration);
		report_conf->setCreatedBy("ahmustm1");
		ReportVariantConfiguration report_var_conf;
		report_var_conf.variant_type = VariantType::SNVS_INDELS;
		report_var_conf.variant_index = 47;
		report_var_conf.causal = true;
		report_var_conf.report_type = "candidate variant";
		report_var_conf.mosaic = true;
		report_var_conf.exclude_artefact = true;
		report_var_conf.comments = "com1";
		report_var_conf.comments2 = "com2";
		report_conf->set(report_var_conf);
		ReportVariantConfiguration report_var_conf2;
		report_var_conf2.variant_type = VariantType::CNVS;
		report_var_conf2.variant_index = 4;
		report_var_conf2.causal = false;
		report_var_conf2.classification = "4";
		report_var_conf2.report_type = "diagnostic variant";
		report_conf->set(report_var_conf2);
		ReportVariantConfiguration report_var_conf3;
		report_var_conf3.variant_type = VariantType::SVS;
		report_var_conf3.variant_index = 81;
		report_var_conf3.causal = true;
		report_var_conf3.classification = "5";
		report_var_conf3.report_type = "diagnostic variant";
		report_conf->set(report_var_conf3);
		ReportVariantConfiguration report_var_conf4;
		report_var_conf4.variant_type = VariantType::RES;
		report_var_conf4.variant_index = 83;
		report_var_conf4.causal = false;
		report_var_conf4.report_type = "diagnostic variant";
		report_var_conf4.comments = "com1_re";
		report_var_conf4.comments2 = "com2_re";
		report_conf->set(report_var_conf4);

		int conf_id1 = db.setReportConfig(ps_id, report_conf, vl, cnvs, svs, res);

		//reportConfigId
		int conf_id = db.reportConfigId(ps_id);
		IS_TRUE(conf_id!=-1);

		//check data - base config
		QSharedPointer<ReportConfiguration> report_conf2 = db.reportConfig(conf_id, vl, cnvs, svs, res);
		S_EQUAL(report_conf2->createdBy(), "Max Mustermann");
		IS_TRUE(report_conf2->createdAt().isValid());
		S_EQUAL(report_conf2->lastUpdatedBy(), "Max Mustermann");
		IS_TRUE(report_conf2->lastUpdatedAt().isValid());
		S_EQUAL(report_conf2->finalizedBy(), "");
		IS_FALSE(report_conf2->finalizedAt().isValid());
		QDateTime last_update_time_before_update = report_conf2->lastUpdatedAt();

		//check data - manual curation
		ReportVariantConfiguration var_conf2 = report_conf2->variantConfig()[1]; //small variant - order changed because they are sorted by index
		I_EQUAL(var_conf2.variant_index, 47);
		S_EQUAL(var_conf2.manual_var, "");
		S_EQUAL(var_conf2.manual_genotype, "");
		var_conf2 = report_conf2->variantConfig()[0]; //CNV - order changed because they are sorted by index
		S_EQUAL(var_conf2.manual_cnv_start, "");
		S_EQUAL(var_conf2.manual_cnv_end, "");
		S_EQUAL(var_conf2.manual_cnv_cn, "");
		var_conf2 = report_conf2->variantConfig()[2]; //SV
		S_EQUAL(var_conf2.manual_sv_start, "");
		S_EQUAL(var_conf2.manual_sv_end, "");
		S_EQUAL(var_conf2.manual_sv_genotype, "");
		S_EQUAL(var_conf2.manual_sv_start_bnd, "");
		S_EQUAL(var_conf2.manual_sv_end_bnd, "");
		var_conf2 = report_conf2->variantConfig()[3]; //RE
		S_EQUAL(var_conf2.manual_re_allele1, "");
		S_EQUAL(var_conf2.manual_re_allele2, "");

		//change manual curation data
		report_var_conf.manual_var = "chr2:47635523-47635523 ->TT";
		report_var_conf.manual_genotype = "hom";
		report_conf->set(report_var_conf);
		report_var_conf2.manual_cnv_start = "89240000";
		report_var_conf2.manual_cnv_end= "89550000";
		report_var_conf2.manual_cnv_cn = "0";
		report_var_conf2.manual_cnv_hgvs_type = "cnv_type";
		report_var_conf2.manual_cnv_hgvs_suffix = "cnv_suffix";
		report_conf->set(report_var_conf2);
		report_var_conf3.manual_sv_start = "9121440";
		report_var_conf3.manual_sv_end = "9121460";
		report_var_conf3.manual_sv_genotype = "hom";
		report_var_conf3.manual_sv_start_bnd = "93712480";
		report_var_conf3.manual_sv_end_bnd = "93712490";
		report_var_conf3.manual_sv_hgvs_type = "sv_type";
		report_var_conf3.manual_sv_hgvs_suffix = "sv_suffix";
		report_conf->set(report_var_conf3);
		report_var_conf4.manual_re_allele2 = "47";
		report_var_conf4.manual_re_allele1 = "11";
		report_var_conf4.manual_re_allele2 = "47";
		report_conf->set(report_var_conf4);

		//update
		QThread::sleep(1);
		int conf_id2 = db.setReportConfig(ps_id, report_conf, vl, cnvs, svs, res);
		IS_TRUE(conf_id1==conf_id2);
		//check that no double entries are inserted after second execution of setReportConfig
		I_EQUAL(db.getValue("SELECT count(*) FROM cnv WHERE cnv_callset_id=1 AND chr='chr2' AND start=89246800 AND end=89545067 AND cn=1").toInt(), 1);

		//check data
		report_conf2 = db.reportConfig(conf_id, vl, cnvs, svs, res);
		S_EQUAL(report_conf2->createdBy(), "Max Mustermann");
		IS_TRUE(report_conf2->createdAt().isValid());
		S_EQUAL(report_conf2->lastUpdatedBy(), "Max Mustermann");
		IS_TRUE(report_conf2->lastUpdatedAt().isValid());
		IS_TRUE(last_update_time_before_update<report_conf2->lastUpdatedAt());

		S_EQUAL(report_conf2->finalizedBy(), "");
		IS_FALSE(report_conf2->finalizedAt().isValid());

		S_EQUAL(report_conf2->createdBy(), "Max Mustermann");
		IS_TRUE(report_conf2->createdAt().date()==QDate::currentDate());
		I_EQUAL(report_conf2->variantConfig().count(), 4);
		ReportVariantConfiguration var_conf = report_conf2->variantConfig()[1]; //order changed because they are sorted by index
		I_EQUAL(var_conf.id, 2)
		I_EQUAL(var_conf.variant_index, 47);
		IS_TRUE(var_conf.causal);
		S_EQUAL(var_conf.classification, "n/a");
		S_EQUAL(var_conf.report_type, report_var_conf.report_type);
		IS_TRUE(var_conf.mosaic);
		IS_TRUE(var_conf.exclude_artefact);
		S_EQUAL(var_conf.comments, report_var_conf.comments);
		S_EQUAL(var_conf.comments2, report_var_conf.comments2);
		IS_FALSE(var_conf.de_novo);
		IS_FALSE(var_conf.comp_het);
		IS_FALSE(var_conf.exclude_frequency);
		IS_FALSE(var_conf.exclude_mechanism);
		IS_FALSE(var_conf.exclude_other);
		IS_FALSE(var_conf.exclude_phenotype);
		S_EQUAL(var_conf.manual_var, "chr2:47635523-47635523 ->TT");
		S_EQUAL(var_conf.manual_genotype, "hom");

		var_conf = report_conf2->variantConfig()[0]; //order changed because they are sorted by index
		I_EQUAL(var_conf.id, 2)
		I_EQUAL(var_conf.variant_index, 4);
		IS_FALSE(var_conf.causal);
		S_EQUAL(var_conf.classification, "4");
		S_EQUAL(var_conf.report_type, report_var_conf2.report_type);
		IS_FALSE(var_conf.mosaic);
		IS_FALSE(var_conf.exclude_artefact);
		S_EQUAL(var_conf.comments, report_var_conf2.comments);
		S_EQUAL(var_conf.comments2, report_var_conf2.comments2);
		IS_FALSE(var_conf.de_novo);
		IS_FALSE(var_conf.comp_het);
		IS_FALSE(var_conf.exclude_frequency);
		IS_FALSE(var_conf.exclude_mechanism);
		IS_FALSE(var_conf.exclude_other);
		IS_FALSE(var_conf.exclude_phenotype);
		S_EQUAL(var_conf.manual_cnv_start, "89240000");
		S_EQUAL(var_conf.manual_cnv_end, "89550000");
		S_EQUAL(var_conf.manual_cnv_cn, "0");
		S_EQUAL(var_conf.manual_cnv_hgvs_type, "cnv_type");
		S_EQUAL(var_conf.manual_cnv_hgvs_suffix, "cnv_suffix");

		var_conf = report_conf2->variantConfig()[2];
		I_EQUAL(var_conf.id, 1)
		I_EQUAL(var_conf.variant_index, 81);
		IS_TRUE(var_conf.causal);
		S_EQUAL(var_conf.classification, "5");
		S_EQUAL(var_conf.report_type, report_var_conf2.report_type);
		IS_FALSE(var_conf.mosaic);
		IS_FALSE(var_conf.exclude_artefact);
		S_EQUAL(var_conf.comments, report_var_conf2.comments);
		S_EQUAL(var_conf.comments2, report_var_conf2.comments2);
		IS_FALSE(var_conf.de_novo);
		IS_FALSE(var_conf.comp_het);
		IS_FALSE(var_conf.exclude_frequency);
		IS_FALSE(var_conf.exclude_mechanism);
		IS_FALSE(var_conf.exclude_other);
		IS_FALSE(var_conf.exclude_phenotype);
		IS_TRUE(var_conf.variant_type == VariantType::SVS);
		S_EQUAL(var_conf.manual_sv_start, "9121440");
		S_EQUAL(var_conf.manual_sv_end, "9121460");
		S_EQUAL(var_conf.manual_sv_genotype, "hom");
		S_EQUAL(var_conf.manual_sv_start_bnd, "93712480");
		S_EQUAL(var_conf.manual_sv_end_bnd, "93712490");
		S_EQUAL(var_conf.manual_sv_hgvs_type, "sv_type");
		S_EQUAL(var_conf.manual_sv_hgvs_suffix, "sv_suffix");

		var_conf = report_conf2->variantConfig()[3];
		I_EQUAL(var_conf.id, 1)
		I_EQUAL(var_conf.variant_index, 83);
		IS_FALSE(var_conf.causal);
		S_EQUAL(var_conf.classification, "n/a");
		S_EQUAL(var_conf.report_type, report_var_conf4.report_type);
		IS_FALSE(var_conf.mosaic);
		IS_FALSE(var_conf.exclude_artefact);
		S_EQUAL(var_conf.comments, report_var_conf4.comments);
		S_EQUAL(var_conf.comments2, report_var_conf4.comments2);
		IS_FALSE(var_conf.de_novo);
		IS_FALSE(var_conf.comp_het);
		IS_FALSE(var_conf.exclude_frequency);
		IS_FALSE(var_conf.exclude_mechanism);
		IS_FALSE(var_conf.exclude_other);
		IS_FALSE(var_conf.exclude_phenotype);
		IS_TRUE(var_conf.variant_type == VariantType::RES);
		S_EQUAL(var_conf.manual_re_allele1, "11");
		S_EQUAL(var_conf.manual_re_allele2, "47");

		//test modification
		var_conf = report_conf2->variantConfig()[1];
		var_conf.comments = "Test comment1";
		var_conf.comments2 = "Test comment2";
		var_conf.causal = false;
		var_conf.exclude_artefact = false;
		report_conf2->set(var_conf);
		conf_id2 = db.setReportConfig(ps_id, report_conf2, vl, cnvs, svs, res);
		report_conf2 = db.reportConfig(conf_id, vl, cnvs, svs, res);
		var_conf = report_conf2->variantConfig()[1];
		I_EQUAL(var_conf.id, 2)
		I_EQUAL(var_conf.variant_index, 47);
		IS_FALSE(var_conf.causal);
		S_EQUAL(var_conf.classification, "n/a");
		S_EQUAL(var_conf.report_type, report_var_conf.report_type);
		IS_TRUE(var_conf.mosaic);
		IS_FALSE(var_conf.exclude_artefact);
		S_EQUAL(var_conf.comments, "Test comment1");
		S_EQUAL(var_conf.comments2, "Test comment2");
		IS_FALSE(var_conf.de_novo);
		IS_FALSE(var_conf.comp_het);
		IS_FALSE(var_conf.exclude_frequency);
		IS_FALSE(var_conf.exclude_mechanism);
		IS_FALSE(var_conf.exclude_other);
		IS_FALSE(var_conf.exclude_phenotype);
		S_EQUAL(var_conf.manual_var, "chr2:47635523-47635523 ->TT");
		S_EQUAL(var_conf.manual_genotype, "hom");

		//finalizeReportConfig
		conf_id = db.setReportConfig(ps_id, report_conf, vl, cnvs, svs, res);
		IS_FALSE(db.reportConfigIsFinalized(conf_id));
		db.finalizeReportConfig(conf_id, db.userId("ahmustm1"));
		IS_TRUE(db.reportConfigIsFinalized(conf_id));
		report_conf2 = db.reportConfig(conf_id, vl, cnvs, svs, res);
		S_EQUAL(report_conf2->finalizedBy(), "Max Mustermann");
		IS_TRUE(report_conf2->finalizedAt().isValid());
		//check finalized report config cannot be modified or deleted
		IS_THROWN(ProgrammingException, db.setReportConfig(ps_id, report_conf, vl, cnvs, svs, res));
		IS_THROWN(ProgrammingException, db.deleteReportConfig(conf_id));

		//check messages if variant is missing
		vl.clear();
		report_conf2 = db.reportConfig(conf_id, vl, cnvs, svs, res);
		I_EQUAL(report_conf2->variantConfig().count(), 3);
		X_EQUAL(report_conf2->variantConfig()[0].variant_type, VariantType::CNVS);

		//deleteReportConfig
		db.getQuery().exec("UPDATE report_configuration SET finalized_by=NULL, finalized_date=NULL WHERE id="+QString::number(conf_id));
		I_EQUAL(db.getValue("SELECT count(*) FROM report_configuration").toInt(), 2); //result is 2 because there is one report config already in test NGSD after init
		db.deleteReportConfig(conf_id);
		I_EQUAL(db.getValue("SELECT count(*) FROM report_configuration").toInt(), 1);

		//cnvId
		CopyNumberVariant cnv = CopyNumberVariant("chr1", 1000, 2000, 1, GeneSet(), QByteArrayList());
		QString cnv_id = db.cnvId(cnv, 4711, false); //callset 4711 does not exist
		S_EQUAL(cnv_id, "");

		processed_sample_id = db.processedSampleId("NA12878_03");
		cnv_id = db.cnvId(cnv, 1, false);
		S_EQUAL(cnv_id, "1");

		cnv = CopyNumberVariant("chr12", 1000, 2000, 1, GeneSet(), QByteArrayList());
		cnv_id = db.cnvId(cnv, 1, false); //CNV on chr12 does not exist
		S_EQUAL(cnv_id, "");

		//addCnv
		CnvList cnv_list;
		cnv_list.load(TESTDATA("data_in/cnvs_clincnv.tsv"));
		cnv_id = db.addCnv(1, cnv_list[0], cnv_list, 201); //ll=200 > no import
		S_EQUAL(cnv_id, "");

		cnv_id = db.addCnv(1, cnv_list[0], cnv_list);
		S_EQUAL(cnv_id, "6");
		S_EQUAL(db.getValue("SELECT cn FROM cnv WHERE id="+cnv_id).toString(), "0");
		S_EQUAL(db.getValue("SELECT quality_metrics FROM cnv WHERE id="+cnv_id).toString(), "{\"loglikelihood\":\"200\",\"qvalue\":\"0\",\"regions\":\"2\"}");

		//cnvCallsetMetrics (of sample)
		QHash<QString, QString> callset_metrics = db.cnvCallsetMetrics(1);
		I_EQUAL(callset_metrics.count(), 6);
		S_EQUAL(callset_metrics["gender of sample"], "M");
		S_EQUAL(callset_metrics["number of iterations"], "1");

		//deleteVariants
		processed_sample_id = db.processedSampleId("NA12878_03");
		I_EQUAL(db.getValue("SELECT count(*) FROM detected_variant WHERE processed_sample_id=" + processed_sample_id).toInt(), 137);
		I_EQUAL(db.getValue("SELECT count(*) FROM cnv_callset WHERE processed_sample_id=" + processed_sample_id).toInt(), 1);
		db.deleteVariants(processed_sample_id);
		I_EQUAL(db.getValue("SELECT count(*) FROM detected_variant WHERE processed_sample_id=" + processed_sample_id).toInt(), 0);
		I_EQUAL(db.getValue("SELECT count(*) FROM cnv_callset WHERE processed_sample_id=" + processed_sample_id).toInt(), 0);

		//tableInfo
		QStringList tables = db.tables();
		foreach(QString table, tables)
		{
			TableInfo table_info = db.tableInfo(table);
		}

		//userId
		I_EQUAL(db.userId("ahkerra1"), 101);
		I_EQUAL(db.userId("Sarah Kerrigan"), 101);

		//userName
		S_EQUAL(db.userName(101), "Sarah Kerrigan");

		//userEmail
		S_EQUAL(db.userEmail(101), "queen_of_blades@the_swarm.org");

		//checkPasword
		S_EQUAL(db.checkPassword("bla", ""), "User 'bla' does not exist!");
		S_EQUAL(db.checkPassword("ahkerra1", ""), "User 'ahkerra1' is no longer active!");
		S_EQUAL(db.checkPassword("ahkerra1", "", false), "Invalid password for user 'ahkerra1'!");
		S_EQUAL(db.checkPassword("ahmustm1", "123456"), ""); //with salt
		S_EQUAL(db.checkPassword("admin", "admin"), ""); //no salt

		//setPassword
		db.setPassword(db.userId("ahmustm1"), "abcdef");
		S_EQUAL(db.checkPassword("ahmustm1", "abcdef"), "");

		//checkValue
		I_EQUAL(db.checkValue("sample", "name", "NA12878", false).count(), 0); //VARCHAR
		I_EQUAL(db.checkValue("sample", "name", "NA12878", true).count(), 1); //VARCHAR: unique
		I_EQUAL(db.checkValue("mid", "sequence", "", true).count(), 1); //VARCHAR: not nullable > not empty
		I_EQUAL(db.checkValue("mid", "sequence", "ACGTACGTACGTACGTACGTACGTACGTACGTACGTACGTACGTACGT", true).count(), 1); //VARCHAR: too long
		I_EQUAL(db.checkValue("mid", "sequence", "ACGTX", true).count(), 1); //VARCHAR: regexp mismatch
		I_EQUAL(db.checkValue("mid", "sequence", "ACGT", true).count(), 0); //VARCHAR

		I_EQUAL(db.checkValue("sample", "disease_status", "Affected", true).count(), 0); //ENUM
		I_EQUAL(db.checkValue("sample", "disease_status", "Typo", true).count(), 1); //ENUM: not valid

		I_EQUAL(db.checkValue("sample", "species_id", "1", true).count(), 0); //FK

		I_EQUAL(db.checkValue("sample", "concentration", "1", true).count(), 0); //FLOAT
		I_EQUAL(db.checkValue("sample", "concentration", "1a", true).count(), 1); //FLOAT: not convertable

		I_EQUAL(db.checkValue("sample", "tumor", "0", true).count(), 0); //BOOL
		I_EQUAL(db.checkValue("sample", "tumor", "1", true).count(), 0); //BOOL
		I_EQUAL(db.checkValue("sample", "tumor", "2", true).count(), 1); //BOOL: not convertable

		I_EQUAL(db.checkValue("sample", "received", "1977-12-01", true).count(), 0); //DATE
		I_EQUAL(db.checkValue("sample", "received", "somewhen", true).count(), 1); //DATE: not convertable

		//omimInfo
		QList<OmimInfo> omim_info = db.omimInfo("DOES_NOT_EXIST");
		I_EQUAL(omim_info.count(), 0);

		omim_info = db.omimInfo("ATM");
		I_EQUAL(omim_info.count(), 1);
		S_EQUAL(omim_info[0].gene_symbol, "ATM");
		S_EQUAL(omim_info[0].mim, "607585");
		I_EQUAL(omim_info[0].phenotypes.count(), 5);
		S_EQUAL(omim_info[0].phenotypes[0].accession(), "208900");
		S_EQUAL(omim_info[0].phenotypes[0].name(), "Ataxia-telangiectasia, 208900 (3)");
		S_EQUAL(omim_info[0].phenotypes[1].accession(), "114480");
		S_EQUAL(omim_info[0].phenotypes[1].name(), "Breast cancer, susceptibility to, 114480 (3)");
		S_EQUAL(omim_info[0].phenotypes[2].accession(), "");
		S_EQUAL(omim_info[0].phenotypes[2].name(), "Lymphoma, B-cell non-Hodgkin, somatic (3)");

		omim_info = db.omimInfo("SHOX");
		I_EQUAL(omim_info.count(), 2);
		S_EQUAL(omim_info[0].gene_symbol, "SHOX");
		S_EQUAL(omim_info[0].mim, "312865");
		I_EQUAL(omim_info[0].phenotypes.count(), 3);
		S_EQUAL(omim_info[1].gene_symbol, "SHOX");
		S_EQUAL(omim_info[1].mim, "400020");
		I_EQUAL(omim_info[1].phenotypes.count(), 3);

		//check SV table names
		QList<StructuralVariantType> sv_types;
		sv_types << StructuralVariantType::DEL << StructuralVariantType::DUP << StructuralVariantType::INV << StructuralVariantType::INS << StructuralVariantType::BND;
		foreach (StructuralVariantType sv_type, sv_types)
		{
			QString table_name = db.svTableName(sv_type);
			db.getValue("SELECT count(*) FROM " + table_name, false);
		}


		//Test Evaluation Sheet Data table
		EvaluationSheetData esd_test_input;
		esd_test_input.ps_id = "4001";
		esd_test_input.dna_rna = "DNA_12345";
		esd_test_input.reviewer1 = "Max Mustermann";
		esd_test_input.review_date1 = QDate(2020, 2, 20);
		esd_test_input.reviewer2 = "Sarah Kerrigan";
		esd_test_input.review_date2 = QDate(2033, 3, 30);
		esd_test_input.analysis_scope = "Analyseumfang";
		esd_test_input.acmg_requested = (rand() % 2) == 0;
		esd_test_input.acmg_noticeable = (rand() % 2) == 0;
		esd_test_input.acmg_analyzed = (rand() % 2) == 0;
		esd_test_input.filtered_by_freq_based_dominant = (rand() % 2) == 0;
		esd_test_input.filtered_by_freq_based_recessive = (rand() % 2) == 0;
		esd_test_input.filtered_by_mito = (rand() % 2) == 0;
		esd_test_input.filtered_by_x_chr = (rand() % 2) == 0;
		esd_test_input.filtered_by_cnv = (rand() % 2) == 0;
		esd_test_input.filtered_by_svs = (rand() % 2) == 0;
		esd_test_input.filtered_by_res = (rand() % 2) == 0;
		esd_test_input.filtered_by_mosaic = (rand() % 2) == 0;
		esd_test_input.filtered_by_phenotype = (rand() % 2) == 0;
		esd_test_input.filtered_by_multisample = (rand() % 2) == 0;
		esd_test_input.filtered_by_trio_stringent = (rand() % 2) == 0;
		esd_test_input.filtered_by_trio_relaxed = (rand() % 2) == 0;

		db.storeEvaluationSheetData(esd_test_input);

		EvaluationSheetData esd_db_export = db.evaluationSheetData("4001");
		IS_TRUE(esd_test_input.ps_id == esd_db_export.ps_id);
		IS_TRUE(esd_test_input.dna_rna == esd_db_export.dna_rna);
		IS_TRUE(esd_test_input.reviewer1 == esd_db_export.reviewer1);
		IS_TRUE(esd_test_input.review_date1 == esd_db_export.review_date1);
		IS_TRUE(esd_test_input.reviewer2 == esd_db_export.reviewer2);
		IS_TRUE(esd_test_input.review_date2 == esd_db_export.review_date2);
		IS_TRUE(esd_test_input.analysis_scope == esd_db_export.analysis_scope);
		IS_TRUE(esd_test_input.acmg_requested == esd_db_export.acmg_requested);
		IS_TRUE(esd_test_input.acmg_noticeable == esd_db_export.acmg_noticeable);
		IS_TRUE(esd_test_input.acmg_analyzed == esd_db_export.acmg_analyzed);
		IS_TRUE(esd_test_input.filtered_by_freq_based_dominant == esd_db_export.filtered_by_freq_based_dominant);
		IS_TRUE(esd_test_input.filtered_by_freq_based_recessive == esd_db_export.filtered_by_freq_based_recessive);
		IS_TRUE(esd_test_input.filtered_by_mito == esd_db_export.filtered_by_mito);
		IS_TRUE(esd_test_input.filtered_by_x_chr == esd_db_export.filtered_by_x_chr);
		IS_TRUE(esd_test_input.filtered_by_cnv == esd_db_export.filtered_by_cnv);
		IS_TRUE(esd_test_input.filtered_by_svs == esd_db_export.filtered_by_svs);
		IS_TRUE(esd_test_input.filtered_by_res == esd_db_export.filtered_by_res);
		IS_TRUE(esd_test_input.filtered_by_mosaic == esd_db_export.filtered_by_mosaic);
		IS_TRUE(esd_test_input.filtered_by_phenotype == esd_db_export.filtered_by_phenotype);
		IS_TRUE(esd_test_input.filtered_by_multisample == esd_db_export.filtered_by_multisample);
		IS_TRUE(esd_test_input.filtered_by_trio_stringent == esd_db_export.filtered_by_trio_stringent);
		IS_TRUE(esd_test_input.filtered_by_trio_relaxed == esd_db_export.filtered_by_trio_relaxed);

		//change input
		esd_test_input.dna_rna = "DNA_67890";

		// test import with no overwrite
		IS_THROWN(DatabaseException, db.storeEvaluationSheetData(esd_test_input));

		// test with overwrite
		db.storeEvaluationSheetData(esd_test_input, true);
		esd_db_export = db.evaluationSheetData("4001");

		// check value change
		S_EQUAL(esd_db_export.dna_rna, "DNA_67890");

		// test failed import
		esd_test_input.ps_id = "invalid_id";
		IS_THROWN(DatabaseException, db.storeEvaluationSheetData(esd_test_input));

		// test failed export
		IS_THROWN(DatabaseException, db.evaluationSheetData("-4"));
		S_EQUAL(db.evaluationSheetData("-4", false).ps_id, "");

		//addPreferredTranscript
		bool added = db.addPreferredTranscript("NIPA1_TR1");
		IS_TRUE(added);
		added = db.addPreferredTranscript("NIPA1_TR1");
		IS_FALSE(added);
		added = db.addPreferredTranscript("NIPA1_TR2");
		IS_TRUE(added);
		added = db.addPreferredTranscript("NIPA1_TR2");
		IS_FALSE(added);
		added = db.addPreferredTranscript("NON-CODING_TR1");
		IS_TRUE(added);
		IS_THROWN(DatabaseException, db.addPreferredTranscript("INVALID_TRANSCRIPT_NAME"));

		//getPreferredTranscripts
		QMap<QByteArray, QByteArrayList> pt = db.getPreferredTranscripts();
		I_EQUAL(pt.count(), 4);
		IS_TRUE(pt.contains("NIPA1"));
		IS_TRUE(pt.contains("NON-CODING"));
		I_EQUAL(pt["NIPA1"].count(), 2);
		IS_TRUE(pt["NIPA1"].contains("NIPA1_TR1"));
		IS_TRUE(pt["NIPA1"].contains("NIPA1_TR2"));
		I_EQUAL(pt["NON-CODING"].count(), 1);
		IS_TRUE(pt["NON-CODING"].contains("NON-CODING_TR1"));
		I_EQUAL(pt["SPG7"].count(), 1);
		IS_TRUE(pt["SPG7"].contains("ENST00000341316"));
		I_EQUAL(pt["MT-CO2"].count(), 2);
		IS_TRUE(pt["MT-CO2"].contains("MT-CO2_TR3"));
		IS_TRUE(pt["MT-CO2"].contains("MT-CO2_TR4"));

		//addSampleRelation
		db.addSampleRelation(SampleRelation{"NA12345", "siblings", "NA12878"});
		IS_THROWN(DatabaseException, db.addSampleRelation(SampleRelation{"NA12345", "siblings", "NA12878"}));

		//sameSample
		I_EQUAL(db.sameSamples(99, SameSampleMode::SAME_PATIENT).count(), 0);
		I_EQUAL(db.sameSamples(2, SameSampleMode::SAME_PATIENT).count(), 3);
		I_EQUAL(db.sameSamples(2, SameSampleMode::SAME_SAMPLE).count(), 2);
		IS_TRUE(db.sameSamples(2, SameSampleMode::SAME_PATIENT).contains(4));
		IS_TRUE(db.sameSamples(2, SameSampleMode::SAME_PATIENT).contains(7));
		IS_TRUE(db.sameSamples(2, SameSampleMode::SAME_PATIENT).contains(8));
		IS_TRUE(db.sameSamples(2, SameSampleMode::SAME_SAMPLE).contains(4));
		IS_TRUE(db.sameSamples(2, SameSampleMode::SAME_SAMPLE).contains(8));
		IS_FALSE(db.sameSamples(2, SameSampleMode::SAME_SAMPLE).contains(7));
		I_EQUAL(db.sameSamples(4, SameSampleMode::SAME_PATIENT).count(), 3);
		IS_TRUE(db.sameSamples(4, SameSampleMode::SAME_PATIENT).contains(2));
		IS_TRUE(db.sameSamples(4, SameSampleMode::SAME_PATIENT).contains(7));
		IS_TRUE(db.sameSamples(4, SameSampleMode::SAME_PATIENT).contains(8));
		I_EQUAL(db.sameSamples(7, SameSampleMode::SAME_PATIENT).count(), 3);
		IS_TRUE(db.sameSamples(7, SameSampleMode::SAME_PATIENT).contains(2));
		IS_TRUE(db.sameSamples(7, SameSampleMode::SAME_PATIENT).contains(4));
		IS_TRUE(db.sameSamples(7, SameSampleMode::SAME_PATIENT).contains(8));

		//relatedSamples
		I_EQUAL(db.relatedSamples(99).count(), 0);
		I_EQUAL(db.relatedSamples(2).count(), 1);
		IS_TRUE(db.relatedSamples(2).contains(4));
		I_EQUAL(db.relatedSamples(4).count(), 2);
		IS_TRUE(db.relatedSamples(4).contains(2));
		IS_TRUE(db.relatedSamples(4).contains(8));
		I_EQUAL(db.relatedSamples(4, "same sample").count(), 2);
		IS_TRUE(db.relatedSamples(4, "same sample").contains(2));
		IS_TRUE(db.relatedSamples(4, "same sample").contains(8));
		I_EQUAL(db.relatedSamples(4, "twins").count(), 0);
		I_EQUAL(db.relatedSamples(4, "same sample", "DNA").count(), 2);
		IS_TRUE(db.relatedSamples(4, "same sample", "DNA").contains(2));
		IS_TRUE(db.relatedSamples(4, "same sample", "DNA").contains(8));

		//omimPreferredPhenotype
		S_EQUAL(db.omimPreferredPhenotype("BRCA1", "Neoplasms"), "");
		S_EQUAL(db.omimPreferredPhenotype("ATM", "Diseases of the immune system"), "");
		S_EQUAL(db.omimPreferredPhenotype("ATM", "Neoplasms"), "114480");

		//userRoleIn
		IS_TRUE(db.userRoleIn("ahmustm1", QStringList{"user"}));
		IS_FALSE(db.userRoleIn("ahmustm1", QStringList{"user_restricted"}));

		IS_FALSE(db.userRoleIn("ahkerra1", QStringList{"user"}));
		IS_TRUE(db.userRoleIn("ahkerra1", QStringList{"user_restricted"}));

		int user_id = db.userId("ahkerra1");
		IS_FALSE(db.userCanAccess(user_id, 3999));
		IS_FALSE(db.userCanAccess(user_id, 4001));
		IS_FALSE(db.userCanAccess(user_id, 4002));
		IS_FALSE(db.userCanAccess(user_id, 5));
		IS_FALSE(db.userCanAccess(user_id, 6));
		IS_TRUE(db.userCanAccess(user_id, 4000)); //access via study 'SecondStudy'
		IS_TRUE(db.userCanAccess(user_id, 4003)); //access via sample 'NA12123repeat'
		IS_TRUE(db.userCanAccess(user_id, 7)); //access via project 'Diagnostik'
		IS_TRUE(db.userCanAccess(user_id, 8)); //access via project 'Diagnostik'

		// test if a restricted user can access a multi-sample analysis
		VariantList multi_sample;
		multi_sample.load(TESTDATA("../cppNGSD-TEST/data_in/multisample_analyisis.GSvar"));
		S_EQUAL(analysisTypeToString(multi_sample.type()), "GERMLINE_MULTISAMPLE");
		// the user has permissions to access each sample from the analysis
		foreach(const SampleInfo& info, multi_sample.getSampleHeader())
		{
			QString processed_sample_id = db.processedSampleId(info.name);
			IS_TRUE(db.userCanAccess(db.userId("restricted"), processed_sample_id.toInt()));
		}
		// the user has a permission to access only one sample
		bool can_access_all = true;
		foreach(const SampleInfo& info, multi_sample.getSampleHeader())
		{
			QString processed_sample_id = db.processedSampleId(info.name);
			if (!db.userCanAccess(db.userId("restricted_one_sample"), processed_sample_id.toInt()))
			{
				can_access_all = false;
				break;
			}
		}
		IS_FALSE(can_access_all);

		//cfDNA panels
		CfdnaPanelInfo panel_info;
		panel_info.tumor_id = db.processedSampleId("DX184894_01").toInt();
		panel_info.created_by = db.userId("ahmustm1");
		panel_info.created_date = QDate(2021, 01, 01);
		panel_info.processing_system_id = db.processingSystemId("IDT_xGenPrism");

		BedFile bed;
		bed.load(TESTDATA("../cppNGSD-TEST/data_in/cfdna_panel.bed"));
		VcfFile vcf;
		vcf.load(TESTDATA("../cppNGSD-TEST/data_in/cfdna_panel.vcf"));

		db.storeCfdnaPanel(panel_info, bed.toText().toUtf8(), vcf.toText());

		I_EQUAL(db.cfdnaPanelInfo(QString::number(panel_info.tumor_id), db.processingSystemId("IDT_xGenPrism")).size(), 1);
		I_EQUAL(db.cfdnaPanelInfo(QString::number(panel_info.tumor_id)).size(), 1);
		I_EQUAL(db.cfdnaPanelInfo(QString::number(panel_info.tumor_id), db.processingSystemId("hpHBOCv5")).size(), 0);

		CfdnaPanelInfo loaded_panel_info = db.cfdnaPanelInfo(QString::number(panel_info.tumor_id), db.processingSystemId("IDT_xGenPrism")).at(0);
		I_EQUAL(loaded_panel_info.tumor_id, panel_info.tumor_id);
        I_EQUAL(loaded_panel_info.created_by, panel_info.created_by);
		IS_TRUE(loaded_panel_info.created_date == panel_info.created_date);
        I_EQUAL(loaded_panel_info.processing_system_id, panel_info.processing_system_id);

		S_EQUAL(db.cfdnaPanelRegions(loaded_panel_info.id).toText(), bed.toText());
		S_EQUAL(db.cfdnaPanelVcf(loaded_panel_info.id).toText(), vcf.toText());

		// test removed regions
		BedFile removed_regions;
		bed.load(TESTDATA("../cppNGSD-TEST/data_in/cfdna_panel.bed"));
		panel_info = db.cfdnaPanelInfo(QString::number(panel_info.tumor_id), db.processingSystemId("IDT_xGenPrism")).at(0);
		db.setCfdnaRemovedRegions(panel_info.id, removed_regions);
		BedFile removed_regions_db = db.cfdnaPanelRemovedRegions(panel_info.id);
		//compare
		removed_regions.clearAnnotations();
		removed_regions.clearHeaders();
		removed_regions_db.clearHeaders();
		S_EQUAL(removed_regions.toText(), removed_regions_db.toText());

		//test ID-SNP extraction
		BedFile target_region;
		target_region.load(TESTDATA("../cppNGSD-TEST/data_in/cfDNA_id_snp.bed"));
		VcfFile id_snps_tumor_normal = db.getIdSnpsFromProcessingSystem(db.processingSystemId("IDT_xGenPrism"), target_region, false, true);
		QByteArrayList vcf_lines;
		for (int i = 0; i < id_snps_tumor_normal.count(); ++i) vcf_lines << id_snps_tumor_normal[i].toString();
		QByteArrayList expected;
		expected << "chr1:13828907 G>A" << "chr1:88923261 A>C" << "chr1:107441476 A>G" << "chr1:160816880 A>G" << "chr1:233312667 C>T" << "chr2:9945593 G>A" << "chr2:59773585 T>C"
				  << "chr2:106122379 A>G" << "chr2:181548532 A>G" << "chr19:4569797 A>C" << "chr19:39069167 A>C" << "chr19:49401772 T>C" << "chr20:52679623 T>C" << "chr21:14109044 G>T"
				  << "chr21:26651051 T>C" << "chr22:20433563 T>C" << "chr22:33163522 T>A" << "chrX:11296872 N>N" << "chrX:24211417 N>N" << "chrY:2787395 N>N" << "chrY:2979985 N>N"
				  << "chrY:6869957 N>N";
		IS_TRUE((vcf_lines==expected));

		//############################### variant publication ###############################
		// variant
		variant = db.variant("199844");
		db.addVariantPublication("NA12878_03.GSvar", variant, "ClinVar", "5", "submission_id=SUB00001234;blabla...");
		SqlQuery query = db.getQuery();
		query.exec("SELECT id, sample_id, variant_table, db, class, details, user_id, result, replaced, linked_id FROM variant_publication WHERE variant_id=199844");
		I_EQUAL(query.size(), 1);
		query.next();
		I_EQUAL(query.value("sample_id").toInt(), 1);
		S_EQUAL(query.value("variant_table").toString(), "variant");
		S_EQUAL(query.value("db").toString(), "ClinVar");
		I_EQUAL(query.value("class").toInt(), 5);
		S_EQUAL(query.value("details").toString(), "submission_id=SUB00001234;blabla...");
		I_EQUAL(query.value("user_id").toInt(), 99);
		S_EQUAL(query.value("result").toString(), "");
		IS_FALSE(query.value("replaced").toBool());
		IS_TRUE(query.value("linked_id").isNull());

		int vp_id = query.value("id").toInt();
		db.updateVariantPublicationResult(vp_id, "processed;SCV12345678");
		query.exec("SELECT id, sample_id, class, details, user_id, result FROM variant_publication WHERE variant_id=199844 AND variant_table='variant'");
		I_EQUAL(query.size(), 1);
		query.next();
		I_EQUAL(query.value("sample_id").toInt(), 1);
		I_EQUAL(query.value("class").toInt(), 5);
		S_EQUAL(query.value("details").toString(), "submission_id=SUB00001234;blabla...");
		I_EQUAL(query.value("user_id").toInt(), 99);
		S_EQUAL(query.value("result").toString(), "processed;SCV12345678");
		IS_TRUE(db.getVariantPublication("NA12878_03.GSvar", variant).startsWith("table: variant db: ClinVar class: 5 user: Max Mustermann date: "));

		// cnv
		CopyNumberVariant cnv2 = db.cnv(4);
		db.addVariantPublication("NA12123repeat_01", cnv2, "ClinVar", "4", "submission_id=SUB00005678;cn=12;variant_id=...");
		query = db.getQuery();
		query.exec("SELECT id, sample_id, variant_table, db, class, details, user_id, result, replaced, linked_id FROM variant_publication WHERE variant_id=4 AND variant_table='cnv'");
		I_EQUAL(query.size(), 1);
		query.next();
		I_EQUAL(query.value("sample_id").toInt(), 4);
		S_EQUAL(query.value("variant_table").toString(), "cnv");
		S_EQUAL(query.value("db").toString(), "ClinVar");
		I_EQUAL(query.value("class").toInt(), 4);
		S_EQUAL(query.value("details").toString(), "submission_id=SUB00005678;cn=12;variant_id=...");
		I_EQUAL(query.value("user_id").toInt(), 99);
		S_EQUAL(query.value("result").toString(), "");
		IS_FALSE(query.value("replaced").toBool());
		IS_TRUE(query.value("linked_id").isNull());
		IS_TRUE(db.getVariantPublication("NA12123repeat_01", cnv2).startsWith("table: cnv db: ClinVar class: 4 user: Max Mustermann date: "));

		//sv
		//reimport SVs
		query.exec("INSERT INTO `sv_callset` (`id`, `processed_sample_id`, `caller`, `caller_version`, `call_date`) VALUES (1, 3999, 'Manta', '1.6.0', '2020-01-01')");
		query.exec("INSERT INTO `sv_deletion` (`id`, `sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) VALUES (2, 1, 'chr1', 1000, 1020, 20000, 20000, '')");
		query.exec("INSERT INTO `sv_insertion` (`id`, `sv_callset_id`, `chr`, `pos`, `ci_upper`) VALUES (3, 1, 'chr1', 17482432, 77)");
		svs.load(TESTDATA("data_in/sv_manta.bedpe"));
		BedpeLine sv = db.structuralVariant(2, StructuralVariantType::DEL, svs);
		int vp_id2 = db.addVariantPublication("NA12878_03", sv, svs, "LOVD", "2", "submission_id=SUB000012354;type=DEL;variant_id=...");
		query = db.getQuery();
		query.exec("SELECT id, sample_id, variant_table, db, class, details, user_id, result, replaced, linked_id FROM variant_publication WHERE variant_id=2 AND variant_table='sv_deletion'");
		I_EQUAL(query.size(), 1);
		query.next();
		I_EQUAL(query.value("sample_id").toInt(), 1);
		S_EQUAL(query.value("variant_table").toString(), "sv_deletion");
		S_EQUAL(query.value("db").toString(), "LOVD");
		I_EQUAL(query.value("class").toInt(), 2);
		S_EQUAL(query.value("details").toString(), "submission_id=SUB000012354;type=DEL;variant_id=...");
		I_EQUAL(query.value("user_id").toInt(), 99);
		S_EQUAL(query.value("result").toString(), "");
		IS_FALSE(query.value("replaced").toBool());
		IS_TRUE(query.value("linked_id").isNull());
		IS_TRUE(db.getVariantPublication("NA12878_03", sv, svs).startsWith("table: sv_deletion db: LOVD class: 2 user: Max Mustermann date: "));

		sv = db.structuralVariant(3, StructuralVariantType::INS, svs);
		db.addVariantPublication("NA12878_03", sv, svs, "LOVD", "5", "submission_id=SUB00001111;type=INS;variant_id=...");
		query = db.getQuery();
		query.exec("SELECT id, sample_id, variant_table, db, class, details, user_id, result, replaced, linked_id FROM variant_publication WHERE variant_id=3 AND variant_table='sv_insertion'");
		I_EQUAL(query.size(), 1);
		query.next();
		I_EQUAL(query.value("sample_id").toInt(), 1);
		S_EQUAL(query.value("variant_table").toString(), "sv_insertion");
		S_EQUAL(query.value("db").toString(), "LOVD");
		I_EQUAL(query.value("class").toInt(), 5);
		S_EQUAL(query.value("details").toString(), "submission_id=SUB00001111;type=INS;variant_id=...");
		I_EQUAL(query.value("user_id").toInt(), 99);
		S_EQUAL(query.value("result").toString(), "");
		IS_FALSE(query.value("replaced").toBool());
		IS_TRUE(query.value("linked_id").isNull());
		IS_TRUE(db.getVariantPublication("NA12878_03", sv, svs).startsWith("table: sv_insertion db: LOVD class: 5 user: Max Mustermann date: "));

		//manual upload
		db.addManualVariantPublication("NA12345", "ClinVar", "4", "submission_id=SUB00002222;type=DEL;variant_desc=chr1:12345-12345 A>T...");
		query = db.getQuery();
		query.exec("SELECT id, sample_id, variant_table, db, class, details, user_id, result, replaced, linked_id FROM variant_publication WHERE variant_id=-1 AND variant_table='none'");
		I_EQUAL(query.size(), 1);
		query.next();
		I_EQUAL(query.value("sample_id").toInt(), 3);
		S_EQUAL(query.value("variant_table").toString(), "none");
		S_EQUAL(query.value("db").toString(), "ClinVar");
		I_EQUAL(query.value("class").toInt(), 4);
		S_EQUAL(query.value("details").toString(), "submission_id=SUB00002222;type=DEL;variant_desc=chr1:12345-12345 A>T...");
		I_EQUAL(query.value("user_id").toInt(), 99);
		S_EQUAL(query.value("result").toString(), "");
		IS_FALSE(query.value("replaced").toBool());
		IS_TRUE(query.value("linked_id").isNull());

		// replaced
		db.flagVariantPublicationAsReplaced(vp_id);
		query.exec("SELECT replaced FROM variant_publication WHERE variant_id=199844");
		I_EQUAL(query.size(), 1);
		query.next();
		IS_TRUE(query.value("replaced").toBool());

		//comp-het
		db.linkVariantPublications(vp_id, vp_id2);
		query.exec("SELECT id, sample_id, variant_table, db, class, details, user_id, result, replaced, linked_id FROM variant_publication WHERE linked_id=" + QString::number(vp_id2));
		I_EQUAL(query.size(), 1);
		query.next();
		I_EQUAL(query.value("id").toInt(), vp_id);
		I_EQUAL(query.value("sample_id").toInt(), 1);
		S_EQUAL(query.value("variant_table").toString(), "variant");
		S_EQUAL(query.value("db").toString(), "ClinVar");
		I_EQUAL(query.value("class").toInt(), 5);
		S_EQUAL(query.value("details").toString(), "submission_id=SUB00001234;blabla...");
		I_EQUAL(query.value("user_id").toInt(), 99);
		S_EQUAL(query.value("result").toString(), "processed;SCV12345678");
		IS_TRUE(query.value("replaced").toBool());
		I_EQUAL(query.value("linked_id").toInt(), vp_id2);

		query.exec("SELECT id, sample_id, variant_table, db, class, details, user_id, result, replaced, linked_id FROM variant_publication WHERE linked_id=" + QString::number(vp_id));
		I_EQUAL(query.size(), 1);
		query.next();
		I_EQUAL(query.value("id").toInt(), vp_id2);
		I_EQUAL(query.value("sample_id").toInt(), 1);
		S_EQUAL(query.value("variant_table").toString(), "sv_deletion");
		S_EQUAL(query.value("db").toString(), "LOVD");
		I_EQUAL(query.value("class").toInt(), 2);
		S_EQUAL(query.value("details").toString(), "submission_id=SUB000012354;type=DEL;variant_id=...");
		I_EQUAL(query.value("user_id").toInt(), 99);
		S_EQUAL(query.value("result").toString(), "");
		IS_FALSE(query.value("replaced").toBool());
		I_EQUAL(query.value("linked_id").toInt(), vp_id);


		//test with invalid IDs
		IS_THROWN(DatabaseException, db.updateVariantPublicationResult(-42, "processed;SCV12345678"));
		IS_THROWN(DatabaseException, db.flagVariantPublicationAsReplaced(-42));

		//############################### gaps ###############################
		int gap_id = db.addGap(3999, "chr1", 5000, 6000, "to close");
		I_EQUAL(db.gapId(3999, "chr1", 5000, 6000), gap_id);
		I_EQUAL(db.gapId(3999, "chr2", 5001, 6001), -1);

		db.updateGapStatus(gap_id, "closed");
		db.updateGapStatus(gap_id, "closed");
		IS_TRUE(db.getValue("SELECT history FROM gaps WHERE id=" + QString::number(gap_id)).toString().contains("closed"));

		db.addGapComment(gap_id, "my_comment");
		IS_TRUE(db.getValue("SELECT history FROM gaps WHERE id=" + QString::number(gap_id)).toString().contains("my_comment"));


		//############################### sub-panels ###############################
		//subPanelList
		QStringList subpanels = db.subPanelList(true);
		I_EQUAL(subpanels.size(), 0);

		subpanels = db.subPanelList(false);
		I_EQUAL(subpanels.size(), 1);

		//subpanelGenes
		GeneSet subpanel_genes = db.subpanelGenes("some_target_region");
		I_EQUAL(subpanel_genes.count(), 1);
		S_EQUAL(subpanel_genes[0], "WDPCP");

		//subpanelRegions
		BedFile subpanel_regions = db.subpanelRegions("some_target_region");
		I_EQUAL(subpanel_regions.count(), 20);
		I_EQUAL(subpanel_regions.baseCount(), 2508);

		//############################### somatic pathways ###############################

		//getSomaticPathways()
		QByteArrayList pathways = db.getSomaticPathways("SMARCA1");
		I_EQUAL(pathways.count(), 2);
		S_EQUAL(pathways[0], "DNA damage repair");
		S_EQUAL(pathways[1], "chromatin remodeling");

		//getSomaticPathways(gene_symbol)
		pathways = db.getSomaticPathways("SMARCA1");
		I_EQUAL(pathways.count(), 2);
		S_EQUAL(pathways[0], "DNA damage repair");
		S_EQUAL(pathways[1], "chromatin remodeling");

		pathways = db.getSomaticPathways("BRAF");
		I_EQUAL(pathways.count(), 1);
		S_EQUAL(pathways[0], "Hedgehog");

		//getSomaticPathwayGenes(pathway_name)
		genes = db.getSomaticPathwayGenes("DNA damage repair");
		I_EQUAL(genes.count(), 3);
		S_EQUAL(genes[0], "BRCA1");
		S_EQUAL(genes[1], "BRCA2");
		S_EQUAL(genes[2], "SMARCA1");
	}

	//Test for germline report
	inline void report_germline()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		//init NGSD
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in2.sql"));
		LoginManager::login("ahmustm1", "", true);

		QDate report_date = QDate::fromString("2021-02-19", Qt::ISODate);

		//setup
		VariantList variants;
		variants.load(TESTDATA("../cppNGS-TEST/data_in/panel.GSvar"));
		CnvList cnvs;
		cnvs.load(TESTDATA("../cppNGS-TEST/data_in/panel_cnvs_clincnv.tsv"));
		BedpeFile svs;
		svs.load(TESTDATA("/data_in/sv_manta.bedpe"));
		RepeatLocusList res;
		res.load(TESTDATA("data_in/re_calls.vcf"));

		PrsTable prs;
		prs.load(TESTDATA("../cppNGS-TEST/data_in/panel_prs.tsv"));
		ReportSettings report_settings;
		report_settings.report_type = "diagnostic variant";
		report_settings.min_depth = 20;
		report_settings.show_coverage_details = false;
		report_settings.cov_based_on_complete_roi = false;
		report_settings.cov_exon_padding = 20;
		report_settings.recalculate_avg_depth = false;
		report_settings.show_omim_table = false;
		report_settings.show_one_entry_in_omim_table = false;
		report_settings.show_class_details = false;

		FilterCascade filters;
		filters.add(QSharedPointer<FilterBase>(new FilterAlleleFrequency()));
		QMap<QByteArray, QByteArrayList> preferred_transcripts;
		preferred_transcripts.insert("SPG7", QByteArrayList() << "ENST00000268704");
		QSharedPointer<StatisticsService> statistics_service = QSharedPointer<StatisticsService>(new StatisticsServiceLocal());
		GermlineReportGeneratorData data(GenomeBuild::HG38, "NA12878_03", variants, cnvs, svs, res, prs, report_settings, filters, preferred_transcripts, *(statistics_service));
		data.processing_system_roi.load(TESTDATA("../cppNGS-TEST/data_in/panel.bed"));
		data.ps_bam = TESTDATA("../cppNGS-TEST/data_in/panel.bam");
		data.ps_lowcov = TESTDATA("../cppNGS-TEST/data_in/panel_lowcov.bed");

		//############################### TEST 1 - minimal ###############################
		{
			GermlineReportGenerator generator(data, true);
			generator.overrideDate(report_date);

			generator.writeHTML("out/germline_report1.html");
			COMPARE_FILES("out/germline_report1.html", TESTDATA("data_out/germline_report1.html"));
			generator.writeXML("out/germline_report1.xml", "out/germline_report1.html");
			COMPARE_FILES("out/germline_report1.xml", TESTDATA("data_out/germline_report1.xml"));
		}

		//############################### TEST 2 - with variants, with target region, all optional parts enabled ###############################
		{

			report_settings.selected_variants.append(qMakePair(VariantType::SNVS_INDELS, 252)); //small variant - chr16:89531961 G>A (SPG7)
			ReportVariantConfiguration var_conf;
			var_conf.variant_type = VariantType::SNVS_INDELS;
			var_conf.variant_index = 252;
			var_conf.causal = true;
			var_conf.mosaic = true;
			var_conf.de_novo = true;
			var_conf.comp_het = false;
			var_conf.report_type = "diagnostic variant";
			var_conf.rna_info = "n/a";
			report_settings.report_config->set(var_conf);

			report_settings.selected_variants.append(qMakePair(VariantType::SNVS_INDELS, 173)); //small variant - chr13:40793234 C>G (SLC25A15) - manually curated
			var_conf.variant_type = VariantType::SNVS_INDELS;
			var_conf.variant_index = 173;
			var_conf.causal = false;
			var_conf.mosaic = false;
			var_conf.de_novo = false;
			var_conf.comp_het = true;
			var_conf.report_type = "diagnostic variant";
			var_conf.rna_info = "splicing effect validated by RNA dataset";
			var_conf.manual_var = "chr13:40793233-40793233 T>A";
			var_conf.manual_genotype = "hom";
			report_settings.report_config->set(var_conf);

			report_settings.selected_variants.append(qMakePair(VariantType::CNVS, 0)); //CNV - het deletion
			var_conf.variant_type = VariantType::CNVS;
			var_conf.variant_index = 0;
			var_conf.causal = false;
			var_conf.mosaic = false;
			var_conf.de_novo = false;
			var_conf.comp_het = true;
			var_conf.report_type = "diagnostic variant";
			var_conf.rna_info = "no splicing effect found in RNA dataset";
			report_settings.report_config->set(var_conf);

			report_settings.selected_variants.append(qMakePair(VariantType::CNVS, 1)); //CNV - het duplication - manually curated
			var_conf.variant_type = VariantType::CNVS;
			var_conf.variant_index = 1;
			var_conf.causal = false;
			var_conf.mosaic = false;
			var_conf.de_novo = false;
			var_conf.comp_het = true;
			var_conf.report_type = "diagnostic variant";
			var_conf.rna_info = "no splicing effect found in RNA dataset";
			var_conf.manual_cnv_start = "26799369";
			var_conf.manual_cnv_end = "26991734";
			var_conf.manual_cnv_cn = "0";
			var_conf.manual_cnv_hgvs_type = "cnv_type";
			var_conf.manual_cnv_hgvs_suffix = "cnv_suffix";
			report_settings.report_config->set(var_conf);

			report_settings.selected_variants.append(qMakePair(VariantType::SVS, 3)); //SV - Insertion
			var_conf.variant_type = VariantType::SVS;
			var_conf.variant_index = 3;
			var_conf.causal = false;
			var_conf.mosaic = false;
			var_conf.de_novo = false;
			var_conf.comp_het = false;
			var_conf.report_type = "diagnostic variant";
			var_conf.rna_info = "RNA dataset not usable";
			report_settings.report_config->set(var_conf);

			report_settings.selected_variants.append(qMakePair(VariantType::SVS, 16)); //SV - Deletion
			var_conf.variant_type = VariantType::SVS;
			var_conf.variant_index = 16;
			var_conf.causal = false;
			var_conf.mosaic = false;
			var_conf.de_novo = false;
			var_conf.comp_het = false;
			var_conf.report_type = "diagnostic variant";
			var_conf.rna_info = "n/a";
			report_settings.report_config->set(var_conf);

			report_settings.selected_variants.append(qMakePair(VariantType::SVS, 12)); //SV - breakpoint - manually curated
			var_conf.variant_type = VariantType::SVS;
			var_conf.variant_index = 12;
			var_conf.causal = false;
			var_conf.mosaic = false;
			var_conf.de_novo = false;
			var_conf.comp_het = false;
			var_conf.report_type = "diagnostic variant";
			var_conf.rna_info = "n/a";
			var_conf.manual_sv_start = "1584540";
			var_conf.manual_sv_end = "1584550";
			var_conf.manual_sv_genotype = "hom";
			var_conf.manual_sv_start_bnd = "2301860";
			var_conf.manual_sv_end_bnd = "2301870";
			var_conf.manual_sv_hgvs_type = "sv_type";
			var_conf.manual_sv_hgvs_suffix = "sv_suffix";
			report_settings.report_config->set(var_conf);

			report_settings.selected_variants.append(qMakePair(VariantType::RES, 1)); //RE - DAB1
			var_conf.variant_type = VariantType::RES;
			var_conf.variant_index = 1;
			var_conf.causal = false;
			var_conf.mosaic = false;
			var_conf.de_novo = false;
			var_conf.comp_het = false;
			var_conf.report_type = "diagnostic variant";
			var_conf.rna_info = "n/a";
			var_conf.manual_re_allele1 = "15";
			var_conf.manual_re_allele2 = "30";
			report_settings.report_config->set(var_conf);

			OtherCausalVariant causal_variant;
			causal_variant.coordinates = "chr2:123456-789012";
			causal_variant.gene = "EPRS";
			causal_variant.type = "uncalled CNV";
			causal_variant.inheritance = "AR";
			causal_variant.comment = "This is a comment!\n And it has\n multiple lines!\n";
			causal_variant.comment_reviewer1 = "This is a comment from reviewer1!";
			causal_variant.comment_reviewer2 = "This is a comment from reviewer2!";
			report_settings.report_config->setOtherCausalVariant(causal_variant);
			report_settings.select_other_causal_variant = true;
			report_settings.show_refseq_transcripts = true;

			report_settings.show_coverage_details = true;
			report_settings.cov_based_on_complete_roi = true;
			report_settings.recalculate_avg_depth = true;
			report_settings.show_omim_table = true;
			report_settings.show_one_entry_in_omim_table = true;
			report_settings.show_class_details = true;

			data.roi.name = "panel";
			data.roi.regions.load(TESTDATA("../cppNGS-TEST/data_in/panel.bed"));
			data.roi.genes.insert("SLC25A15");
			data.roi.genes.insert("SPG7");
			data.roi.genes.insert("CYP7B1");

			GermlineReportGenerator generator(data, true);
			generator.overrideDate(report_date);
			generator.writeHTML("out/germline_report2.html");
			COMPARE_FILES("out/germline_report2.html", TESTDATA("data_out/germline_report2.html"));
			generator.writeXML("out/germline_report2.xml", "out/germline_report2.html");
			COMPARE_FILES("out/germline_report2.xml", TESTDATA("data_out/germline_report2.xml"));
		}

		//############################### TEST 3 - english ###############################
		{
			report_settings.language = "english";

			GermlineReportGenerator generator(data, true);
			generator.overrideDate(report_date);

			generator.writeHTML("out/germline_report3.html");
			COMPARE_FILES("out/germline_report3.html", TESTDATA("data_out/germline_report3.html"));
		}

		//############################### TEST 4 - report type 'all' ###############################
		{
			report_settings.report_type = "all";
			report_settings.language = "german";

			GermlineReportGenerator generator(data, true);
			generator.overrideDate(report_date);

			generator.writeHTML("out/germline_report4.html");
			COMPARE_FILES("out/germline_report4.html", TESTDATA("data_out/germline_report4.html"));
			generator.writeXML("out/germline_report4.xml", "out/germline_report4.html");
			COMPARE_FILES("out/germline_report4.xml", TESTDATA("data_out/germline_report4.xml"));
		}

		//############################### TEST 5 - evaluation sheet ###############################
		{
			GermlineReportGenerator generator(data, true);
			generator.overrideDate(report_date);

			EvaluationSheetData sheet_data;
			sheet_data.ps_id = "";
			sheet_data.dna_rna = "NA12878";
			sheet_data.reviewer1 = "Jim Raynor";
			sheet_data.review_date1 = report_date;
			sheet_data.reviewer2 = "Sarah Kerrigan";
			sheet_data.review_date2 = QDate::fromString("2021-02-21", Qt::ISODate);
			sheet_data.analysis_scope = "Alles";
			sheet_data.acmg_requested = true;
			sheet_data.acmg_noticeable = true;
			sheet_data.acmg_analyzed = true;
			sheet_data.filtered_by_freq_based_dominant = true;
			sheet_data.filtered_by_freq_based_recessive = false;
			sheet_data.filtered_by_mito = false;
			sheet_data.filtered_by_x_chr = true;
			sheet_data.filtered_by_cnv = true;
			sheet_data.filtered_by_svs = true;
			sheet_data.filtered_by_res = false;
			sheet_data.filtered_by_mosaic = true;
			sheet_data.filtered_by_phenotype = false;
			sheet_data.filtered_by_multisample = true;
			sheet_data.filtered_by_trio_stringent = false;
			sheet_data.filtered_by_trio_relaxed = true;

			generator.writeEvaluationSheet("out/germline_sheet1.html", sheet_data);
			COMPARE_FILES("out/germline_sheet1.html", TESTDATA("data_out/germline_sheet1.html"));
		}

	}

	//Tests for SomaticReportConfiguration and specific somatic variants
	inline void report_somatic()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		QCoreApplication::setApplicationVersion("0.1-cppNGSD-TEST-Version"); //application version (is written into somatic xml report)
		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in1.sql"));
		//log in user
		LoginManager::login("ahmustm1", "", true);



		//Test methods for somatic CNVs in NGSD
		S_EQUAL(db.somaticCnv(1).toString(), "chr4:18000-200000");
		//Test get CNV ID
		S_EQUAL(db.somaticCnvId(CopyNumberVariant(Chromosome("chr7"), 87000, 350000), 5, false), "4");
		S_EQUAL(db.somaticCnvId(CopyNumberVariant(Chromosome("chr7"), 87000, 350000), 1, false), ""); //CNV on callset 1 does not exist
		S_EQUAL(db.somaticCnvId(CopyNumberVariant(Chromosome("chr8"), 87000, 350000), 5, false), ""); //CNV does not exist
		S_EQUAL(db.somaticCnvId(CopyNumberVariant(Chromosome("chr7"), 87040, 350000), 5, false), ""); //CNV does not exist
		S_EQUAL(db.somaticCnvId(CopyNumberVariant(Chromosome("chr7"), 87000, 350005), 5, false), ""); //CNV does not exist
		IS_THROWN(DatabaseException, db.somaticCnvId(CopyNumberVariant(Chromosome("chr7"), 87000, 350000), 1));

		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/somatic_cnvs_clincnv.tsv"));
		int cnv_id =  db.addSomaticCnv(1, cnvs[3], cnvs).toInt();
		CopyNumberVariant res_cnv = db.somaticCnv(cnv_id);
		S_EQUAL(res_cnv.chr().strNormalized(true), "chr11");
		I_EQUAL(res_cnv.start(), 26582421);
		I_EQUAL(res_cnv.end(), 27694430);


		//Test methods for somatic SVs in NGSD:
		BedpeFile svs;
		svs.load(TESTDATA("data_in/somatic_svs_manta.bedpe"));

		S_EQUAL(db.somaticSvId(svs[0], 1, svs), "1");
		S_EQUAL(db.somaticSvId(svs[1], 1, svs), "3");
		S_EQUAL(db.somaticSvId(svs[2], 1, svs), "4");
		S_EQUAL(db.somaticSvId(svs[3], 1, svs), "5");
		S_EQUAL(db.somaticSvId(svs[4], 1, svs), "2");
		S_EQUAL(db.somaticSvId(svs[5], 1, svs, false), "");

		BedpeLine var = db.somaticSv("1", StructuralVariantType::DEL, svs);
		S_EQUAL(var.chr1().strNormalized(true), "chr1");
		I_EQUAL(var.start1(), 33036849);
		I_EQUAL(var.end1(), 33037059);
		S_EQUAL(var.chr2().strNormalized(true), "chr1");
		I_EQUAL(var.start2(), 58631324);
		I_EQUAL(var.end2(), 58631627);

		var = db.somaticSv("3", StructuralVariantType::INS, svs);
		S_EQUAL(var.chr1().strNormalized(true), "chr2");
		I_EQUAL(var.start1(), 71555977);
		I_EQUAL(var.end1(), 71555986);

		var = db.somaticSv("4", StructuralVariantType::INV, svs);
		S_EQUAL(var.chr1().strNormalized(true), "chr6");
		I_EQUAL(var.start1(), 440279);
		I_EQUAL(var.end1(), 440281);
		S_EQUAL(var.chr2().strNormalized(true), "chr6");
		I_EQUAL(var.start2(), 33683482);
		I_EQUAL(var.end2(), 33683482);

		var = db.somaticSv("5", StructuralVariantType::BND, svs);
		S_EQUAL(var.chr1().strNormalized(true), "chr12");
		I_EQUAL(var.start1(), 50807963);
		I_EQUAL(var.end1(), 50807965);
		S_EQUAL(var.chr2().strNormalized(true), "chr22");
		I_EQUAL(var.start2(), 29291555);
		I_EQUAL(var.end2(), 29291557);

		var = db.somaticSv("2", StructuralVariantType::DUP, svs);
		S_EQUAL(var.chr1().strNormalized(true), "chr14");
		I_EQUAL(var.start1(), 55063186);
		I_EQUAL(var.end1(), 55063187);
		S_EQUAL(var.chr2().strNormalized(true), "chr14");
		I_EQUAL(var.start2(), 55176618);
		I_EQUAL(var.end2(), 55176618);

		QString sv_id = db.addSomaticSv(1, svs[6], svs); // add last DUP variant
		S_EQUAL(sv_id, "3");
		var = db.somaticSv("3", StructuralVariantType::DUP, svs);
		S_EQUAL(var.chr1().strNormalized(true), "chr22");
		I_EQUAL(var.start1(), 37944357);
		I_EQUAL(var.end1(), 37944357);
		S_EQUAL(var.chr2().strNormalized(true), "chr22");
		I_EQUAL(var.start2(), 38103385);
		I_EQUAL(var.end2(), 38103385);

		//Test methods for somatic report configuration
		VariantList vl;
		vl.load(TESTDATA("../cppNGSD-TEST/data_in/somatic_report_config.GSvar"));

		cnvs.clear();
		cnvs.load(TESTDATA("data_in/somatic_cnvs_clincnv.tsv"));

		VariantList vl_germl;
		vl_germl.load(TESTDATA("../cppNGSD-TEST/data_in/somatic_report_config_germline.GSvar"));

		//Resolve somatic report configuration ID
		I_EQUAL(db.somaticReportConfigId("5","6"), 3);
		I_EQUAL(db.somaticReportConfigId("5","4000"), 51);
		I_EQUAL(db.somaticReportConfigId("5","10"), -1);

		//Resolve rep conf. creation data
		SomaticReportConfigurationData config_data = db.somaticReportConfigData(51);
		S_EQUAL(config_data.created_by,"Max Mustermann");
		S_EQUAL(config_data.created_date, "05.01.2019 14:06:12");
		S_EQUAL(config_data.last_edit_by, "Sarah Kerrigan");
		S_EQUAL(config_data.last_edit_date, "07.12.2019 17:06:10");
		S_EQUAL(config_data.mtb_xml_upload_date, "27.07.2020 09:20:10");
		S_EQUAL(config_data.target_file, "nowhere.bed");

		//set somatic report configuration in test NGSD, using 2 SNVs
		SomaticReportVariantConfiguration var1;
		var1.variant_index = 1;
		var1.variant_type = VariantType::SNVS_INDELS;
		var1.exclude_artefact = true;
		var1.exclude_high_baf_deviation = true;
		var1.exclude_low_copy_number = true;
		var1.exclude_low_tumor_content = true;
		var1.comment = "This variant is a test variant and shall be excluded.";

		SomaticReportVariantConfiguration var2;
		var2.variant_index = 2;
		var2.variant_type = VariantType::SNVS_INDELS;
		var2.include_variant_alteration = "c.-124A>C";
		var2.include_variant_description = "Testtreiber (bekannt)";
		var2.comment = "known test driver was not included in any db yet.";

		SomaticReportConfiguration som_rep_conf;
		som_rep_conf.addSomaticVariantConfiguration(var1);
		som_rep_conf.addSomaticVariantConfiguration(var2);
		som_rep_conf.setCreatedBy("ahmustm1");
		som_rep_conf.setTargetRegionName("/path/to/somewhere.bed");
		som_rep_conf.setIncludeTumContentByMaxSNV(true);
		som_rep_conf.setIncludeTumContentByClonality(true);
		som_rep_conf.setIncludeTumContentByHistological(true);
		som_rep_conf.setIncludeTumContentByEstimated(true);
		som_rep_conf.setTumContentByEstimated(42);
		som_rep_conf.setMsiStatus(true);
		som_rep_conf.setCnvBurden(true);
		som_rep_conf.setIncludeMutationBurden(true);
		som_rep_conf.setHrdStatement("undeterminable");
		som_rep_conf.setCnvLohCount(12);
		som_rep_conf.setCnvTaiCount(3);
		som_rep_conf.setCnvLstCount(43);
		som_rep_conf.setTmbReferenceText("Median: 1.70 Var/Mbp, Maximum: 10.80 Var/Mbp, Probenanzahl:65 (PMID: 28420421)");
		som_rep_conf.setQuality(QStringList(""));
		som_rep_conf.setFusionsDetected(true);

		som_rep_conf.setCinChromosomes({"chr1", "chr5", "chr9", "chrX", "chrY"});
		som_rep_conf.setLimitations("Due to low coverage we could not detect all variants for gene BRAF.");
		som_rep_conf.setFilterName("somatic");
		QStringList filter_text;
		filter_text << "Variant type	HIGH=frameshift_variant,splice_acceptor_variant,splice_donor_variant,start_lost,start_retained_variant,stop_gained,stop_lost	MODERATE=inframe_deletion,inframe_insertion,missense_variant	LOW=splice_region_variant	MODIFIER=";
		filter_text << "Column match	pattern=promoter	column=regulatory	action=KEEP";
		filter_text << "Filter column empty";
		FilterCascade filters = FilterCascade::fromText(filter_text);
		som_rep_conf.setFilters(filters);


		SomaticReportVariantConfiguration cnv1;
		cnv1.variant_index = 2;
		cnv1.variant_type = VariantType::CNVS;
		cnv1.exclude_artefact = true;
		cnv1.exclude_other_reason = true;
		cnv1.comment = "This test somatic cnv shall be excluded.";
		som_rep_conf.addSomaticVariantConfiguration(cnv1);

		SomaticReportGermlineVariantConfiguration var1_germl, var2_germl;
		var1_germl.variant_index = 2;
		var1_germl.tum_freq = 0.7;
		var1_germl.tum_depth = 1210;

		var2_germl.variant_index = 4;
		var2_germl.tum_freq = 0.68;
		var2_germl.tum_depth = 1022;
		som_rep_conf.addGermlineVariantConfiguration(var1_germl);
		som_rep_conf.addGermlineVariantConfiguration(var2_germl);


		//Check resolving single variant config from report configuration
		S_EQUAL(som_rep_conf.variantConfig(2, VariantType::SNVS_INDELS).include_variant_alteration, "c.-124A>C");
		S_EQUAL(som_rep_conf.variantConfig(2, VariantType::SNVS_INDELS).include_variant_description, "Testtreiber (bekannt)");
		S_EQUAL(som_rep_conf.variantConfig(2, VariantType::SNVS_INDELS).comment, "known test driver was not included in any db yet.");
		I_EQUAL(som_rep_conf.variantConfig(2, VariantType::SNVS_INDELS).variant_index, 2);
		I_EQUAL(som_rep_conf.variantConfig(2, VariantType::SNVS_INDELS).variant_type, VariantType::SNVS_INDELS);

		IS_TRUE(som_rep_conf.variantConfig(2, VariantType::CNVS).exclude_artefact);
		IS_TRUE(som_rep_conf.variantConfig(2, VariantType::CNVS).exclude_other_reason);
		IS_FALSE(som_rep_conf.variantConfig(2, VariantType::CNVS).exclude_high_baf_deviation);
		IS_FALSE(som_rep_conf.variantConfig(2, VariantType::CNVS).exclude_low_copy_number);
		IS_FALSE(som_rep_conf.variantConfig(2, VariantType::CNVS).exclude_low_tumor_content);
		S_EQUAL(som_rep_conf.variantConfig(2, VariantType::CNVS).include_variant_alteration, "");
		S_EQUAL(som_rep_conf.variantConfig(2, VariantType::CNVS).include_variant_description, "");
		S_EQUAL(som_rep_conf.variantConfig(2, VariantType::CNVS).comment, "This test somatic cnv shall be excluded.");
		I_EQUAL(som_rep_conf.variantConfig(2, VariantType::CNVS).variant_index, 2);
		I_EQUAL(som_rep_conf.variantConfig(2, VariantType::CNVS).variant_type, VariantType::CNVS);


		QString t_ps_id = db.processedSampleId("NA12345_01");
		QString n_ps_id = db.processedSampleId("NA12123_04");
		int config_id = db.setSomaticReportConfig(t_ps_id, n_ps_id, som_rep_conf, vl, cnvs, svs, vl_germl, "ahmustm1"); //id will be 52 in test NGSD

		//test changing existing variant config:

		SomaticReportVariantConfiguration var2_changed;
		var2_changed.variant_index = 2;
		var2_changed.variant_type = VariantType::SNVS_INDELS;
		var2_changed.include_variant_alteration = "c.-124A>C";
		var2_changed.include_variant_description = "Testtreiber (bekannt)";
		var2_changed.comment = "known test driver was not included in any db yet. Now published in NCBI:XYZ.";

		som_rep_conf.addSomaticVariantConfiguration(var2_changed);

		config_id = db.setSomaticReportConfig(t_ps_id, n_ps_id, som_rep_conf, vl, cnvs, svs, vl_germl, "ahmustm1"); //id will still be 52 in test NGSD

		S_EQUAL(som_rep_conf.variantConfig(2, VariantType::SNVS_INDELS).comment, "known test driver was not included in any db yet. Now published in NCBI:XYZ.");

		QStringList messages = {};

		//Test resolving report config
		SomaticReportConfiguration res_config = db.somaticReportConfig(t_ps_id, n_ps_id, vl, cnvs, svs, vl_germl, messages);
		IS_TRUE(res_config.includeTumContentByMaxSNV());
		IS_TRUE(res_config.includeTumContentByClonality());
		IS_TRUE(res_config.includeTumContentByHistological());
		IS_TRUE(res_config.includeTumContentByEstimated());
		I_EQUAL(res_config.tumContentByEstimated(), 42);
		IS_TRUE(res_config.msiStatus());
		IS_TRUE(res_config.cnvBurden());
		IS_TRUE(res_config.includeMutationBurden());
		S_EQUAL(res_config.hrdStatement(), "undeterminable");
		I_EQUAL(res_config.cnvLohCount(), 12);
		I_EQUAL(res_config.cnvTaiCount(), 3);
		I_EQUAL(res_config.cnvLstCount(), 43);

		S_EQUAL(res_config.tmbReferenceText(), "Median: 1.70 Var/Mbp, Maximum: 10.80 Var/Mbp, Probenanzahl:65 (PMID: 28420421)");
		I_EQUAL(res_config.quality().count(), 0);
		IS_TRUE(res_config.fusionsDetected());
		S_EQUAL(res_config.cinChromosomes().join(','), "chr1,chr5,chr9,chrX,chrY");
		IS_THROWN(ArgumentException, som_rep_conf.setCinChromosomes({"chr1", "chr24"}));
		S_EQUAL(res_config.limitations(), "Due to low coverage we could not detect all variants for gene BRAF.");
		S_EQUAL(res_config.filterName(), "somatic");
		IS_TRUE(res_config.filters() == filters)

		QStringList res_filter_text = res_config.filters().toText();
		for (int i=0; i< filter_text.count(); i++)
		{
			S_EQUAL(res_filter_text[i].trimmed(), filter_text[i]);
		}

		//Test variants included in resolved report
		QList<SomaticReportVariantConfiguration> res =  res_config.variantConfig();
		const SomaticReportVariantConfiguration& res0 = res.at(0);

		I_EQUAL(res.count(), 3);

		I_EQUAL(res0.variant_index, 1);
		IS_TRUE(res0.exclude_artefact);
		IS_TRUE(res0.exclude_low_tumor_content);
		IS_TRUE(res0.exclude_low_copy_number);
		IS_TRUE(res0.exclude_high_baf_deviation);
		IS_FALSE(res0.exclude_other_reason);
		S_EQUAL(res0.include_variant_alteration, "");
		S_EQUAL(res0.include_variant_description, "");
		S_EQUAL(res0.comment, "This variant is a test variant and shall be excluded.");
		IS_FALSE(res0.showInReport());

		const SomaticReportVariantConfiguration& res1 = res.at(1);
		I_EQUAL(res1.variant_index, 2);
		IS_FALSE(res1.exclude_artefact);
		IS_FALSE(res1.exclude_low_tumor_content);
		IS_FALSE(res1.exclude_low_copy_number);
		IS_FALSE(res1.exclude_high_baf_deviation);
		IS_FALSE(res1.exclude_other_reason);
		S_EQUAL(res1.include_variant_alteration, "c.-124A>C");
		S_EQUAL(res1.include_variant_description, "Testtreiber (bekannt)");
		S_EQUAL(res1.comment, "known test driver was not included in any db yet. Now published in NCBI:XYZ.");
		IS_TRUE(res1.showInReport());

		//Test germline variants included in resolved report
		QList<SomaticReportGermlineVariantConfiguration> res_germl = res_config.variantConfigGermline();
		I_EQUAL(res_germl.count(), 2);
		I_EQUAL(res_germl[0].variant_index, 2);
		F_EQUAL(res_germl[0].tum_freq, 0.7);
		F_EQUAL(res_germl[0].tum_depth, 1210);

		I_EQUAL(res_germl[1].variant_index, 4);
		F_EQUAL(res_germl[1].tum_freq, 0.68);
		I_EQUAL(res_germl[1].tum_depth, 1022);



		SomaticReportConfigurationData config_data_1 = db.somaticReportConfigData(config_id);
		S_EQUAL(config_data_1.created_by, "Max Mustermann");
		S_EQUAL(config_data_1.last_edit_by, "Max Mustermann");
		S_EQUAL(config_data_1.target_file, "somewhere.bed");
		S_EQUAL(config_data_1.mtb_xml_upload_date, "");

		//set
		db.setSomaticMtbXmlUpload(config_id);

		IS_TRUE(db.somaticReportConfigData(config_id).mtb_xml_upload_date != "");


		//Update somatic report configuration (by other user), should update target_file and last_edits
		som_rep_conf.setTargetRegionName("/path/to/somewhere/else.bed");
		som_rep_conf.setIncludeTumContentByMaxSNV(false);
		som_rep_conf.setIncludeTumContentByClonality(false);
		som_rep_conf.setIncludeTumContentByHistological(false);
		som_rep_conf.setIncludeTumContentByEstimated(false);
		som_rep_conf.setTumContentByEstimated(31);
		som_rep_conf.setMsiStatus(false);
		som_rep_conf.setCnvBurden(false);
		som_rep_conf.setIncludeMutationBurden(false);

		som_rep_conf.setHrdStatement("proof");
		som_rep_conf.setCnvLohCount(9);
		som_rep_conf.setCnvTaiCount(1);
		som_rep_conf.setCnvLstCount(23);


		som_rep_conf.setTmbReferenceText("An alternative tmb reference value.");
		som_rep_conf.setQuality(QStringList("DNA quantity too low"));
		som_rep_conf.setFusionsDetected(false);
		som_rep_conf.setCinChromosomes({"chr10","chr21"});
		som_rep_conf.setLimitations("With German umlauts: ???????");
		som_rep_conf.setFilterName("");

		db.setSomaticReportConfig(t_ps_id, n_ps_id, som_rep_conf, vl, cnvs, svs, vl_germl, "ahkerra1");

		SomaticReportConfiguration res_config_2 = db.somaticReportConfig(t_ps_id, n_ps_id, vl, cnvs, svs, vl_germl, messages);
		IS_FALSE(res_config_2.includeTumContentByMaxSNV());
		IS_FALSE(res_config_2.includeTumContentByClonality());
		IS_FALSE(res_config_2.includeTumContentByHistological());
		IS_FALSE(res_config_2.includeTumContentByEstimated());
		I_EQUAL(res_config_2.tumContentByEstimated(), 31);
		IS_FALSE(res_config_2.msiStatus());
		IS_FALSE(res_config_2.cnvBurden());
		IS_FALSE(res_config_2.includeMutationBurden());

		S_EQUAL(res_config_2.hrdStatement(), "proof");
		I_EQUAL(res_config_2.cnvLohCount(), 9);
		I_EQUAL(res_config_2.cnvTaiCount(), 1);
		I_EQUAL(res_config_2.cnvLstCount(), 23);

		S_EQUAL(res_config_2.tmbReferenceText(), "An alternative tmb reference value.");
		S_EQUAL(res_config_2.quality()[0], "DNA quantity too low");
		IS_FALSE(res_config_2.fusionsDetected());
		S_EQUAL(res_config_2.cinChromosomes().join(','), "chr10,chr21");
		S_EQUAL(res_config_2.limitations(), "With German umlauts: ???????");
		S_EQUAL(res_config_2.filterName(), "");

		SomaticReportConfigurationData config_data_2 =  db.somaticReportConfigData(config_id);
		S_EQUAL(config_data_2.created_by, "Max Mustermann");
		S_EQUAL(config_data_2.last_edit_by, "Sarah Kerrigan");
		S_EQUAL(config_data_2.target_file, "else.bed");
		IS_TRUE(config_data_2.created_date == config_data_1.created_date);
		IS_TRUE(config_data_2.last_edit_date != "");

		//report config in case of no target file
		som_rep_conf.setTargetRegionName("");
		db.setSomaticReportConfig(t_ps_id, n_ps_id, som_rep_conf, vl, cnvs, svs, vl_germl, "ahkerra1");

		SomaticReportConfigurationData config_data_3 = db.somaticReportConfigData(config_id);
		S_EQUAL(config_data_3.target_file, "");

		//Test CNV report configuration
		const SomaticReportVariantConfiguration& res2 = res.at(2);
		I_EQUAL(res2.variant_index, 2);
		IS_TRUE(res2.exclude_artefact);
		IS_FALSE(res2.exclude_low_tumor_content);
		IS_FALSE(res2.exclude_high_baf_deviation);
		IS_TRUE(res2.exclude_other_reason);
		S_EQUAL(res2.comment, "This test somatic cnv shall be excluded.");

		//Delete a somatic report configuration
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_report_configuration").toInt(), 3);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_report_configuration_cnv").toInt(), 2); //one CNV is already inserted by NGSD init.
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_report_configuration_variant").toInt(), 2);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_report_configuration_germl_var").toInt(), 2);

		db.deleteSomaticReportConfig(config_id);

		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_report_configuration").toInt(), 2);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_report_configuration_cnv").toInt(), 1);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_report_configuration_variant").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_report_configuration_germl_var").toInt(), 0);


		//Delete Variants
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_cnv").toInt(), 4);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_cnv_callset").toInt(), 2);
		I_EQUAL(db.getValue("SELECT count(*) FROM detected_somatic_variant").toInt(), 1);
		db.deleteSomaticVariants("4000", "3999");
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_cnv").toInt(), 2);
		I_EQUAL(db.getValue("SELECT count(*) FROM somatic_cnv_callset").toInt(), 1);
		I_EQUAL(db.getValue("SELECT count(*) FROM detected_somatic_variant").toInt(), 0);


		//Test somatic xml report
		SomaticReportSettings settings;
		settings.report_config = res_config;
		settings.tumor_ps = "DX184894_01";
		settings.normal_ps = "DX184263_01";
		settings.target_region_filter.name = "SureSelect Somatic vTEST";
		settings.target_region_filter.genes = GeneSet::createFromFile(TESTDATA("../cppNGSD-TEST/data_in/ssSC_test_genes.txt"));;
		settings.target_region_filter.regions.load(TESTDATA("../cppNGSD-TEST/data_in/ssSC_test.bed"));

		//Test somatic XML report

		VariantList vl_filtered = SomaticReportSettings::filterVariants(vl, settings);
		VariantList vl_germl_filtered =  SomaticReportSettings::filterGermlineVariants(vl_germl, settings);
		CnvList cnvs_filtered = SomaticReportSettings::filterCnvs(cnvs,settings);
		SomaticXmlReportGeneratorData xml_data(GenomeBuild::HG19, settings, vl_filtered, vl_germl_filtered, cnvs_filtered);


		IS_THROWN(ArgumentException, xml_data.check());

		xml_data.msi_unstable_percent = 12.74;
		xml_data.tumor_content_histology = 0.6;
		xml_data.tumor_mutation_burden = 17.3;
		xml_data.tumor_content_clonality = 0.8;
		xml_data.tumor_content_snvs = 0.73;

		xml_data.rtf_part_summary = "I am the summary part of the RTF report";
		xml_data.rtf_part_relevant_variants = "relevant SNVs and INDELs";
		xml_data.rtf_part_unclear_variants = "unclear SNVs";
		xml_data.rtf_part_cnvs = "chromosomal aberrations";
		xml_data.rtf_part_svs = "Fusions";
		xml_data.rtf_part_pharmacogenetics = "RTF pharmacogenomics table";
                xml_data.rtf_part_general_info = "general meta data";
		xml_data.rtf_part_igv_screenshot = "89504E470D0A1A0A0000000D4948445200000002000000020802000000FDD49A73000000097048597300002E2300002E230178A53F76000000164944415408D763606060686E6E66F8FFFFFF7F0606001FCD0586CC377DEC0000000049454E44AE426082";
		xml_data.rtf_part_mtb_summary = "MTB summary";
                xml_data.rtf_part_hla_summary = "HLA summary";

		QSharedPointer<QFile> out_file = Helper::openFileForWriting("out/somatic_report.xml");
		SomaticXmlReportGenerator::generateXML(xml_data, out_file, db, true);
		out_file->close();

		COMPARE_FILES("out/somatic_report.xml", TESTDATA("data_out/somatic_report.xml"));


		I_EQUAL(db.getSomaticViccId(Variant("chr13", 32929387, 32929387, "T", "C")), 1);
		I_EQUAL(db.getSomaticViccId(Variant("chr15", 43707808, 43707808, "A", "T")), 2);
		I_EQUAL(db.getSomaticViccId(Variant("chr17", 43707815, 43707815, "A", "T")), -1);

		//Variant exists but not VICC interpretation
		IS_THROWN(DatabaseException, db.getSomaticViccData(Variant("chr5", 112175770, 112175770, "G", "A")) );
		//Variant does not exist
		IS_THROWN(DatabaseException, db.getSomaticViccData(Variant("chr1", 112175770, 112175770, "C", "A")) );



		//somatic Variant Interpretation for Cancer Consortium
		SomaticViccData vicc_data1 = db.getSomaticViccData(Variant("chr13", 32929387, 32929387, "T", "C"));

		I_EQUAL(vicc_data1.null_mutation_in_tsg, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(vicc_data1.known_oncogenic_aa, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data1.strong_cancerhotspot, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data1.located_in_canerhotspot, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(vicc_data1.absent_from_controls, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(vicc_data1.protein_length_change, SomaticViccData::State::NOT_APPLICABLE);
		I_EQUAL(vicc_data1.other_aa_known_oncogenic, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(vicc_data1.weak_cancerhotspot, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data1.computational_evidence, SomaticViccData::State::NOT_APPLICABLE);
		I_EQUAL(vicc_data1.mutation_in_gene_with_etiology, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data1.very_weak_cancerhotspot, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(vicc_data1.very_high_maf, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data1.benign_functional_studies, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data1.high_maf, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data1.benign_computational_evidence, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data1.synonymous_mutation, SomaticViccData::State::NOT_APPLICABLE);
		S_EQUAL(vicc_data1.comment, "this variant was evaluated as an oncogenic variant");
		S_EQUAL(db.getValue("SELECT classification FROM somatic_vicc_interpretation WHERE id=" + QString::number(db.getSomaticViccId(Variant("chr13", 32929387, 32929387, "T", "C")))).toString(), "ONCOGENIC");
		S_EQUAL(db.getValue("SELECT classification FROM somatic_vicc_interpretation WHERE id=" + QString::number(db.getSomaticViccId(Variant("chr13", 32929387, 32929387, "T", "C")))).toString(), SomaticVariantInterpreter::viccScoreAsString(vicc_data1));


		S_EQUAL(vicc_data1.created_by, "ahmustm1");
		S_EQUAL(vicc_data1.created_at.toString("yyyy-MM-dd hh:mm:ss"), "2020-11-05 13:06:13");
		S_EQUAL(vicc_data1.last_updated_by, "ahkerra1");
		S_EQUAL(vicc_data1.last_updated_at.toString("yyyy-MM-dd hh:mm:ss"), "2020-12-07 11:06:10");

		SomaticViccData vicc_data2 = db.getSomaticViccData( Variant("chr15", 43707808, 43707808, "A", "T" ) );
		S_EQUAL(vicc_data2.comment, "this variant was evaluated as variant of unclear significance");
		S_EQUAL(vicc_data2.last_updated_by, "ahkerra1");
		S_EQUAL(vicc_data2.last_updated_at.toString("yyyy-MM-dd hh:mm:ss"),  "2020-12-08 13:45:11");
		S_EQUAL(db.getValue("SELECT classification FROM somatic_vicc_interpretation WHERE id=" + QString::number(db.getSomaticViccId(Variant("chr15", 43707808, 43707808, "A", "T")))).toString(), "UNCERTAIN_SIGNIFICANCE");


		//Update somatic VICC data in NGSD
		SomaticViccData vicc_update = vicc_data2;
		vicc_update.null_mutation_in_tsg = SomaticViccData::State::VICC_TRUE;
		vicc_update.known_oncogenic_aa = SomaticViccData::State::VICC_TRUE;
		vicc_update.very_high_maf = SomaticViccData::State::VICC_FALSE;
		vicc_update.benign_functional_studies =SomaticViccData::State::VICC_FALSE;
		vicc_update.high_maf = SomaticViccData::State::VICC_FALSE;
		vicc_update.benign_computational_evidence = SomaticViccData::State::VICC_FALSE;
		vicc_update.synonymous_mutation = SomaticViccData::State::VICC_FALSE;
		vicc_update.comment = "This variant was reevaluated as oncogenic!";

		//known oncogneic amino acid change: located_in_cancerhotspot and other_aa_is_oncogenic cannot apply
		IS_THROWN(ArgumentException, db.setSomaticViccData(Variant("chr15", 43707808, 43707808, "A", "T" ), vicc_update, "ahmustm1") );
		vicc_update.located_in_canerhotspot = SomaticViccData::State::NOT_APPLICABLE;
		vicc_update.other_aa_known_oncogenic = SomaticViccData::State::NOT_APPLICABLE;
		db.setSomaticViccData(Variant("chr15", 43707808, 43707808, "A", "T" ), vicc_update, "ahmustm1");

		vicc_data2 = db.getSomaticViccData( Variant("chr15", 43707808, 43707808, "A", "T") );

		I_EQUAL(vicc_data2.null_mutation_in_tsg, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(vicc_data2.known_oncogenic_aa, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(vicc_data2.strong_cancerhotspot, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data2.located_in_canerhotspot, SomaticViccData::State::NOT_APPLICABLE);
		I_EQUAL(vicc_data2.absent_from_controls, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(vicc_data2.protein_length_change, SomaticViccData::State::NOT_APPLICABLE);
		I_EQUAL(vicc_data2.other_aa_known_oncogenic, SomaticViccData::State::NOT_APPLICABLE);
		I_EQUAL(vicc_data2.weak_cancerhotspot, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data2.computational_evidence, SomaticViccData::State::NOT_APPLICABLE);
		I_EQUAL(vicc_data2.mutation_in_gene_with_etiology, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data2.very_weak_cancerhotspot, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(vicc_data2.very_high_maf, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data2.benign_functional_studies, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data2.high_maf, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data2.benign_computational_evidence, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(vicc_data2.synonymous_mutation, SomaticViccData::State::VICC_FALSE);
		S_EQUAL(vicc_data2.comment, "This variant was reevaluated as oncogenic!");
		S_EQUAL(db.getValue("SELECT classification FROM somatic_vicc_interpretation WHERE id=" + QString::number(db.getSomaticViccId(Variant("chr15", 43707808, 43707808, "A", "T")))).toString(), "ONCOGENIC");
		S_EQUAL(vicc_data2.last_updated_by, "ahmustm1");
		IS_TRUE(vicc_data2.last_updated_at.toString("yyyy-MM-dd hh:mm:ss") != "2020-12-08 13:45:11");
		//Insert new somatic VICC interpretation
		SomaticViccData new_vicc_data;
		new_vicc_data.null_mutation_in_tsg = SomaticViccData::State::VICC_FALSE;
		new_vicc_data.oncogenic_functional_studies = SomaticViccData::State::VICC_FALSE;
		new_vicc_data.protein_length_change = SomaticViccData::State::VICC_FALSE;
		new_vicc_data.computational_evidence = SomaticViccData::State::VICC_TRUE;
		new_vicc_data.high_maf = SomaticViccData::State::VICC_TRUE;
		new_vicc_data.benign_functional_studies = SomaticViccData::State::VICC_TRUE;
		new_vicc_data.synonymous_mutation = SomaticViccData::State::VICC_TRUE;
		new_vicc_data.comment = "This is a benign somatic variant.";
		db.setSomaticViccData(Variant("chr17", 59763465, 59763465, "T", "C"), new_vicc_data, "ahmustm1");

		SomaticViccData new_vicc_result = db.getSomaticViccData(Variant("chr17", 59763465, 59763465, "T", "C") );
		I_EQUAL(new_vicc_result.null_mutation_in_tsg, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(new_vicc_result.oncogenic_functional_studies, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(new_vicc_result.protein_length_change, SomaticViccData::State::VICC_FALSE);
		I_EQUAL(new_vicc_result.computational_evidence, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(new_vicc_result.high_maf, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(new_vicc_result.benign_functional_studies, SomaticViccData::State::VICC_TRUE);
		I_EQUAL(new_vicc_result.synonymous_mutation, SomaticViccData::State::VICC_TRUE);
		S_EQUAL(new_vicc_result.comment, "This is a benign somatic variant.");

		I_EQUAL(db.getSomaticViccId(Variant("chr17", 59763465, 59763465, "T", "C")), 5); //id of new inserted vicc data set is 5 in TEST-NGSD

		//When updating one variant, all other data sets must not change (test case initially created for Bugfix)
		db.setSomaticViccData(Variant("chr17", 59763465, 59763465, "T", "C"), new_vicc_data, "ahkerra1");
		SomaticViccData vicc_data3 = db.getSomaticViccData( Variant("chr15", 43707808, 43707808, "A", "T") );
		S_EQUAL(vicc_data3.comment, vicc_data2.comment);



		//somatic CNV gene role
		I_EQUAL(db.getSomaticGeneRoleId("EPRS"), 3);
		I_EQUAL(db.getSomaticGeneRoleId("BRCA2"), 1);
		I_EQUAL(db.getSomaticGeneRoleId("PTGS2"), 2);
		I_EQUAL(db.getSomaticGeneRoleId("FOXP1"), -1);
		I_EQUAL(db.getSomaticGeneRoleId("ASDFJKL"), -1);


		IS_THROWN(DatabaseException, db.getSomaticGeneRole("FOXP1", true));
		IS_THROWN(DatabaseException, db.getSomaticGeneRole("ASDFJKL", true));

		SomaticGeneRole gene_role_res1 =  db.getSomaticGeneRole("PTGS2", true);
		S_EQUAL(gene_role_res1.gene, "PTGS2");
		I_EQUAL(gene_role_res1.role, SomaticGeneRole::Role::AMBIGUOUS);
		IS_FALSE(gene_role_res1.high_evidence);
		S_EQUAL(gene_role_res1.comment, "comment on gene");

		SomaticGeneRole gene_role_res2 = db.getSomaticGeneRole("BRCA2");
		S_EQUAL(gene_role_res2.gene, "BRCA2");
		I_EQUAL(gene_role_res2.role, SomaticGeneRole::Role::LOSS_OF_FUNCTION);
		IS_TRUE(gene_role_res2.high_evidence);
		S_EQUAL(gene_role_res2.comment, "test comment");

		SomaticGeneRole gene_role_res3= db.getSomaticGeneRole("EPRS");
		S_EQUAL(gene_role_res3.gene, "EPRS");
		I_EQUAL(gene_role_res3.role, SomaticGeneRole::Role::ACTIVATING);
		IS_TRUE(gene_role_res3.high_evidence);
		S_EQUAL(gene_role_res3.comment, "");

		SomaticGeneRole gene_role_res4 = db.getSomaticGeneRole("ASDFJKL"); //empty
		S_EQUAL(gene_role_res4.gene, "");


		//Update gene Roles
		gene_role_res1.role = SomaticGeneRole::Role::ACTIVATING;
		gene_role_res1.high_evidence = true;
		gene_role_res1.comment = "comment update";
		db.setSomaticGeneRole(gene_role_res1);
		gene_role_res1 =  db.getSomaticGeneRole("PTGS2", true);
		S_EQUAL(gene_role_res1.gene, "PTGS2");
		I_EQUAL(gene_role_res1.role, SomaticGeneRole::Role::ACTIVATING);
		IS_TRUE(gene_role_res1.high_evidence);
		S_EQUAL(gene_role_res1.comment, "comment update");
		I_EQUAL(db.getSomaticGeneRoleId("PTGS2"), 2);

		//insert new gene roles: gene symbol does not exist
		SomaticGeneRole role_for_ins1;
		role_for_ins1.gene = "NOTEXISTING";
		role_for_ins1.high_evidence = true;
		role_for_ins1.role = SomaticGeneRole::Role::AMBIGUOUS;
		IS_THROWN(DatabaseException, db.setSomaticGeneRole(role_for_ins1));

		//insert new gene role
		SomaticGeneRole role_for_ins2;
		role_for_ins2.gene = "FOXP1";
		role_for_ins2.role = SomaticGeneRole::Role::ACTIVATING;
		role_for_ins2.comment = "newly inserted test role";
		db.setSomaticGeneRole(role_for_ins2);
		SomaticGeneRole gene_role_res5 = db.getSomaticGeneRole("FOXP1");
		S_EQUAL(gene_role_res5.gene, "FOXP1");
		I_EQUAL(gene_role_res5.role, SomaticGeneRole::Role::ACTIVATING);
		IS_FALSE(gene_role_res5.high_evidence);
		S_EQUAL(gene_role_res5.comment, "newly inserted test role");
		I_EQUAL(db.getSomaticGeneRoleId("FOXP1"), 4);


		//delete gene role
		IS_THROWN( ProgrammingException, db.deleteSomaticGeneRole("NONEXISTING") );
		IS_THROWN( ProgrammingException, db.deleteSomaticGeneRole("SUFU") );

		db.deleteSomaticGeneRole("PTGS2");
		SqlQuery query = db.getQuery();
		query.exec("SELECT id FROM somatic_gene_role"); //3 remaining rows
		I_EQUAL(query.size(), 3);
		I_EQUAL(-1, db.getSomaticGeneRoleId("PTGS2"));
	}

	// Test the somatic report RTF generation
	void test_somatic_rtf_1()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in4.sql"));
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in4_gene_exons.sql"));
		SqlQuery query = db.getQuery();
		query.prepare("UPDATE processed_sample SET folder_override = '" + TESTDATA("data_in/somatic/Sample_DNA123456_01/") + "' WHERE id = 4004");
		query.exec();

		QString tumor_sample = TESTDATA("data_in/somatic/Somatic_DNA123456_01-NA12878_03/DNA123456_01-NA12878_03.GSvar");
		QString normal_sample = TESTDATA("data_in/somatic/Sample_NA12878_03/NA12878_03.GSvar");

		VariantList vl;
		vl.load(tumor_sample);

		VariantList control_tissue_variants;
		control_tissue_variants.load(normal_sample);


		QSharedPointer<FileLocationProvider> flp = QSharedPointer<FileLocationProviderLocal>(new FileLocationProviderLocal(tumor_sample, vl.getSampleHeader(), AnalysisType::SOMATIC_SINGLESAMPLE)); //vl.type()
		FileLocation cnv_loc = flp->getAnalysisCnvFile();

		CnvList cnv_list;
		cnv_list.load(cnv_loc.filename);

		BedpeFile svs;
		svs.load(flp->getAnalysisSvFile().filename);

		QStringList messages;

		SomaticReportSettings somatic_report_settings;
		SomaticReportConfiguration somatic_report_config = db.somaticReportConfig(db.processedSampleId("DNA123456_01"), db.processedSampleId("NA12878_03"), vl, cnv_list, svs, control_tissue_variants, messages);
		somatic_report_settings.report_config = somatic_report_config;

		somatic_report_settings.tumor_ps = "DNA123456_01";
		somatic_report_settings.normal_ps = "NA12878_03";
		somatic_report_settings.msi_file = flp->getSomaticMsiFile().filename;
		somatic_report_settings.viral_file = TESTDATA("data_in/somatic/Sample_DNA123456_01/DNA123456_01_viral_1.tsv");

		S_EQUAL(db.processedSampleId("DNA123456_01"), "4004");

		somatic_report_settings.report_config.setIncludeTumContentByHistological(true);
		somatic_report_settings.report_config.setIncludeTumContentByClonality(true);
		somatic_report_settings.report_config.setIncludeTumContentByMaxSNV(true);
		somatic_report_settings.report_config.setIncludeTumContentByEstimated(false);
		somatic_report_settings.report_config.setMsiStatus(true);
		somatic_report_settings.report_config.setCnvBurden(true);
		somatic_report_settings.report_config.setIncludeMutationBurden(true);
		somatic_report_settings.report_config.setHrdStatement("proof");
		somatic_report_settings.report_config.setCnvLohCount(12);
		somatic_report_settings.report_config.setCnvTaiCount(3);
		somatic_report_settings.report_config.setCnvLstCount(33);
		somatic_report_settings.report_config.setTmbReferenceText("Test reference text for the tmb of this analysis!");
		somatic_report_settings.report_config.setEvaluationDate(QDate(2022,12,1));
		somatic_report_settings.report_config.setLimitations("This text should appear as limitations!");
		//preferred transcripts
		somatic_report_settings.preferred_transcripts = db.getPreferredTranscripts();

		OntologyTermCollection obo_terms("://Resources/so-xp_3_1_0.obo",true);
		QList<QByteArray> ids;
		ids << obo_terms.childIDs("SO:0001580",true); //coding variants
		ids << obo_terms.childIDs("SO:0001568",true); //splicing variants
		foreach(const QByteArray& id, ids)
		{
			somatic_report_settings.obo_terms_coding_splicing.add(obo_terms.getByID(id));
		}

		TargetRegionInfo target_region = TargetRegionInfo();
		target_region.name = "VirtualTumorPanel_v5_exon20_ahott1a1_20230505";
		target_region.genes = db.subpanelGenes(target_region.name);
		target_region.regions = db.subpanelRegions(target_region.name);
		somatic_report_settings.target_region_filter = target_region;
		QStringList quality;
		quality.append("DNA quantity too low");
		quality.append("heterogeneous sample");
		somatic_report_settings.report_config.setQuality(quality);

		SomaticReportHelper report(GenomeBuild::HG38, vl, cnv_list, svs, control_tissue_variants, somatic_report_settings, true);
		report.storeRtf("out/somatic_report_tumor_normal_1.rtf");

		COMPARE_FILES("out/somatic_report_tumor_normal_1.rtf", TESTDATA("data_out/somatic_report_tumor_normal_1.rtf"));

		// test xml generation is legal:
		SomaticXmlReportGenerator xml_report;
		QSharedPointer<QFile> out_xml = Helper::openFileForWriting("out/somatic_report_tumor_normal_1.xml");
		xml_report.generateXML(report.getXmlData(), out_xml, db, true);

		COMPARE_FILES("out/somatic_report_tumor_normal_1.xml", TESTDATA("data_out/somatic_report_tumor_normal_1.xml"));

	}

	void test_somatic_rtf_2()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in4.sql"));
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in4_gene_exons.sql"));
		SqlQuery query = db.getQuery();
		query.prepare("UPDATE processed_sample SET folder_override = '" + TESTDATA("data_in/somatic/Sample_DNA123456_01/") + "' WHERE id = 4004");
		query.exec();

		QString tumor_sample = TESTDATA("data_in/somatic/Somatic_DNA123456_01-NA12878_03/DNA123456_01-NA12878_03.GSvar");
		QString normal_sample = TESTDATA("data_in/somatic/Sample_NA12878_03/NA12878_03.GSvar");

		VariantList vl;
		vl.load(tumor_sample);

		VariantList control_tissue_variants;
		control_tissue_variants.load(normal_sample);


		QSharedPointer<FileLocationProvider> flp = QSharedPointer<FileLocationProviderLocal>(new FileLocationProviderLocal(tumor_sample, vl.getSampleHeader(), AnalysisType::SOMATIC_SINGLESAMPLE)); //vl.type()
		FileLocation cnv_loc = flp->getAnalysisCnvFile();

		CnvList cnv_list;
		cnv_list.load(cnv_loc.filename);

		BedpeFile svs;
		svs.load(flp->getAnalysisSvFile().filename);

		query.prepare("DELETE FROM somatic_report_configuration_sv  WHERE id > 0");
		query.exec();

		QStringList messages;

		SomaticReportSettings somatic_report_settings;
		SomaticReportConfiguration somatic_report_config = db.somaticReportConfig(db.processedSampleId("DNA123456_01"), db.processedSampleId("NA12878_03"), vl, cnv_list, svs, control_tissue_variants, messages);
		somatic_report_settings.report_config = somatic_report_config;

		somatic_report_settings.tumor_ps = "DNA123456_01";
		somatic_report_settings.normal_ps = "NA12878_03";
		somatic_report_settings.msi_file = flp->getSomaticMsiFile().filename;
		somatic_report_settings.viral_file = TESTDATA("data_in/somatic/Sample_DNA123456_01/DNA123456_01_viral_2.tsv");

		somatic_report_settings.sbs_signature = TESTDATA("data_in/somatic/Somatic_DNA123456_01-NA12878_03/snv_signatures/De_Novo_map_to_COSMIC_SBS96.csv");
		somatic_report_settings.dbs_signature = TESTDATA("data_in/somatic/Somatic_DNA123456_01-NA12878_03/snv_signatures/De_Novo_map_to_COSMIC_DBS78.csv");
		somatic_report_settings.id_signature = TESTDATA("data_in/somatic/Somatic_DNA123456_01-NA12878_03/snv_signatures/De_Novo_map_to_COSMIC_ID83.tsv");
		somatic_report_settings.cnv_signature = TESTDATA("data_in/somatic/Somatic_DNA123456_01-NA12878_03/cnv_signatures/De_Novo_map_to_COSMIC_CNV48.csv");

		S_EQUAL(db.processedSampleId("DNA123456_01"), "4004");

		somatic_report_settings.report_config.setIncludeTumContentByHistological(true);
		somatic_report_settings.report_config.setIncludeTumContentByClonality(false);
		somatic_report_settings.report_config.setIncludeTumContentByMaxSNV(false);
		somatic_report_settings.report_config.setIncludeTumContentByEstimated(true);
		somatic_report_settings.report_config.setTumContentByEstimated(42);
		somatic_report_settings.report_config.setMsiStatus(false);
		somatic_report_settings.report_config.setCnvBurden(false);
		somatic_report_settings.report_config.setIncludeMutationBurden(false);
		somatic_report_settings.report_config.setHrdStatement("no proof");
		somatic_report_settings.report_config.setCnvLohCount(0);
		somatic_report_settings.report_config.setCnvTaiCount(1);
		somatic_report_settings.report_config.setCnvLstCount(2);
		somatic_report_settings.report_config.setTmbReferenceText("Test reference text for the tmb of this analysis!");
		somatic_report_settings.report_config.setEvaluationDate(QDate(2022,12,1));
		somatic_report_settings.report_config.setLimitations("This text should appear as limitations!");

		//preferred transcripts
		somatic_report_settings.preferred_transcripts = db.getPreferredTranscripts();

		OntologyTermCollection obo_terms("://Resources/so-xp_3_1_0.obo",true);
		QList<QByteArray> ids;
		ids << obo_terms.childIDs("SO:0001580",true); //coding variants
		ids << obo_terms.childIDs("SO:0001568",true); //splicing variants
		foreach(const QByteArray& id, ids)
		{
			somatic_report_settings.obo_terms_coding_splicing.add(obo_terms.getByID(id));
		}


		TargetRegionInfo target_region = TargetRegionInfo();
		target_region.name = "VirtualTumorPanel_v5_exon20_ahott1a1_20230505";
		target_region.genes = db.subpanelGenes(target_region.name);
		target_region.regions = db.subpanelRegions(target_region.name);
		somatic_report_settings.target_region_filter = target_region;
		QStringList quality;
		somatic_report_settings.report_config.setQuality(quality);

		SomaticReportHelper report(GenomeBuild::HG38, vl, cnv_list, svs, control_tissue_variants, somatic_report_settings, true);
		report.storeRtf("out/somatic_report_tumor_normal_2.rtf");
		COMPARE_FILES("out/somatic_report_tumor_normal_2.rtf", TESTDATA("data_out/somatic_report_tumor_normal_2.rtf"));

		SomaticXmlReportGenerator xml_report;
		QSharedPointer<QFile> out_xml = Helper::openFileForWriting("out/somatic_report_tumor_normal_2.xml");
		xml_report.generateXML(report.getXmlData(), out_xml, db, true);

		COMPARE_FILES("out/somatic_report_tumor_normal_2.xml", TESTDATA("data_out/somatic_report_tumor_normal_2.xml"));
	}

	void test_rna_report_functions()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		QCoreApplication::setApplicationVersion("0.1-cppNGSD-TEST-Version"); //application version (is written into somatic xml report)
		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in1.sql"));
		//log in user
		LoginManager::login("ahmustm1", "", true);

		//Test methods for RNA fusions in NGSD
		S_EQUAL(db.rnaFusion(1).toString(), "NAPG chr18:10530837::SRPK2(1688),PUS7(38665) chr7:105400996");
		//Test get Fusion ID
		Fusion fu1(GenomePosition("chr18:10530837"), GenomePosition("chr7:105400996"), "NAPG", "ENST00000322897", "SRPK2(1688),PUS7(38665)", ".", "transclocation", "out-of-frame");
		Fusion fu2(GenomePosition("chr20:53256444"), GenomePosition("chr20:53487144"), "TSHZ2-1", "ENSG00000182463", "TSHZ2-2", "ENSG00000182463", "deletion/read-through", "out-of-frame");
		S_EQUAL(db.rnaFusionId(fu1, 1, false), "1");
		S_EQUAL(db.rnaFusionId(fu1, 2, false), ""); //Fusion on callset 2 does not exist
		S_EQUAL(db.rnaFusionId(fu2, 1, false), ""); //Fusion does not exist

		IS_THROWN(DatabaseException, db.somaticCnvId(CopyNumberVariant(Chromosome("chr7"), 87000, 350000), 1));

		int fu_id =  db.addRnaFusion(1, fu2).toInt();
		Fusion res_fu = db.rnaFusion(fu_id);
		S_EQUAL(res_fu.breakpoint1().chr().strNormalized(true), "chr20");
		S_EQUAL(res_fu.breakpoint1().pos(), 53256444);
		S_EQUAL(res_fu.breakpoint2().chr().strNormalized(true), "chr20");
		S_EQUAL(res_fu.breakpoint2().pos(), 53487144);
		S_EQUAL(res_fu.symbol1(), "TSHZ2-1");
		S_EQUAL(res_fu.symbol2(), "TSHZ2-2");
		S_EQUAL(res_fu.transcript1(), "ENSG00000182463");
		S_EQUAL(res_fu.transcript2(), "ENSG00000182463");
		S_EQUAL(res_fu.type(), "deletion/read-through");
		S_EQUAL(res_fu.reading_frame(), "out-of-frame");
	}

	void test_rna_report_rtf()
	{
		S_EQUAL("TODO", "finished");
	}

	//Test tumor only RTF report generation
	void report_tumor_only()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in2.sql"));

		VariantList vl;
		vl.load(TESTDATA("data_in/tumor_only.GSvar"));

		//Specifiy filter for report generation
		FilterCascade filters;
		filters.add(QSharedPointer<FilterBase>(new FilterFilterColumnEmpty()));

		QSharedPointer<FilterBase> keep_filter(new FilterClassificationNGSD());
		keep_filter->setString("action", "KEEP");
		keep_filter->setStringList("classes", {"4","5"});
		filters.add( ( keep_filter ) );

		//Fill report config
		TumorOnlyReportWorkerConfig config;
		config.filter_result = filters.apply(vl);
		config.low_coverage_file = TESTDATA("data_in/tumor_only_stat_lowcov.bed");
		config.preferred_transcripts.insert("MITF", QByteArrayList() << "ENST00000314589");

		ProcessingSystemData sys;
		sys.name = "tumor only test panel";
		sys.type = "Panel";
		config.sys = sys;

		ProcessedSampleData ps_data;
		ps_data.name = "DX000001_01";
		ps_data.comments = "MHH_STUFF_IN_COMMENT";
		config.ps_data = ps_data;

		config.roi.name = "tum_only_target_filter";
		config.roi.genes = GeneSet::createFromStringList(QStringList() << "MITF" << "SYNPR");

		BedFile tum_only_roi_filter;
		tum_only_roi_filter.load(TESTDATA("data_in/tumor_only_target_region.bed"));
		config.roi.regions = tum_only_roi_filter;
		config.bam_file = TESTDATA("data_in/tumor_only.bam");
		config.include_coverage_per_gap = true;
		config.include_exon_number_per_gap = true;
		config.use_test_db = true;
		config.build = GenomeBuild::HG19;

		//create RTF report with 2 SNVs and two gaps
		TumorOnlyReportWorker report_worker(vl, config);
		report_worker.checkAnnotation(vl);
		report_worker.writeRtf("out/tumor_only_report.rtf");

        REMOVE_LINES("out/tumor_only_report.rtf", QRegularExpression(QDate::currentDate().toString("dd.MM.yyyy").toUtf8())); //today's date
        REMOVE_LINES("out/tumor_only_report.rtf", QRegularExpression(QCoreApplication::applicationName().toUtf8())); //application name and version
		COMPARE_FILES("out/tumor_only_report.rtf", TESTDATA("data_out/tumor_only_report.rtf"));


		//create XML report
		report_worker.writeXML("out/tumor_only_report.xml", true);

		COMPARE_FILES("out/tumor_only_report.xml",  TESTDATA("data_out/tumor_only_report.xml") );
	}

	//Test for RNA expression data
	void rna_expression()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in3.sql"));

		//Test ENSG->gene_id mapping
		QMap<QByteArray, QByteArray> ensg_gene_mapping = db.getEnsemblGeneMapping();
		QMap<QByteArray, QByteArray> gene_ensg_mapping = db.getGeneEnsemblMapping();

		S_EQUAL(ensg_gene_mapping.value("ENSG00000204518"), "AADACL4");
		S_EQUAL(ensg_gene_mapping.value("ENSG00000171735"), "CAMTA1");
		S_EQUAL(ensg_gene_mapping.value("ENSG00000127463"), "EMC1");
		S_EQUAL(ensg_gene_mapping.value("ENSG00000231510"), "LINC02782");
		S_EQUAL(ensg_gene_mapping.value("ENSG00000263793"), "MIR3115");
		S_EQUAL(ensg_gene_mapping.value("ENSG00000187583"), "PLEKHN1");

		S_EQUAL(gene_ensg_mapping.value("AADACL4"), "ENSG00000204518");
		S_EQUAL(gene_ensg_mapping.value("CAMTA1"), "ENSG00000171735");
		S_EQUAL(gene_ensg_mapping.value("EMC1"), "ENSG00000127463");
		S_EQUAL(gene_ensg_mapping.value("LINC02782"), "ENSG00000231510");
		S_EQUAL(gene_ensg_mapping.value("MIR3115"), "ENSG00000263793");
		S_EQUAL(gene_ensg_mapping.value("PLEKHN1"), "ENSG00000187583");

		//Test expression data import
		db.importGeneExpressionData(TESTDATA("data_in/NGSD_expr_in1.tsv"), "RX001_01", false, false);
		int count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 102);
		db.importGeneExpressionData(TESTDATA("data_in/NGSD_expr_in2.tsv"), "RX002_01", false, false);
		count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 204);
		db.importGeneExpressionData(TESTDATA("data_in/NGSD_expr_in3.tsv"), "RX003_01", false, false);
		count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 306);
		db.importGeneExpressionData(TESTDATA("data_in/NGSD_expr_in4.tsv"), "RX004_01", false, false);
		count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 408);
		db.importGeneExpressionData(TESTDATA("data_in/NGSD_expr_in5.tsv"), "RX005_01", false, false);
		count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 510);
		db.importGeneExpressionData(TESTDATA("data_in/NGSD_expr_in6.tsv"), "RX006_01", false, false);
		count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 612);
		db.importGeneExpressionData(TESTDATA("data_in/NGSD_expr_in7.tsv"), "RX007_01", false, false);
		count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 714);
		db.importGeneExpressionData(TESTDATA("data_in/NGSD_expr_in8.tsv"), "RX008_01", false, false);
		count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 816);
		db.importGeneExpressionData(TESTDATA("data_in/NGSD_expr_in8.tsv"), "RX008_01", true, false);
		count = db.getValue("SELECT count(*) FROM expression").toInt();
		I_EQUAL(count, 816);

		//check imported values
		QMap<QByteArray,int> gene2id = db.getGeneExpressionGene2IdMapping();
		I_EQUAL(db.getValue("SELECT raw FROM expression WHERE processed_sample_id=5001 AND symbol_id=" + QString::number(gene2id.value(ensg_gene_mapping.value("ENSG00000049249")))).toInt(), 20934);
		F_EQUAL2(db.getValue("SELECT tpm FROM expression WHERE processed_sample_id=5001 AND symbol_id=" + QString::number(gene2id.value(ensg_gene_mapping.value("ENSG00000215720")))).toFloat(), 116.816, 0.001);
		I_EQUAL(db.getValue("SELECT raw FROM expression WHERE processed_sample_id=5002 AND symbol_id=" + QString::number(gene2id.value(ensg_gene_mapping.value("ENSG00000229716")))).toInt(), 1371);
		F_EQUAL2(db.getValue("SELECT tpm FROM expression WHERE processed_sample_id=5002 AND symbol_id=" + QString::number(gene2id.value(ensg_gene_mapping.value("ENSG00000159189")))).toFloat(), 204.76, 0.001);
		I_EQUAL(db.getValue("SELECT raw FROM expression WHERE processed_sample_id=5005 AND symbol_id=" + QString::number(gene2id.value(ensg_gene_mapping.value("ENSG00000227634")))).toInt(), 15679);
		F_EQUAL2(db.getValue("SELECT tpm FROM expression WHERE processed_sample_id=5005 AND symbol_id=" + QString::number(gene2id.value(ensg_gene_mapping.value("ENSG00000282740")))).toFloat(), 0.0, 0.001);


		//Test exon expression data import
		db.importExonExpressionData(TESTDATA("data_in/NGSD_expr_exon_in1.tsv"), "RX001_01", false, false);
		count = db.getValue("SELECT count(*) FROM expression_exon").toInt();
		I_EQUAL(count, 71);
		db.importExonExpressionData(TESTDATA("data_in/NGSD_expr_exon_in2.tsv"), "RX002_01", false, false);
		count = db.getValue("SELECT count(*) FROM expression_exon").toInt();
		I_EQUAL(count, 142);
		db.importExonExpressionData(TESTDATA("data_in/NGSD_expr_exon_in3.tsv"), "RX003_01", false, false);
		count = db.getValue("SELECT count(*) FROM expression_exon").toInt();
		I_EQUAL(count, 213);
		db.importExonExpressionData(TESTDATA("data_in/NGSD_expr_exon_in4.tsv"), "RX004_01", false, false);
		count = db.getValue("SELECT count(*) FROM expression_exon").toInt();
		I_EQUAL(count, 284);
		IS_THROWN(DatabaseException, db.importExonExpressionData(TESTDATA("data_in/NGSD_expr_exon_in1.tsv"), "RX001_01", false, false));
		db.importExonExpressionData(TESTDATA("data_in/NGSD_expr_exon_in1.tsv"), "RX001_01", true, false);
		count = db.getValue("SELECT count(*) FROM expression_exon").toInt();
		I_EQUAL(count, 284);


		//Test cohort determination:
		QSet<int> cohort = db.getRNACohort(1, "blood");
		I_EQUAL(cohort.size(), 4);
		IS_TRUE(cohort.contains(5005));
		IS_TRUE(cohort.contains(5006));
		IS_TRUE(cohort.contains(5007));
		IS_TRUE(cohort.contains(5008));

		cohort = db.getRNACohort(1, "blood", "", "", RNA_COHORT_GERMLINE, "genes", QStringList() << "bad" << "medium" << "n/a");
		I_EQUAL(cohort.size(), 2);
		IS_TRUE(cohort.contains(5006));
		IS_TRUE(cohort.contains(5008));

		cohort = db.getRNACohort(1, "skin", "KontrollDNACoriell", "5001", RNA_COHORT_GERMLINE_PROJECT);
		I_EQUAL(cohort.size(), 2);
		IS_TRUE(cohort.contains(5001));
		IS_TRUE(cohort.contains(5003));

		cohort = db.getRNACohort(1, "", "KontrollDNACoriell", "5001", RNA_COHORT_SOMATIC);
		I_EQUAL(cohort.size(), 4);
		IS_TRUE(cohort.contains(5001));
		IS_TRUE(cohort.contains(5003));
		IS_TRUE(cohort.contains(5005));
		IS_TRUE(cohort.contains(5007));


		//Test expression stats:
		QMap<QByteArray, ExpressionStats> expression_stats = db.calculateCohortExpressionStatistics(1, "blood", cohort);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000232596")).mean, 121.091, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000232596")).mean_log2, 5.373, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000232596")).stddev_log2, 3.167, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000049245")).mean, 0, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000049245")).mean_log2, 0, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000049245")).stddev_log2, 0, 0.001);
		I_EQUAL(cohort.size(), 4);

		expression_stats = db.calculateCohortExpressionStatistics(1, "blood", cohort, "KontrollDNACoriell", "5001", RNA_COHORT_GERMLINE_PROJECT);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000232596")).mean, 204.681, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000232596")).mean_log2, 7.6221, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000232596")).stddev_log2, 0.427, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000049245")).mean, 0.0, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000049245")).mean_log2, 0.0, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000049245")).stddev_log2, 0.0, 0.001);
		I_EQUAL(cohort.size(), 2);

		expression_stats = db.calculateCohortExpressionStatistics(1, "skin", cohort);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000157916")).mean, 47.9532, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000283234")).mean, 0.0, 0.001);
		I_EQUAL(cohort.size(), 4);

		expression_stats = db.calculateCohortExpressionStatistics(1, "skin", cohort, "KontrollDNACoriell", "5001", RNA_COHORT_GERMLINE);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000157916")).mean, 47.953, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000157916")).mean_log2, 1.898, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000157916")).stddev_log2, 3.287, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000283234")).mean, 0, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000283234")).mean_log2, 0, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000283234")).stddev_log2, 0, 0.001);
		I_EQUAL(cohort.size(), 4);

		expression_stats = db.calculateCohortExpressionStatistics(1, "skin", cohort, "KontrollDNACoriell", "5001", RNA_COHORT_GERMLINE_PROJECT);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000157916")).mean, 95.907, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000157916")).mean_log2, 3.796, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000157916")).stddev_log2, 3.796, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000283234")).mean, 0, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000283234")).mean_log2, 0, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000283234")).stddev_log2, 0, 0.001);
		I_EQUAL(cohort.size(), 2);

		//test for somatic cohort
		expression_stats = db.calculateCohortExpressionStatistics(1, "", cohort, "KontrollDNACoriell", "5001", RNA_COHORT_SOMATIC);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000232596")).mean, 177.952, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000232596")).mean_log2, 7.436, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000232596")).stddev_log2, 0.355, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000049245")).mean, 38.422, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000049245")).mean_log2, 1.818, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000049245")).stddev_log2, 3.149, 0.001);
		I_EQUAL(cohort.size(), 4);

		expression_stats = db.calculateCohortExpressionStatistics(1, "", cohort, "KontrollDNACoriell2", "5002", RNA_COHORT_SOMATIC);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000157916")).mean, 27.191, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000157916")).mean_log2, 1.695, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000157916")).stddev_log2, 2.935, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000283234")).mean, 0, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000283234")).mean_log2, 0, 0.001);
		F_EQUAL2(expression_stats.value(ensg_gene_mapping.value("ENSG00000283234")).stddev_log2, 0, 0.001);
		I_EQUAL(cohort.size(), 4);


		//Test limited gene expresion stats:
		cohort = db.getRNACohort(1, "blood");
		expression_stats = db.calculateGeneExpressionStatistics(cohort, "LINC01646");
		I_EQUAL(expression_stats.size(), 1);
		F_EQUAL2(expression_stats.value("LINC01646").mean, 121.091, 0.001);
		F_EQUAL2(expression_stats.value("LINC01646").mean_log2, 5.373, 0.001);
		F_EQUAL2(expression_stats.value("LINC01646").stddev_log2, 3.167, 0.001);
		expression_stats = db.calculateGeneExpressionStatistics(cohort, "VAMP3");
		I_EQUAL(expression_stats.size(), 1);
		F_EQUAL2(expression_stats.value("VAMP3").mean, 0, 0.001);
		F_EQUAL2(expression_stats.value("VAMP3").mean_log2, 0, 0.001);
		F_EQUAL2(expression_stats.value("VAMP3").stddev_log2, 0, 0.001);

		cohort = db.getRNACohort(1, "blood", "KontrollDNACoriell", "5001", RNA_COHORT_GERMLINE_PROJECT);
		expression_stats = db.calculateGeneExpressionStatistics(cohort, "LINC01646");
		I_EQUAL(expression_stats.size(), 1);
		F_EQUAL2(expression_stats.value("LINC01646").mean, 204.681, 0.001);
		F_EQUAL2(expression_stats.value("LINC01646").mean_log2, 7.6221, 0.001);
		F_EQUAL2(expression_stats.value("LINC01646").stddev_log2, 0.427, 0.001);
		expression_stats = db.calculateGeneExpressionStatistics(cohort, "VAMP3");
		I_EQUAL(expression_stats.size(), 1);
		F_EQUAL2(expression_stats.value("VAMP3").mean, 0.0, 0.001);
		F_EQUAL2(expression_stats.value("VAMP3").mean_log2, 0.0, 0.001);
		F_EQUAL2(expression_stats.value("VAMP3").stddev_log2, 0.0, 0.001);

		cohort = db.getRNACohort(1, "", "KontrollDNACoriell2", "5002", RNA_COHORT_SOMATIC);
		expression_stats = db.calculateGeneExpressionStatistics(cohort, "RER1");
		I_EQUAL(expression_stats.size(), 1);
		F_EQUAL2(expression_stats.value("RER1").mean, 27.191, 0.001);
		F_EQUAL2(expression_stats.value("RER1").mean_log2, 1.695, 0.001);
		F_EQUAL2(expression_stats.value("RER1").stddev_log2, 2.935, 0.001);

		//Test sample expression values
		QMap<QByteArray, double> sample_expression_data = db.getGeneExpressionValuesOfSample("5001", false);
		I_EQUAL(sample_expression_data.size(), 102);
		F_EQUAL2(sample_expression_data.value("BRWD1P1"), 51.7352, 0.001);
		F_EQUAL2(sample_expression_data.value("CASP9"), 28.8433, 0.001);
		F_EQUAL2(sample_expression_data.value("TMEM201"), 58.3165, 0.001);
		F_EQUAL2(sample_expression_data.value("USP48"), 0.0000, 0.001);

		sample_expression_data = db.getGeneExpressionValuesOfSample("123456", true);
		I_EQUAL(sample_expression_data.size(), 0);


		//Test exon expression stats:
		cohort = db.getRNACohort(1, "skin", "", "", RNA_COHORT_GERMLINE, "exons");
		expression_stats = db.calculateExonExpressionStatistics(cohort);
		F_EQUAL2(expression_stats.value("chr1:966704-966803").mean, 85.09675, 0.001);
		F_EQUAL2(expression_stats.value("chr1:966704-966803").mean_log2, 2.103816078, 0.001);
		F_EQUAL2(expression_stats.value("chr1:966704-966803").stddev_log2, 3.643916337, 0.001);

		F_EQUAL2(expression_stats.value("chr1:971077-971208").mean, 62.94, 0.001);
		F_EQUAL2(expression_stats.value("chr1:971077-971208").mean_log2, 3.403752578, 0.001);
		F_EQUAL2(expression_stats.value("chr1:971077-971208").stddev_log2, 3.443240651, 0.001);

		F_EQUAL2(expression_stats.value("chr1:868240-868530").mean, 134.428, 0.001);
		F_EQUAL2(expression_stats.value("chr1:868240-868530").mean_log2, 4.031839133, 0.001);
		F_EQUAL2(expression_stats.value("chr1:868240-868530").stddev_log2, 4.03405759, 0.001);

		F_EQUAL2(expression_stats.value("chr1:1180731-1180860").mean, 63.429, 0.001);
		F_EQUAL2(expression_stats.value("chr1:1180731-1180860").mean_log2, 1.998186444, 0.001);
		F_EQUAL2(expression_stats.value("chr1:1180731-1180860").stddev_log2, 3.460960444, 0.001);

		F_EQUAL2(expression_stats.value("chr1:30267-30667").mean, 10.3584, 0.001);
		F_EQUAL2(expression_stats.value("chr1:30267-30667").mean_log2, 1.351783794, 0.001);
		F_EQUAL2(expression_stats.value("chr1:30267-30667").stddev_log2, 2.341358211, 0.001);

		//Test single exon:
		expression_stats = db.calculateExonExpressionStatistics(cohort, BedLine(Chromosome("chr1"), 30267, 30667));
		F_EQUAL2(expression_stats.value("chr1:30267-30667").mean, 10.3584, 0.001);
		F_EQUAL2(expression_stats.value("chr1:30267-30667").mean_log2, 1.351783794, 0.001);
		F_EQUAL2(expression_stats.value("chr1:30267-30667").stddev_log2, 2.341358211, 0.001);

	}

	void report_rna()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		QCoreApplication::setApplicationVersion("0.1-cppNGSD-TEST-Version"); //application version (is written into somatic xml report)
		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in1.sql"));
		//log in user
		LoginManager::login("ahmustm1", "", true);

		//import of RNA fusions

		S_EQUAL(db.rnaFusion(1).toString(), "DIAPH2 chrX:96763120::DIAPH2 chrX:96881579");
		S_EQUAL(db.rnaFusion(2).toString(), "FRS2 chr12:69500323::HNRNPU chr1:244856550");
		S_EQUAL(db.rnaFusion(3).toString(), "PIK3C3 chr18:41987898::PRORP chr14:35180670");

		Fusion fusion1(GenomePosition("chrX", 96763120), GenomePosition("chrX", 96881579), "DIAPH2", "ENST00000324765", "DIAPH2", "ENST00000324765", "deletion/read-through", "in-frame");
		S_EQUAL(db.rnaFusionId(fusion1, 1), "1");
		Fusion fusion2(GenomePosition("chrX", 96763120), GenomePosition("chrX", 96881579), "DIAPH2", "ENST00000324765", "DIAPH2", "ENST00000324765", "", ""); // type and reading frame are not necessary to find fusion
		S_EQUAL(db.rnaFusionId(fusion2, 1), "1");
		Fusion fusion3(GenomePosition("chr1", 12345), GenomePosition("chr1", 23456), "FRS2", ".", "HNRNPU", ".", "", "");
		S_EQUAL(db.rnaFusionId(fusion3, 1, false), "");
		IS_THROWN(DatabaseException, db.rnaFusionId(fusion3, 0))

		ArribaFile fusions;
		fusions.load(TESTDATA("data_in/rna_fusions_arriba.tsv"));
		QString fusion_id = "";
		fusion_id = db.addRnaFusion(2, fusions.getFusion(0));
		S_EQUAL(fusion_id, "5");
		S_EQUAL(db.rnaFusion(5).toString(), "NAPG chr18:10530837::SRPK2(1688),PUS7(38665) chr7:105400996");
		fusion_id = db.addRnaFusion(2, fusions.getFusion(1));
		S_EQUAL(fusion_id, "6");
		S_EQUAL(db.rnaFusion(6).toString(), "TSHZ2 chr20:53256444::TSHZ2 chr20:53487144");
		fusion_id = db.addRnaFusion(2, fusions.getFusion(2));
		S_EQUAL(fusion_id, "7");
		S_EQUAL(db.rnaFusion(7).toString(), "LRBA chr4:150599059::LRBA chr4:150342832");
		//already imported fusion
		IS_THROWN(DatabaseException, db.addRnaFusion(2, fusions.getFusion(2)));


		//report config of RNA fusions:
		RnaReportConfiguration rna_config;
		rna_config.setCreatedAt(QDateTime(QDate(2000,1,1), QTime(11,11)));
		rna_config.setCreatedBy("ahmustm1");

		RnaReportFusionConfiguration fusion_config1;
		fusion_config1.exclude_artefact = true;
		fusion_config1.variant_index = 2;
		fusion_config1.comment = "is an artifact";
		rna_config.addRnaFusionConfiguration(fusion_config1);

		RnaReportFusionConfiguration fusion_config2;
		fusion_config2.variant_index = 0;
		fusion_config2.comment = "is real and should be in the report";

		rna_config.addRnaFusionConfiguration(fusion_config2); // add to report

		db.setRnaReportConfig("10", rna_config, fusions, "ahmustm1");

		QStringList messages;
		RnaReportConfiguration loaded =  db.rnaReportConfig("10", fusions, messages);

		I_EQUAL(loaded.count(), 2);
		RnaReportFusionConfiguration conf1 = loaded.get(0);
		IS_FALSE(conf1.exclude_artefact);
		S_EQUAL(conf1.comment, "is real and should be in the report");
		RnaReportFusionConfiguration conf2 = loaded.get(2);
		IS_TRUE(conf2.exclude_artefact);
		S_EQUAL(conf2.comment, "is an artifact");
		IS_THROWN(ArgumentException, loaded.get(1));

	}

	//This test should be in VariantHgvsAnnotator_Test.h, but it requires the production NGSD. Thus it is here.
	//Test data exported from NGSD via GSvar (debug section of ahsturm1) on Nov 1th 2022.
	//Annotation done with VEP 107 (/mnt/users/ahsturm1/Sandbox/2022_11_04_compare_annotations_with_VEP/).
	//Some annotations were manually corrected because VEP was wrong - this is documented in the CORRECTED info entry of the variant.
	void VariantHgvsAnnotator_comparison_vep()
	{
		if (!NGSD::isAvailable()) SKIP("Test needs access to the NGSD production database!");
		NGSD db;
		int gene_count = db.getValue("SELECT count(*) FROM gene").toInt();
		if (gene_count<15000) SKIP("Too little genes in production database!");
		int tans_count = db.getValue("SELECT count(*) FROM gene_transcript").toInt();
		if (tans_count<40000) SKIP("Too transcripts genes in production database!");

		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator annotator(reference);
		QTextStream out(stdout);

		int c_pass = 0;
		int c_fail = 0;

		//load best transcripts
		QMap<QByteArray, QByteArray> best;
		TsvFile tmp;
		tmp.load(TESTDATA("data_in/VariantHgvsAnnotator_comparison_vep_best_transcripts.tsv"));
		for (int i=0; i<tmp.count(); ++i)
		{
			const QStringList& row = tmp[i];
			best[row[0].toUtf8()] = row[1].toUtf8();
		}

		//process VCF
		VcfFile vcf;
		vcf.load(TESTDATA("data_in/VariantHgvsAnnotator_comparison_vep.vcf.gz"));
		for(int i=0; i<vcf.count(); ++i)
		{
			const VcfLine& v = vcf[i];

			//process overlapping genes
			GeneSet genes = db.genesOverlapping(v.chr(), v.start(), v.end());
            for (const QByteArray& gene : genes)
			{
				//process best transcript for gene
				if (!best.contains(gene)) continue;
				Transcript trans = db.transcript(db.transcriptId(best[gene]));
				best[gene] = trans.name();
				if (trans.isValid())
				{

					//check VEP for transcript exists
					QByteArrayList vep_annos;
					foreach(QByteArray entry, v.info("CSQ").split(','))
					{
						if (entry.contains("|" + trans.name() + "."))
						{
							vep_annos = entry.split('|');
						}
					}
					if (vep_annos.isEmpty()) continue;

					//compare VEP and own annotation
					QByteArrayList differences;
					VariantConsequence cons = annotator.annotate(trans, v);
					if (cons.hgvs_p=="p.?") cons.hgvs_p="";

					QByteArray vep_hgvsc = (vep_annos[2]+':').split(':')[1];
					if (vep_hgvsc!=cons.hgvs_c) differences << vep_hgvsc + " > " + cons.hgvs_c;

					QByteArray vep_hgvsp = (vep_annos[3]+':').split(':')[1];
					vep_hgvsp.replace("%3D", "=");
					if (vep_hgvsp!=cons.hgvs_p) differences << vep_hgvsp + " > " + cons.hgvs_p;

					QByteArrayList vep_types = vep_annos[4].split('&');
					if (vep_types.contains("splice_polypyrimidine_tract_variant")) vep_types << "splice_region_variant"; //we don't annotate this type
					if (vep_types.contains("splice_donor_region_variant")) vep_types << "splice_region_variant";  //we don't annotate this type
					if (vep_types.contains("splice_donor_5th_base_variant")) vep_types << "splice_region_variant";  //we don't annotate this type
					if (vep_types.contains("mature_miRNA_variant")) vep_types << "non_coding_transcript_exon_variant";  //we don't annotate this type
					if (vep_types.contains("frameshift_variant") && vep_hgvsp.contains("Ter") && !vep_hgvsp.contains("fs")) vep_types << "stop_gained"; //VEP handles direct stop-gain variants as frameshift, which is not correct.
					VariantConsequenceType max_csq_type = VariantConsequenceType::INTERGENIC_VARIANT;
					foreach(VariantConsequenceType csq_type, cons.types)
					{
						if(csq_type > max_csq_type)
						{
							max_csq_type = csq_type;
						}
					}
					if (!vep_types.contains(VariantConsequence::typeToString(max_csq_type)))
					{
						differences << VariantConsequence::typeToString(max_csq_type) + " not in VEP (" + vep_types.join(", ") + ")";
					}

					QByteArray vep_impact = vep_annos[5];
					if (vep_impact != variantImpactToString(cons.impact)) differences << vep_impact + " > " + variantImpactToString(cons.impact);

					QByteArray vep_exon = vep_annos[6].split('/')[0];
					if (vep_exon.contains('-')) vep_exon = vep_exon.split('-')[0]; //we annotate only the first affected exon
					int vep_exon_nr = vep_exon.isEmpty() ? -1 : vep_exon.toInt();
					if (vep_exon_nr!=cons.exon_number) differences << "exon " + QByteArray::number(vep_exon_nr) + " > " + QByteArray::number(cons.exon_number);

					QByteArray vep_intron = vep_annos[7].split('/')[0];
					if (vep_intron.contains('-')) vep_intron = vep_intron.split('-')[0]; //we annotate only the first affected intron
					int vep_intron_nr = vep_intron.isEmpty() ? -1 : vep_intron.toInt();
					if (vep_intron_nr!=cons.intron_number) differences << "intron " + QByteArray::number(vep_intron_nr) + " > " + QByteArray::number(cons.intron_number);

					if (differences.isEmpty())
					{
						++c_pass;
					}
					else
					{
						++c_fail;
                        out << v.toString(true) << " (" << cons.normalized << ") transcript=" << trans.name() << " " << cons.toString() << QT_ENDL;
						foreach(QByteArray difference, differences)
						{
                            out << "  " << difference << QT_ENDL;
						}
					}
				}
			}
		}

		I_EQUAL(c_fail, 0);
	}

	void test_overriding_the_processed_sample_data_folder()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in4.sql"));

		QString path_with_override = db.processedSamplePath(db.processedSampleId("NA12878_02"), PathType::GSVAR);
		IS_TRUE(path_with_override.endsWith("new/folder/NA12878_02.GSvar"));

		QString path_without_override = db.processedSamplePath(db.processedSampleId("NA12878_03"), PathType::GSVAR);
		IS_TRUE(path_without_override.endsWith("somatic/Sample_NA12878_03/NA12878_03.GSvar"));
	}

	void test_create_sample_sheet_for_novaseqx()
	{
		if (!NGSD::isAvailable(true)) SKIP("Test needs access to the NGSD test database!");

		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in5.sql"));

		QStringList warnings;
		QString sample_sheet = db.createSampleSheet(1, warnings, NsxAnalysisSettings());
		S_EQUAL(warnings.at(0), "WARNING: The number of lanes covered by samples (5) and the number of lanes on the flow cell (8) does not match!");

		//write to file
		QSharedPointer<QFile> output_file = Helper::openFileForWriting("out/NovaSeqX_samplesheet.csv");
		output_file->write(sample_sheet.toLatin1());
		output_file->flush();
		output_file->close();

		COMPARE_FILES("out/NovaSeqX_samplesheet.csv",  TESTDATA("data_out/NovaSeqX_samplesheet.csv") );

		//second run without adapter sequence
		warnings.clear();
		sample_sheet = db.createSampleSheet(2, warnings, NsxAnalysisSettings());
		S_EQUAL(warnings.at(0), "WARNING: The number of lanes covered by samples (3) and the number of lanes on the flow cell (2) does not match!");
		S_EQUAL(warnings.at(1), "WARNING: No adapter for read 1 provided! Adapter trimming will not work.");
		S_EQUAL(warnings.at(2), "WARNING: No adapter for read 2 provided! Adapter trimming will not work.");

		//write to file
		output_file = Helper::openFileForWriting("out/NovaSeqX_samplesheet2.csv");
		output_file->write(sample_sheet.toLatin1());
		output_file->flush();
		output_file->close();

		COMPARE_FILES("out/NovaSeqX_samplesheet2.csv",  TESTDATA("data_out/NovaSeqX_samplesheet2.csv") );

	}

};
