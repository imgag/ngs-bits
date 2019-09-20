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

		//processedSampleName
		QString ps_name = db.processedSampleName(db.processedSampleId("NA12878_03"), false);
		S_EQUAL(ps_name, "NA12878_03");

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
		S_EQUAL(processed_sample_data.run_name, "#00372");
		S_EQUAL(processed_sample_data.normal_sample_name, "");
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
		ginfo.oe_syn = 0.11;
		ginfo.oe_mis = 0.22;
		ginfo.oe_lof = 0.33;
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
		ginfo.oe_syn = 0.11;
		ginfo.oe_mis = 0.22;
		ginfo.oe_lof = 0.33;
		db.setGeneInfo(ginfo);
		ginfo = db.geneInfo("NEWGENE");
		S_EQUAL(ginfo.symbol, "NEWGENE");
		S_EQUAL(ginfo.oe_syn, "n/a");
		S_EQUAL(ginfo.oe_mis, "n/a");
		S_EQUAL(ginfo.oe_lof, "n/a");
		S_EQUAL(ginfo.inheritance, "AD");
		S_EQUAL(ginfo.comments, "comment");

		//precalculateGenotypeCounts
		messages.clear();
		db.precalculateGenotypeCounts(&stream, 50);
		I_EQUAL(db.getValue("SELECT count_hom FROM detected_variant_counts WHERE variant_id=2336993").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count_het FROM detected_variant_counts WHERE variant_id=2336993").toInt(), 1);
		I_EQUAL(db.getValue("SELECT count_hom FROM detected_variant_counts WHERE variant_id=2346586").toInt(), 2);
		I_EQUAL(db.getValue("SELECT count_het FROM detected_variant_counts WHERE variant_id=2346586").toInt(), 1);
		I_EQUAL(db.getValue("SELECT count_hom FROM detected_variant_counts WHERE variant_id=2407544").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count_het FROM detected_variant_counts WHERE variant_id=2407544").toInt(), 2);
		//by group
		I_EQUAL(db.getValue("SELECT COUNT(*) FROM detected_variant_counts_by_group").toInt(), 2);
		I_EQUAL(db.getValue("SELECT count_hom FROM detected_variant_counts_by_group WHERE variant_id=2346586 AND disease_group='Neoplasms'").toInt(), 1);
		I_EQUAL(db.getValue("SELECT count_het FROM detected_variant_counts_by_group WHERE variant_id=2346586 AND disease_group='Neoplasms'").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count_hom FROM detected_variant_counts_by_group WHERE variant_id=2407544 AND disease_group='Neoplasms'").toInt(), 0);
		I_EQUAL(db.getValue("SELECT count_het FROM detected_variant_counts_by_group WHERE variant_id=2407544 AND disease_group='Neoplasms'").toInt(), 1);
		//messages
		foreach(QString message,  messages.split("\n"))
		{
			//qDebug() << message;
		}

		//approvedGeneNames
		GeneSet approved = db.approvedGeneNames();
		I_EQUAL(approved.count(), 8);

		//phenotypes
		QList<Phenotype> phenos = db.phenotypes(QStringList() << "aBNOrmality");
		I_EQUAL(phenos.count(), 1);
		IS_TRUE(phenos.contains(Phenotype("HP:0000118","Phenotypic abnormality")));
		//synonyms
		phenos = db.phenotypes(QStringList() << "sYNonym");
		I_EQUAL(phenos.count(), 2);
		IS_TRUE(phenos.contains(Phenotype("HP:0012823","Clinical modifier")));
		IS_TRUE(phenos.contains(Phenotype("HP:0040279","Frequency")));
		//phenotypeByName
		Phenotype pheno = db.phenotypeByName("Frequency");
		S_EQUAL(pheno.accession(), "HP:0040279");
		S_EQUAL(pheno.name(), "Frequency");

		//phenotypeChildTems
		phenos = db.phenotypeChildTems(Phenotype("HP:0000001", "All"), true);
		I_EQUAL(phenos.count(), 10);
		phenos = db.phenotypeChildTems(Phenotype("HP:0000001", "All"), false);
		I_EQUAL(phenos.count(), 4);
		IS_TRUE(phenos.contains(Phenotype("HP:0000005","Mode of inheritance")));
		IS_TRUE(phenos.contains(Phenotype("HP:0000118","Phenotypic abnormality")));
		IS_TRUE(phenos.contains(Phenotype("HP:0012823","Clinical modifier")));
		IS_TRUE(phenos.contains(Phenotype("HP:0040279","Frequency")));
		//inner node
		phenos = db.phenotypeChildTems(Phenotype("HP:0000005", "Mode of inheritance"), true);
		I_EQUAL(phenos.count(), 6);
		IS_TRUE(phenos.contains(Phenotype("HP:0001419","X-linked recessive inheritance")));
		phenos = db.phenotypeChildTems(Phenotype("HP:0000005", "Mode of inheritance"), false);
		I_EQUAL(phenos.count(), 4);
		IS_FALSE(phenos.contains(Phenotype("HP:0001419","X-linked recessive inheritance")));
		//leaf
		phenos = db.phenotypeChildTems(Phenotype("HP:0001427", "Mitochondrial inheritance"), true);
		I_EQUAL(phenos.count(), 0);
		phenos = db.phenotypeChildTems(Phenotype("HP:0001427", "Mitochondrial inheritance"), false);
		I_EQUAL(phenos.count(), 0);

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
		db.setDiagnosticStatus(db.processedSampleId("NA12878_04"), diag_status, "ahmustm1");
		diag_status = db.getDiagnosticStatus(db.processedSampleId("NA12878_04"));
		S_EQUAL(diag_status.user, "Max Mustermann");
		IS_TRUE(diag_status.date.isValid());
		S_EQUAL(diag_status.dagnostic_status, "done");
		S_EQUAL(diag_status.outcome, "significant findings");
		S_EQUAL(diag_status.comments, "comment1");
		//update existing entry
		diag_status = db.getDiagnosticStatus(db.processedSampleId("NA12878_03"));
		diag_status.comments = "comment2";
		db.setDiagnosticStatus(db.processedSampleId("NA12878_03"), diag_status, "ahmustm1");
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
		db.queueAnalysis("single sample", true, QStringList() << "-steps ma,vc,an", QList<AnalysisJobSample>() << AnalysisJobSample { "NA12878_03", "index" }, "ahmustm1");
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
		bool canceled = db.cancelAnalysis(2, "ahmustm1");
		I_EQUAL(canceled, true);
		analysis_job = db.analysisInfo(2);
		I_EQUAL(analysis_job.history.count(), 2);
		S_EQUAL(analysis_job.history[0].status, "queued");
		S_EQUAL(analysis_job.history[0].user, "ahmustm1");
		S_EQUAL(analysis_job.history[1].status, "cancel");
		S_EQUAL(analysis_job.history[1].user, "ahmustm1");
		IS_TRUE(analysis_job.history[1].timeAsString().startsWith(QDate::currentDate().toString(Qt::ISODate)));
		S_EQUAL(analysis_job.history[1].output.join("\n"), "");

		canceled = db.cancelAnalysis(2, "ahmustm1");
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
		IS_TRUE(folder.endsWith("/test/KontrollDNACoriell/Sample_NA12878_03/"));
		db.queueAnalysis("somatic", false, QStringList(), QList<AnalysisJobSample>() << AnalysisJobSample{"NA12345_01", "tumor"} << AnalysisJobSample{"NA12878_03", "normal"}, "ahmustm1");
		folder = db.analysisJobFolder(3);
		IS_TRUE(folder.endsWith("/test/KontrollDNACoriell/Somatic_NA12345_01-NA12878_03/"));
		db.queueAnalysis("trio", false, QStringList(), QList<AnalysisJobSample>() << AnalysisJobSample{"NA12878_03", "child"} << AnalysisJobSample{"NA12123_04", "father"} << AnalysisJobSample{"NA12345_01", "mother"}, "ahmustm1");
		folder = db.analysisJobFolder(4);
		IS_TRUE(folder.endsWith("/test/KontrollDNACoriell/Trio_NA12878_03_NA12123_04_NA12345_01/"));
		db.queueAnalysis("multi sample", false, QStringList(), QList<AnalysisJobSample>() << AnalysisJobSample{"NA12123_04", "affected"} << AnalysisJobSample{"NA12345_01", "affected"}, "ahmustm1");
		folder = db.analysisJobFolder(5);
		IS_TRUE(folder.endsWith("/test/KontrollDNACoriell/Multi_NA12123_04_NA12345_01/"));

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
		QString var_id = db.addVariant(vl, 0);

		//variant
		IS_TRUE(db.variant(var_id)==vl[0]);

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
		I_EQUAL(ps_table.rowCount(), 5);
		I_EQUAL(ps_table.columnCount(), 18);
		//add path
		params.add_path = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 5);
		I_EQUAL(ps_table.columnCount(), 19);
		//add outcome
		params.add_outcome = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 5);
		I_EQUAL(ps_table.columnCount(), 21);
		//add path
		params.add_disease_details = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 5);
		I_EQUAL(ps_table.columnCount(), 28);
		//add QC
		params.add_qc = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 5);
		I_EQUAL(ps_table.columnCount(), 67);
		//apply all search parameters
		params.s_name = "NA12878";
		params.s_species = "human";
		params.include_bad_quality_samples = false;
		params.include_tumor_samples = false;
		params.include_ffpe_samples = false;
		params.p_name = "KontrollDNACoriell";
		params.p_type = "test";
		params.sys_name = "HBOC";
		params.sys_type = "Panel Haloplex";
		params.r_name = "#00372";
		params.include_bad_quality_runs = false;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 2);
		I_EQUAL(ps_table.columnCount(), 67);

		//reportConfigId
		QString ps_id = db.processedSampleId("NA12878_03");
		I_EQUAL(db.reportConfigId(ps_id), -1);

		//setReportConfig
		ReportVariantConfiguration report_var_conf;
		report_var_conf.variant_type = VariantType::SNVS_INDELS;
		report_var_conf.variant_index = 47;
		report_var_conf.causal = true;
		report_var_conf.type = "candidate variant";
		report_var_conf.mosaic = true;
		report_var_conf.exclude_artefact = true;
		report_var_conf.comments = "com1";
		report_var_conf.comments2 = "com2";
		ReportConfiguration report_conf;
		report_conf.setCreatedBy("ahmustm1");
		report_conf.set(report_var_conf);
		QString conf_id1 = db.setReportConfig(ps_id, report_conf, vl);
		QString conf_id2 = db.setReportConfig(ps_id, report_conf, vl);
		IS_TRUE(conf_id1!=conf_id2);

		//reportConfigId
		int conf_id = db.reportConfigId(ps_id);
		IS_TRUE(conf_id!=-1);
		S_EQUAL(db.reportConfigCreationData(conf_id).first, "Max Mustermann");

		//reportConfig
		QStringList messages2;
		ReportConfiguration report_conf2 = db.reportConfig(ps_id, vl, messages2);
		I_EQUAL(messages2.count(), 0);
		S_EQUAL(report_conf2.createdBy(), "Max Mustermann");
		IS_TRUE(report_conf2.createdAt().date()==QDate::currentDate());
		I_EQUAL(report_conf2.variantConfig().count(), 1);
		IS_TRUE(report_conf2.variantConfig()[0].causal);
		S_EQUAL(report_conf2.variantConfig()[0].type, report_var_conf.type);
		IS_TRUE(report_conf2.variantConfig()[0].mosaic);
		IS_TRUE(report_conf2.variantConfig()[0].exclude_artefact);
		S_EQUAL(report_conf2.variantConfig()[0].comments, report_var_conf.comments);
		S_EQUAL(report_conf2.variantConfig()[0].comments2, report_var_conf.comments2);
		IS_FALSE(report_conf2.variantConfig()[0].de_novo);
		IS_FALSE(report_conf2.variantConfig()[0].comp_het);
		IS_FALSE(report_conf2.variantConfig()[0].exclude_frequency);
		IS_FALSE(report_conf2.variantConfig()[0].exclude_mechanism);
		IS_FALSE(report_conf2.variantConfig()[0].exclude_other);
		IS_FALSE(report_conf2.variantConfig()[0].exclude_phenotype);

		vl.clear();
		report_conf2 = db.reportConfig(ps_id, vl, messages2);
		I_EQUAL(messages2.count(), 1);
		S_EQUAL(messages2[0], "Could not find variant 'chr2:47635523-47635523 ->T' in given variant list!");
		I_EQUAL(report_conf2.variantConfig().count(), 0);
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
