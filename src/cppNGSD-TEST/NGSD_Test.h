#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"

TEST_CLASS(NGSD_Test)
{
Q_OBJECT
private slots:

	//Normally, one member is tested in one QT slot.
	//Because initializing the database takes very long, all NGSD functionality is tested in one slot.
	void main_tests()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in1.sql"));

		//getEnum
		QStringList enum_values = db.getEnum("sample", "disease_group");
		I_EQUAL(enum_values.count(), 18);
		S_EQUAL(enum_values[4], "Endocrine, nutritional or metabolic diseases");

		//getProcessingSystems
		QMap<QString, QString> systems = db.getProcessingSystems(false, false);
		I_EQUAL(systems.size(), 2);
		IS_TRUE(systems.contains("HaloPlex HBOC v5"))
		IS_TRUE(systems.contains("HaloPlex HBOC v6"))

		//getProcessingSystemData
		ProcessingSystemData system_data = db.getProcessingSystemData(db.processedSampleId("NA12878_03"), false);
		S_EQUAL(system_data.name, "HaloPlex HBOC v5");
		S_EQUAL(system_data.name_short, "hpHBOCv5");
		S_EQUAL(system_data.adapter1_p5, "AGATCGGAAGAGCACACGTCTGAACTCCAGTCAC");
		S_EQUAL(system_data.adapter2_p7, "AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT");
		S_EQUAL(system_data.type, "Panel Haloplex");
		IS_FALSE(system_data.shotgun);
		S_EQUAL(system_data.genome, "hg19");

		//normalSample
		S_EQUAL(db.normalSample(db.processedSampleId("NA12345_01")), "NA12878_03")

		//nextProcessingId
		S_EQUAL(db.nextProcessingId(db.sampleId("NA12878")), "5");

		//processedSamplePath
		QString gsvar_path = db.processedSamplePath(db.processedSampleId("NA12878_03"), NGSD::GSVAR);
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

		//geneToApprovedID
		int gene_app_id = db.geneToApprovedID("BRCA1");
		I_EQUAL(gene_app_id, 1);
		gene_app_id = db.geneToApprovedID("BLABLA");
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
		S_EQUAL(sample_data.quality, "good");
		S_EQUAL(sample_data.comments, "comment_s1");
		S_EQUAL(sample_data.disease_group, "n/a");
		S_EQUAL(sample_data.disease_status, "n/a");
		IS_FALSE(sample_data.is_tumor);
		IS_FALSE(sample_data.is_ffpe);
		//second sample (tumor)
		sample_id = db.sampleId("NA12345_01");
		sample_data = db.getSampleData(sample_id);
		S_EQUAL(sample_data.name, "NA12345");
		S_EQUAL(sample_data.name_external, "ex3");
		S_EQUAL(sample_data.quality, "bad");
		S_EQUAL(sample_data.comments, "comment_s3");
		S_EQUAL(sample_data.disease_group, "n/a");
		S_EQUAL(sample_data.disease_status, "n/a");
		IS_TRUE(sample_data.is_tumor);
		IS_TRUE(sample_data.is_ffpe);

		//getProcessedSampleData
		QString processed_sample_id = db.processedSampleId("NA12878_03");
		ProcessedSampleData processed_sample_data = db.getProcessedSampleData(processed_sample_id);
		S_EQUAL(processed_sample_data.name, "NA12878_03");
		S_EQUAL(processed_sample_data.quality, "medium");
		S_EQUAL(processed_sample_data.comments, "comment_ps1");
		S_EQUAL(processed_sample_data.project_name, "KontrollDNACoriell");
		S_EQUAL(processed_sample_data.run_name, "#00372");
		S_EQUAL(processed_sample_data.normal_sample_name, "");
		//second sample (tumor)
		processed_sample_id = db.processedSampleId("NA12345_01");
		processed_sample_data = db.getProcessedSampleData(processed_sample_id);
		S_EQUAL(processed_sample_data.name, "NA12345_01");
		S_EQUAL(processed_sample_data.quality, "good");
		S_EQUAL(processed_sample_data.comments, "comment_ps4");
		S_EQUAL(processed_sample_data.project_name, "KontrollDNACoriell");
		S_EQUAL(processed_sample_data.run_name, "#00372");
		S_EQUAL(processed_sample_data.normal_sample_name, "NA12878_03");

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
		S_EQUAL(regions[0].annotations()[0], "NIPA1 NIPA1_TR2");
		S_EQUAL(regions[1].annotations()[0], "NIPA1 NIPA1_TR1");
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
		S_EQUAL(regions[0].annotations()[0], "NIPA1 NIPA1_TR1");
		S_EQUAL(regions[1].annotations()[0], "NIPA1 NIPA1_TR2");
		S_EQUAL(regions[2].annotations()[0], "NIPA1 NIPA1_TR2");
		S_EQUAL(regions[3].annotations()[0], "NIPA1 NIPA1_TR1");
		regions.merge(); //overlapping regions
		I_EQUAL(regions.count(), 2);
		I_EQUAL(regions.baseCount(), 202);
		IS_TRUE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(GeneSet() << "NON-CODING", Transcript::ENSEMBL, "exon", false, true, &stream); //exon mode, non-coding, annotate_transcripts
		I_EQUAL(regions.count(), 2);
		S_EQUAL(regions[0].annotations()[0], "NON-CODING NON-CODING_TR1");
		S_EQUAL(regions[1].annotations()[0], "NON-CODING NON-CODING_TR1");
		I_EQUAL(regions.count(), 2);
		I_EQUAL(regions.baseCount(), 202);
		IS_TRUE(messages.isEmpty());


		//transcripts
		QList<Transcript> transcripts = db.transcripts(1, Transcript::CCDS, true); //BRCA1, CCDS, coding
		I_EQUAL(transcripts.count(), 1);
		S_EQUAL(transcripts[0].name(), "BRCA1_TR1");
		I_EQUAL(transcripts[0].strand(), Transcript::PLUS);
		I_EQUAL(transcripts[0].source(), Transcript::CCDS);
		I_EQUAL(transcripts[0].regions().count(), 4);
		I_EQUAL(transcripts[0].regions().baseCount(), 44);

		transcripts = db.transcripts(3, Transcript::ENSEMBL, true); //NIPA1, Ensembl, coding
		I_EQUAL(transcripts.count(), 2);
		S_EQUAL(transcripts[0].name(), "NIPA1_TR1");
		I_EQUAL(transcripts[0].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[0].source(), Transcript::ENSEMBL);
		I_EQUAL(transcripts[0].regions().count(), 2);
		I_EQUAL(transcripts[0].regions().baseCount(), 202);
		S_EQUAL(transcripts[1].name(), "NIPA1_TR2");
		I_EQUAL(transcripts[1].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[1].source(), Transcript::ENSEMBL);
		I_EQUAL(transcripts[1].regions().count(), 2);
		I_EQUAL(transcripts[1].regions().baseCount(), 102);

		transcripts = db.transcripts(3, Transcript::ENSEMBL, false); //NIPA1, Ensembl, non-coding
		I_EQUAL(transcripts.count(), 2);
		S_EQUAL(transcripts[0].name(), "NIPA1_TR1");
		I_EQUAL(transcripts[0].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[0].source(), Transcript::ENSEMBL);
		I_EQUAL(transcripts[0].regions().count(), 2);
		I_EQUAL(transcripts[0].regions().baseCount(), 202);
		S_EQUAL(transcripts[1].name(), "NIPA1_TR2");
		I_EQUAL(transcripts[1].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[1].source(), Transcript::ENSEMBL);
		I_EQUAL(transcripts[1].regions().count(), 4);
		I_EQUAL(transcripts[1].regions().baseCount(), 224);

		transcripts = db.transcripts(4, Transcript::ENSEMBL, true); //NON-CODING, Ensembl, coding
		I_EQUAL(transcripts.count(), 0);

		transcripts = db.transcripts(4, Transcript::ENSEMBL, false); //NON-CODING, Ensembl, non-coding
		I_EQUAL(transcripts.count(), 1);
		S_EQUAL(transcripts[0].name(), "NON-CODING_TR1");
		I_EQUAL(transcripts[0].regions().count(), 2);
		I_EQUAL(transcripts[0].regions().baseCount(), 202);

		//longestCodingTranscript
		Transcript transcript = db.longestCodingTranscript(4, Transcript::ENSEMBL); //NON-CODING, zero transcripts
		IS_FALSE(transcript.isValid());

		transcript = db.longestCodingTranscript(1, Transcript::CCDS); //BRCA1, one transcript
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "BRCA1_TR1");
		I_EQUAL(transcript.regions().count(), 4);
		I_EQUAL(transcript.regions().baseCount(), 44);

		transcript = db.longestCodingTranscript(3, Transcript::ENSEMBL); //NIPA1, two transcripts
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "NIPA1_TR1");
		I_EQUAL(transcript.regions().count(), 2);
		I_EQUAL(transcript.regions().baseCount(), 202);

		//geneInfo
		GeneInfo ginfo = db.geneInfo("BRCA1");
		S_EQUAL(ginfo.symbol, "BRCA1");
		S_EQUAL(ginfo.name, "Breast cancer associated gene 1");
		S_EQUAL(ginfo.exac_pli, "0.00");
		S_EQUAL(ginfo.inheritance, "AD");
		S_EQUAL(ginfo.comments, "");

		ginfo = db.geneInfo("NIPA1");
		S_EQUAL(ginfo.symbol, "NIPA1");
		S_EQUAL(ginfo.exac_pli, "n/a");
		S_EQUAL(ginfo.inheritance, "n/a");
		S_EQUAL(ginfo.comments, "");

		//setGeneInfo (existing gene)
		S_EQUAL(ginfo.symbol, "NIPA1");
		ginfo.exac_pli = "1.23";
		ginfo.inheritance = "AD";
		ginfo.comments = "comment";
		db.setGeneInfo(ginfo);
		ginfo = db.geneInfo("NIPA1");
		S_EQUAL(ginfo.symbol, "NIPA1");
		S_EQUAL(ginfo.exac_pli, "n/a");
		S_EQUAL(ginfo.inheritance, "AD");
		S_EQUAL(ginfo.comments, "comment");

		//setGeneInfo (new gene)
		ginfo.symbol = "NEWGENE";
		db.setGeneInfo(ginfo);
		ginfo = db.geneInfo("NEWGENE");
		S_EQUAL(ginfo.symbol, "NEWGENE");
		S_EQUAL(ginfo.exac_pli, "n/a");
		S_EQUAL(ginfo.inheritance, "AD");
		S_EQUAL(ginfo.comments, "comment");

		//precalculateGenotypeCounts
		messages.clear();
		db.precalculateGenotypeCounts(&stream, 50);
		//qDebug() << messages.split("\n");
		I_EQUAL(db.getValue("SELECT count_hom FROM detected_variant_counts WHERE variant_id=2336993").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count_het FROM detected_variant_counts WHERE variant_id=2336993").toInt(), 1);
		I_EQUAL(db.getValue("SELECT count_hom FROM detected_variant_counts WHERE variant_id=2346586").toInt(), 2);
		I_EQUAL(db.getValue("SELECT count_het FROM detected_variant_counts WHERE variant_id=2346586").toInt(), 1);
		I_EQUAL(db.getValue("SELECT count_hom FROM detected_variant_counts WHERE variant_id=2407544").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count_het FROM detected_variant_counts WHERE variant_id=2407544").toInt(), 2);

		//approvedGeneNames
		GeneSet approved = db.approvedGeneNames();
		I_EQUAL(approved.count(), 4);

		//phenotypeChildren
		QList<Phenotype> hpo_ids = db.phenotypeChildTems(Phenotype("HP:0000001", "All"), true);
		I_EQUAL(hpo_ids.count(), 10);
		hpo_ids = db.phenotypeChildTems(Phenotype("HP:0000001", "All"), false);
		I_EQUAL(hpo_ids.count(), 4);
		IS_TRUE(hpo_ids.contains(Phenotype("HP:0000005","Mode of inheritance")));
		IS_TRUE(hpo_ids.contains(Phenotype("HP:0000118","Phenotypic abnormality")));
		IS_TRUE(hpo_ids.contains(Phenotype("HP:0012823","Clinical modifier")));
		IS_TRUE(hpo_ids.contains(Phenotype("HP:0040279","Frequency")));
		//inner node
		hpo_ids = db.phenotypeChildTems(Phenotype("HP:0000005", "Mode of inheritance"), true);
		I_EQUAL(hpo_ids.count(), 6);
		IS_TRUE(hpo_ids.contains(Phenotype("HP:0001419","X-linked recessive inheritance")));
		hpo_ids = db.phenotypeChildTems(Phenotype("HP:0000005", "Mode of inheritance"), false);
		I_EQUAL(hpo_ids.count(), 4);
		IS_FALSE(hpo_ids.contains(Phenotype("HP:0001419","X-linked recessive inheritance")));
		//leaf
		hpo_ids = db.phenotypeChildTems(Phenotype("HP:0001427", "Mitochondrial inheritance"), true);
		I_EQUAL(hpo_ids.count(), 0);
		hpo_ids = db.phenotypeChildTems(Phenotype("HP:0001427", "Mitochondrial inheritance"), false);
		I_EQUAL(hpo_ids.count(), 0);

		//getDiagnosticStatus
		DiagnosticStatusData diag_status = db.getDiagnosticStatus(db.processedSampleId("NA12878_03"));
		S_EQUAL(diag_status.date.toString(Qt::ISODate), "2014-07-29T09:40:49");
		S_EQUAL(diag_status.user, "Max Mustermann");
		S_EQUAL(diag_status.dagnostic_status, "done");
		S_EQUAL(diag_status.outcome, "no significant findings");
		S_EQUAL(diag_status.genes_causal, "ATM");
		S_EQUAL(diag_status.inheritance_mode, "autosomal recessive");
		S_EQUAL(diag_status.genes_incidental, "BRCA2");
		S_EQUAL(diag_status.comments, "free text");
		//no entry in DB
		diag_status = db.getDiagnosticStatus(db.processedSampleId("NA12878_04"));
		S_EQUAL(diag_status.user, "");
		IS_FALSE(diag_status.date.isValid());
		S_EQUAL(diag_status.dagnostic_status, "");
		S_EQUAL(diag_status.outcome, "n/a");
		S_EQUAL(diag_status.genes_causal, "");
		S_EQUAL(diag_status.inheritance_mode, "n/a");
		S_EQUAL(diag_status.genes_incidental, "");
		S_EQUAL(diag_status.comments, "");

		//setDiagnosticStatus
		//create new entry
		diag_status.dagnostic_status = "done";
		diag_status.outcome = "significant findings";
		diag_status.genes_causal = "BRCA1";
		diag_status.inheritance_mode = "autosomal dominant";
		diag_status.genes_incidental = "TP53";
		diag_status.comments = "comment1";
		db.setDiagnosticStatus(db.processedSampleId("NA12878_04"), diag_status, "ahmustm1");
		diag_status = db.getDiagnosticStatus(db.processedSampleId("NA12878_04"));
		S_EQUAL(diag_status.user, "Max Mustermann");
		IS_TRUE(diag_status.date.isValid());
		S_EQUAL(diag_status.dagnostic_status, "done");
		S_EQUAL(diag_status.outcome, "significant findings");
		S_EQUAL(diag_status.genes_causal, "BRCA1");
		S_EQUAL(diag_status.inheritance_mode, "autosomal dominant");
		S_EQUAL(diag_status.genes_incidental, "TP53");
		S_EQUAL(diag_status.comments, "comment1");
		//update existing entry
		diag_status = db.getDiagnosticStatus(db.processedSampleId("NA12878_03"));
		diag_status.genes_incidental = "BRCA2,POLG";
		diag_status.comments = "comment2";
		db.setDiagnosticStatus(db.processedSampleId("NA12878_03"), diag_status, "ahmustm1");
		diag_status = db.getDiagnosticStatus(db.processedSampleId("NA12878_03"));
		IS_TRUE(diag_status.date.isValid());
		S_EQUAL(diag_status.user, "Max Mustermann");
		S_EQUAL(diag_status.dagnostic_status, "done");
		S_EQUAL(diag_status.outcome, "no significant findings");
		S_EQUAL(diag_status.genes_causal, "ATM");
		S_EQUAL(diag_status.inheritance_mode, "autosomal recessive");
		S_EQUAL(diag_status.genes_incidental, "BRCA2,POLG");
		S_EQUAL(diag_status.comments, "comment2");

		//setSampleDiseaseData
		sample_id = db.sampleId("NA12878");
		db.setSampleDiseaseData(sample_id, "Neoplasms", "Affected");
		sample_data = db.getSampleData(sample_id);
		S_EQUAL(sample_data.disease_group, "Neoplasms");
		S_EQUAL(sample_data.disease_status, "Affected");

		//getQCData
		QCCollection qc_data = db.getQCData(db.processedSampleId("NA12878_03"));
		I_EQUAL(qc_data.count(), 3);
		S_EQUAL(qc_data.value("target region 20x percentage").toString(2), "95.96");
		S_EQUAL(qc_data.value("target region read depth").toString(2), "103.24");
		S_EQUAL(qc_data.value("kasp").asString(), "n/a"); //special value that is not from the DB

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

		//getValidationStatus
		ValidationInfo val_info = db.getValidationStatus("NA12878_03", variant);
		S_EQUAL(val_info.status, "");
		S_EQUAL(val_info.type, "");
		S_EQUAL(val_info.comments, "");

		//setValidationStatus
		val_info.status = "to validate";
		val_info.type = "diagnostics";
		val_info.comments = "val_comm1";
		db.setValidationStatus("NA12878_03", variant, val_info, "ahmustm1");
		val_info = db.getValidationStatus("NA12878_03", variant);
		S_EQUAL(val_info.status, "to validate");
		S_EQUAL(val_info.type, "diagnostics");
		S_EQUAL(val_info.comments, "val_comm1");
		//update existing entry
		val_info.status = "to segregate";
		val_info.type = "research";
		val_info.comments = "val_comm2";
		db.setValidationStatus("NA12878_03", variant, val_info, "ahmustm1");
		val_info = db.getValidationStatus("NA12878_03", variant);
		S_EQUAL(val_info.status, "to segregate");
		S_EQUAL(val_info.type, "research");
		S_EQUAL(val_info.comments, "val_comm2");

		//getClassification
		ClassificationInfo class_info = db.getClassification(variant);
		S_EQUAL(class_info.classification, "");
		S_EQUAL(class_info.comments, "");

		//setClassification
		class_info.classification = "2";
		class_info.comments = "class_comm1";
		db.setClassification(variant, class_info);
		class_info = db.getClassification(variant);
		S_EQUAL(class_info.classification, "2");
		S_EQUAL(class_info.comments, "class_comm1");
		//update existing entry
		class_info.classification = "5";
		class_info.comments = "class_comm2";
		db.setClassification(variant, class_info);
		class_info = db.getClassification(variant);
		S_EQUAL(class_info.classification, "5");
		S_EQUAL(class_info.comments, "class_comm2");
	}

	//Test for debugging (without initialization because of speed)
	/*
	void debug()
	{
		QString host = Settings::string("ngsd_test_host");
		if (host=="") SKIP("Test needs access to the NGSD test database!");
		NGSD db(true);

		//getProcessingSystem
		QString sys = db.getProcessingSystem("NA12878_03", NGSD::SHORT);
		S_EQUAL(sys, "hpHBOCv5");
	}
	*/
};
