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

		//getProcessingSystem
		QString sys = db.getProcessingSystem("NA12878_03", NGSD::SHORT);
		S_EQUAL(sys, "hpHBOCv5");

		//geneToApproved
		auto gene_app = db.geneToApproved("BRCA1");
		S_EQUAL(gene_app.first, "BRCA1");
		S_EQUAL(gene_app.second, "KEPT: BRCA1 is an approved symbol");
		gene_app = db.geneToApproved("BLABLA");
		S_EQUAL(gene_app.first, "BLABLA");
		S_EQUAL(gene_app.second, "ERROR: BLABLA is unknown symbol");

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

		//sampleGender
		QString gender = db.sampleGender("NA12878_03");
		S_EQUAL(gender, "female");

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
