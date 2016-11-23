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
		QStringList genes = db.genesOverlapping("chr13", 90, 95, 0); //nearby left
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

		//sampleGender
		QString gender = db.sampleGender("NA12878_03");
		S_EQUAL(gender, "female");

		//genesToRegions
		QString messages;
		QTextStream stream(&messages);
		BedFile regions;

		messages.clear();
		regions = db.genesToRegions(QStringList() << "BRCA1", Transcript::CCDS, "gene", false, &stream); //gene mode, hit
		I_EQUAL(regions.baseCount(), 101);
		IS_TRUE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(QStringList() << "NIPA1", Transcript::CCDS, "gene", false, &stream); //gene mode, no hit
		I_EQUAL(regions.baseCount(), 0);
		IS_FALSE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(QStringList() << "NIPA1", Transcript::CCDS, "gene", true, &stream); //gene mode, no hit, fallback
		I_EQUAL(regions.baseCount(), 301);
		IS_TRUE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(QStringList() << "BRCA1", Transcript::CCDS, "exon", false, &stream); //exon mode, hit
		I_EQUAL(regions.baseCount(), 44);
		IS_TRUE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(QStringList() << "NIPA1", Transcript::CCDS, "exon", false, &stream); //exon mode, no hit
		I_EQUAL(regions.baseCount(), 0);
		IS_FALSE(messages.isEmpty());

		messages.clear();
		regions = db.genesToRegions(QStringList() << "NIPA1", Transcript::CCDS, "exon", true, &stream); //exon mode, no hit, fallback
		I_EQUAL(regions.baseCount(), 304);
		regions.merge(); //serveral tarnsc
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

		transcripts = db.transcripts(3, Transcript::UCSC, true); //NIPA1, UCSC, coding
		I_EQUAL(transcripts.count(), 2);
		S_EQUAL(transcripts[0].name(), "NIPA1_TR1");
		I_EQUAL(transcripts[0].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[0].source(), Transcript::UCSC);
		I_EQUAL(transcripts[0].regions().count(), 2);
		I_EQUAL(transcripts[0].regions().baseCount(), 202);
		S_EQUAL(transcripts[1].name(), "NIPA1_TR2");
		I_EQUAL(transcripts[1].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[1].source(), Transcript::UCSC);
		I_EQUAL(transcripts[1].regions().count(), 2);
		I_EQUAL(transcripts[1].regions().baseCount(), 102);

		transcripts = db.transcripts(3, Transcript::UCSC, false); //NIPA1, UCSC, non-coding
		I_EQUAL(transcripts.count(), 2);
		S_EQUAL(transcripts[0].name(), "NIPA1_TR1");
		I_EQUAL(transcripts[0].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[0].source(), Transcript::UCSC);
		I_EQUAL(transcripts[0].regions().count(), 2);
		I_EQUAL(transcripts[0].regions().baseCount(), 202);
		S_EQUAL(transcripts[1].name(), "NIPA1_TR2");
		I_EQUAL(transcripts[1].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[1].source(), Transcript::UCSC);
		I_EQUAL(transcripts[1].regions().count(), 4);
		I_EQUAL(transcripts[1].regions().baseCount(), 224);

		transcripts = db.transcripts(4, Transcript::UCSC, true); //NON-CODING, UCSC, coding
		I_EQUAL(transcripts.count(), 0);

		transcripts = db.transcripts(4, Transcript::UCSC, false); //NON-CODING, UCSC, non-coding
		I_EQUAL(transcripts.count(), 1);
		S_EQUAL(transcripts[0].name(), "NON-CODING_TR1");
		I_EQUAL(transcripts[0].regions().count(), 2);
		I_EQUAL(transcripts[0].regions().baseCount(), 202);

		//longestCodingTranscript
		Transcript transcript = db.longestCodingTranscript(4, Transcript::UCSC); //NON-CODING, zero transcripts
		IS_FALSE(transcript.isValid());

		transcript = db.longestCodingTranscript(1, Transcript::CCDS); //BRCA1, one transcript
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "BRCA1_TR1");
		I_EQUAL(transcript.regions().count(), 4);
		I_EQUAL(transcript.regions().baseCount(), 44);

		transcript = db.longestCodingTranscript(3, Transcript::UCSC); //NIPA1, two transcripts
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
