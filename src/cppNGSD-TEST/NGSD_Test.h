#include "TestFramework.h"
#include "Settings.h"
#include "NGSD.h"
#include "LoginManager.h"
#include "SomaticXmlReportGenerator.h"
#include "SomaticReportSettings.h"
#include "GermlineReportGenerator.h"
#include "TumorOnlyReportWorker.h"

#include <QThread>
#include <cmath>
#include <QCoreApplication>

TEST_CLASS(NGSD_Test)
{
Q_OBJECT
private slots:

	void batch_test_HGVS2GSvar()
	{
		return; //TODO check wrong variants - is VEP wrong there?
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex idx(ref_file);

		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");
		NGSD db;

		QHash<QString, Transcript> transcrips;
		VariantList vl;
		vl.load("D:\\Data\\NGS\\Sample_NA12878_38\\NA12878_38.GSvar");
		int i_co = vl.annotationIndexByName("coding_and_splicing");
		int tests = 0;
		int correct = 0;
		int wrong = 0;
		int error = 0;
		for (int i=0; i<vl.count(); ++i)
		{
			QStringList vep_annos = QString(vl[i].annotations()[i_co]).split(',');
			foreach(QString vep_anno, vep_annos)
			{
				QStringList parts = vep_anno.split(":");
				//qDebug() << parts;
				QString gene = parts[0];
				QString trans_name = parts[1];
				QString type = parts[2];
				QString cdna = parts[5].trimmed();
				if (cdna.isEmpty()) continue;

				if (!transcrips.contains(trans_name))
				{
					try
					{
						int trans_id = db.transcriptId(trans_name);
						transcrips[trans_name] = db.transcript(trans_id);
					}
					catch(Exception& e)
					{
						continue;
					}
				}

				++tests;
				Transcript trans = transcrips[trans_name];
				try
				{
					Variant v = trans.hgvsToVariant(cdna, idx);
					if (v==vl[i]) ++correct;
					else
					{
						++wrong;
						qDebug() << "Conversion wrong:" << gene << trans_name << cdna << type;
						qDebug() << "  should be:" << vl[i].toString();
						qDebug() << "  is       :" << v.toString();
					}
				}
				catch(Exception& e)
				{
					++error;
					qDebug() << "Conversion error:" << gene << trans_name << cdna << type;
					qDebug() << "  " << e.message();
				}
			}
		}

		qDebug() << "variants: " << vl.count();
		qDebug() << "tests   : " << tests;
		qDebug() << "correct : " << correct << " (" << QString::number(100.0 * correct / tests, 'f', 3) << "%)";
		qDebug() << "wrong   : " << wrong << " (" << QString::number(100.0 * wrong / tests, 'f', 3) << "%)";
		qDebug() << "error   : " << error << " (" << QString::number(100.0 * error / tests, 'f', 3) << "%)";
	}

	//Normally, one member is tested in one QT slot.
	//Because initializing the database takes very long, all NGSD functionality is tested in one slot.
	void main_tests()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in1.sql"));

		//log in user
		LoginManager::login("ahmustm1", true);

		//escapeText
		S_EQUAL(db.escapeText("; '"), "'; '''");

		//getEnum
		QStringList enum_values = db.getEnum("sample", "disease_group");
		I_EQUAL(enum_values.count(), 18);
		S_EQUAL(enum_values[4], "Endocrine, nutritional or metabolic diseases");

		//processedSampleName
		QString ps_name = db.processedSampleName(db.processedSampleId("NA12878_03"), false);
		S_EQUAL(ps_name, "NA12878_03");

		//processingSystemIdFromSample
		int sys_id = db.processingSystemIdFromProcessedSample(ps_name);
		S_EQUAL(sys_id, 1);

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
		S_EQUAL(processed_sample_data.processing_system, "HaloPlex HBOC v5");
		S_EQUAL(processed_sample_data.processing_system_type, "Panel Haloplex");
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
		QList<Transcript> transcripts = db.transcripts(1, Transcript::CCDS, true); //BRCA1, CCDS, coding
		I_EQUAL(transcripts.count(), 1);
		S_EQUAL(transcripts[0].name(), "BRCA1_TR1");
		I_EQUAL(transcripts[0].strand(), Transcript::PLUS);
		I_EQUAL(transcripts[0].source(), Transcript::CCDS);
		I_EQUAL(transcripts[0].regions().count(), 4);
		I_EQUAL(transcripts[0].regions().baseCount(), 44);
		I_EQUAL(transcripts[0].codingRegions().count(), 4);
		I_EQUAL(transcripts[0].codingRegions().baseCount(), 44);

		transcripts = db.transcripts(3, Transcript::ENSEMBL, true); //NIPA1, Ensembl, coding
		I_EQUAL(transcripts.count(), 2);
		S_EQUAL(transcripts[0].name(), "NIPA1_TR1");
		I_EQUAL(transcripts[0].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[0].source(), Transcript::ENSEMBL);
		I_EQUAL(transcripts[0].regions().count(), 2);
		I_EQUAL(transcripts[0].regions().baseCount(), 202);
		I_EQUAL(transcripts[0].codingRegions().count(), 2);
		I_EQUAL(transcripts[0].codingRegions().baseCount(), 202);
		S_EQUAL(transcripts[1].name(), "NIPA1_TR2");
		I_EQUAL(transcripts[1].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[1].source(), Transcript::ENSEMBL);
		I_EQUAL(transcripts[1].regions().count(), 4);
		I_EQUAL(transcripts[1].regions().baseCount(), 224);
		I_EQUAL(transcripts[1].codingRegions().count(), 2);
		I_EQUAL(transcripts[1].codingRegions().baseCount(), 102);

		transcripts = db.transcripts(3, Transcript::ENSEMBL, false); //NIPA1, Ensembl, non-coding
		I_EQUAL(transcripts.count(), 2);
		S_EQUAL(transcripts[0].name(), "NIPA1_TR1");
		I_EQUAL(transcripts[0].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[0].source(), Transcript::ENSEMBL);
		I_EQUAL(transcripts[0].regions().count(), 2);
		I_EQUAL(transcripts[0].regions().baseCount(), 202);
		I_EQUAL(transcripts[0].codingRegions().count(), 2);
		I_EQUAL(transcripts[0].codingRegions().baseCount(), 202);
		S_EQUAL(transcripts[1].name(), "NIPA1_TR2");
		I_EQUAL(transcripts[1].strand(), Transcript::MINUS);
		I_EQUAL(transcripts[1].source(), Transcript::ENSEMBL);
		I_EQUAL(transcripts[1].regions().count(), 4);
		I_EQUAL(transcripts[1].regions().baseCount(), 224);
		I_EQUAL(transcripts[1].codingRegions().count(), 2);
		I_EQUAL(transcripts[1].codingRegions().baseCount(), 102);

		transcripts = db.transcripts(4, Transcript::ENSEMBL, true); //NON-CODING, Ensembl, coding
		I_EQUAL(transcripts.count(), 0);

		transcripts = db.transcripts(4, Transcript::ENSEMBL, false); //NON-CODING, Ensembl, non-coding
		I_EQUAL(transcripts.count(), 1);
		S_EQUAL(transcripts[0].name(), "NON-CODING_TR1");
		I_EQUAL(transcripts[0].regions().count(), 2);
		I_EQUAL(transcripts[0].regions().baseCount(), 202);
		I_EQUAL(transcripts[0].codingRegions().count(), 0);
		I_EQUAL(transcripts[0].codingRegions().baseCount(), 0);

		//longestCodingTranscript
		transcript = db.longestCodingTranscript(4, Transcript::ENSEMBL); //NON-CODING, zero transcripts
		IS_FALSE(transcript.isValid());

		transcript = db.longestCodingTranscript(1, Transcript::CCDS); //BRCA1, one transcript
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "BRCA1_TR1");
		I_EQUAL(transcript.regions().count(), 4);
		I_EQUAL(transcript.regions().baseCount(), 44);
		I_EQUAL(transcript.codingRegions().count(), 4);
		I_EQUAL(transcript.codingRegions().baseCount(), 44);

		transcript = db.longestCodingTranscript(3, Transcript::ENSEMBL); //NIPA1, two transcripts
		IS_TRUE(transcript.isValid());
		S_EQUAL(transcript.name(), "NIPA1_TR1");
		I_EQUAL(transcript.regions().count(), 2);
		I_EQUAL(transcript.regions().baseCount(), 202);
		I_EQUAL(transcript.codingRegions().count(), 2);
		I_EQUAL(transcript.codingRegions().baseCount(), 202);

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

		//approvedGeneNames
		GeneSet approved = db.approvedGeneNames();
		I_EQUAL(approved.count(), 18);

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

		//variantCounts
		QString variant_id = db.variantId(Variant("chr10",43613843,43613843,"G","T")); //hom
		QPair<int, int> ngsd_counts = db.variantCounts(variant_id);
		I_EQUAL(ngsd_counts.first, 0);
		I_EQUAL(ngsd_counts.second, 1);

		variant_id = db.variantId(Variant("chr17",7579472,7579472,"G","C")); //het
		ngsd_counts = db.variantCounts(variant_id);
		I_EQUAL(ngsd_counts.first, 1);
		I_EQUAL(ngsd_counts.second, 0);

		ngsd_counts = db.variantCounts(variant_id, true);
		I_EQUAL(ngsd_counts.first, 0);
		I_EQUAL(ngsd_counts.second, 0);

		db.getQuery().exec("UPDATE variant SET germline_het=17, germline_hom=7 WHERE id=" + variant_id);
		ngsd_counts = db.variantCounts(variant_id, true);
		I_EQUAL(ngsd_counts.first, 17);
		I_EQUAL(ngsd_counts.second, 7);

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
		I_EQUAL(ps_table.rowCount(), 9);
		I_EQUAL(ps_table.columnCount(), 19);
		//add path
		params.add_path = "SAMPLE_FOLDER";
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 9);
		I_EQUAL(ps_table.columnCount(), 20);
		//add outcome
		params.add_outcome = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 9);
		I_EQUAL(ps_table.columnCount(), 22);
		//add disease details
		params.add_disease_details = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 9);
		I_EQUAL(ps_table.columnCount(), 31);
		//add QC
		params.add_qc = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 9);
		I_EQUAL(ps_table.columnCount(), 70);
		//add report config
		params.add_report_config = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 9);
		I_EQUAL(ps_table.columnCount(), 71);
		S_EQUAL(ps_table.row(0).value(70), "");
		S_EQUAL(ps_table.row(4).value(70), "exists, causal variant: chr9:98232224-98232224 A>- (genotype:het genes:PTCH1), causal CNV: chr1:3000-4000 (cn:1 classification:4)");
		//add comments
		params.add_comments = true;
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 9);
		I_EQUAL(ps_table.columnCount(), 73);
		S_EQUAL(ps_table.headers().at(19), "comment_sample");
		S_EQUAL(ps_table.headers().at(20), "comment_processed_sample");
		S_EQUAL(ps_table.row(0).value(19), "comment_s6");
		S_EQUAL(ps_table.row(0).value(20), "comment_ps7");


		//apply all search parameters
		params.s_name = "NA12878";
		params.s_species = "human";
		params.s_sender = "Coriell";
		params.s_study = "SomeStudy";
		params.include_bad_quality_samples = false;
		params.include_tumor_samples = false;
		params.include_ffpe_samples = false;
		params.p_name = "KontrollDNACoriell";
		params.p_type = "test";
		params.sys_name = "hpHBOCv5";
		params.sys_type = "Panel Haloplex";
		params.r_name = "#00372";
		params.r_device_name = "Neo";
		params.include_bad_quality_runs = false;
		params.run_finished = true;
		params.r_before = QDate::fromString("2021-02-19", Qt::ISODate);
		ps_table = db.processedSampleSearch(params);
		I_EQUAL(ps_table.rowCount(), 2);
		I_EQUAL(ps_table.columnCount(), 73);

		//reportConfigId
		QString ps_id = db.processedSampleId("NA12878_03");
		I_EQUAL(db.reportConfigId(ps_id), -1);

		//setReportConfig
		CnvList cnvs;
		cnvs.load(TESTDATA("data_in/cnvs_clincnv.tsv"));
		BedpeFile svs;
		svs.load(TESTDATA("data_in/sv_manta.bedpe"));

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

		int conf_id1 = db.setReportConfig(ps_id, report_conf, vl, cnvs, svs);

		//reportConfigId
		int conf_id = db.reportConfigId(ps_id);
		IS_TRUE(conf_id!=-1);

		//check data
		QStringList messages2;
		QSharedPointer<ReportConfiguration> report_conf2 = db.reportConfig(conf_id, vl, cnvs, svs, messages2);
		S_EQUAL(report_conf2->createdBy(), "Max Mustermann");
		IS_TRUE(report_conf2->createdAt().isValid());
		S_EQUAL(report_conf2->lastUpdatedBy(), "Max Mustermann");
		IS_TRUE(report_conf2->lastUpdatedAt().isValid());
		S_EQUAL(report_conf2->finalizedBy(), "");
		IS_FALSE(report_conf2->finalizedAt().isValid());

		//update
		QThread::sleep(4);
		int conf_id2 = db.setReportConfig(ps_id, report_conf, vl, cnvs, svs);
		IS_TRUE(conf_id1==conf_id2);
		//check that no double entries are inserted after second execution of setReportConfig
		I_EQUAL(db.getValue("SELECT count(*) FROM cnv WHERE cnv_callset_id=1 AND chr='chr2' AND start=89246800 AND end=89545067 AND cn=1").toInt(), 1);

		//check data
		report_conf2 = db.reportConfig(conf_id, vl, cnvs, svs, messages2);
		S_EQUAL(report_conf2->createdBy(), "Max Mustermann");
		IS_TRUE(report_conf2->createdAt().isValid());
		S_EQUAL(report_conf2->lastUpdatedBy(), "Max Mustermann");
		IS_TRUE(report_conf2->lastUpdatedAt().isValid());
		IS_TRUE(report_conf2->createdAt()!=report_conf2->lastUpdatedAt());


		S_EQUAL(report_conf2->finalizedBy(), "");
		IS_FALSE(report_conf2->finalizedAt().isValid());

		I_EQUAL(messages2.count(), 0);
		S_EQUAL(report_conf2->createdBy(), "Max Mustermann");
		IS_TRUE(report_conf2->createdAt().date()==QDate::currentDate());
		I_EQUAL(report_conf2->variantConfig().count(), 3);
		ReportVariantConfiguration var_conf = report_conf2->variantConfig()[1]; //order changed because they are sorted by index
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
		var_conf = report_conf2->variantConfig()[0]; //order changed because they are sorted by index
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
		var_conf = report_conf2->variantConfig()[2];
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

		//finalizeReportConfig
		conf_id = db.setReportConfig(ps_id, report_conf, vl, cnvs, svs);
		IS_FALSE(db.reportConfigIsFinalized(conf_id));
		db.finalizeReportConfig(conf_id, db.userId("ahmustm1"));
		IS_TRUE(db.reportConfigIsFinalized(conf_id));
		report_conf2 = db.reportConfig(conf_id, vl, cnvs, svs, messages2);
		S_EQUAL(report_conf2->finalizedBy(), "Max Mustermann");
		IS_TRUE(report_conf2->finalizedAt().isValid());
		//check finalized report config cannot be modified or deleted
		IS_THROWN(ProgrammingException, db.setReportConfig(ps_id, report_conf, vl, cnvs, svs));
		IS_THROWN(ProgrammingException, db.deleteReportConfig(conf_id));

		//check messages if variant is missing
		vl.clear();
		report_conf2 = db.reportConfig(conf_id, vl, cnvs, svs, messages2);
		I_EQUAL(messages2.count(), 1);
		S_EQUAL(messages2[0], "Could not find variant 'chr2:47635523-47635523 ->T' in given variant list!");
		I_EQUAL(report_conf2->variantConfig().count(), 2);
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

		cnv_list.load(TESTDATA("data_in/cnvs_cnvhunter.tsv"));
		cnv_id = db.addCnv(1, cnv_list[1], cnv_list);
		S_EQUAL(db.getValue("SELECT cn FROM cnv WHERE id="+cnv_id).toString(), "1");
		S_EQUAL(db.getValue("SELECT quality_metrics FROM cnv WHERE id="+cnv_id).toString(), "{\"region_zscores\":\"-4.48,-3.45,-4.27\",\"regions\":\"3\"}");
		S_EQUAL(cnv_id, "7");

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
		S_EQUAL(omim_info[0].phenotypes[1].accession(), "");
		S_EQUAL(omim_info[0].phenotypes[1].name(), "Lymphoma, B-cell non-Hodgkin, somatic (3)");

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
		I_EQUAL(pt.count(), 2);
		IS_TRUE(pt.contains("NIPA1"));
		IS_TRUE(pt.contains("NON-CODING"));
		I_EQUAL(pt["NIPA1"].count(), 2);
		IS_TRUE(pt["NIPA1"].contains("NIPA1_TR1"));
		IS_TRUE(pt["NIPA1"].contains("NIPA1_TR2"));
		I_EQUAL(pt["NON-CODING"].count(), 1);
		IS_TRUE(pt["NON-CODING"].contains("NON-CODING_TR1"));

		//addSampleRelation
		db.addSampleRelation(SampleRelation{"NA12345", "siblings", "NA12878"});
		db.addSampleRelation(SampleRelation{"NA12345", "siblings", "NA12878"}); //ignored
		IS_THROWN(DatabaseException, db.addSampleRelation(SampleRelation{"NA12345", "siblings", "NA12878"}, true));

		//omimPreferredPhenotype
		S_EQUAL(db.omimPreferredPhenotype("BRCA1", "Neoplasms"), "");
		S_EQUAL(db.omimPreferredPhenotype("ATM", "Diseases of the immune system"), "");
		S_EQUAL(db.omimPreferredPhenotype("ATM", "Neoplasms"), "114480");


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
		S_EQUAL(loaded_panel_info.created_by, panel_info.created_by);
		IS_TRUE(loaded_panel_info.created_date == panel_info.created_date);
		S_EQUAL(loaded_panel_info.processing_system_id, panel_info.processing_system_id);

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

	}

	inline void report_germline()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		//init NGSD
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in2.sql"));
		LoginManager::login("ahmustm1", true);

		QDate report_date = QDate::fromString("2021-02-19", Qt::ISODate);

		//setup
		VariantList variants;
		variants.load(TESTDATA("../cppNGS-TEST/data_in/panel.GSvar"));
		CnvList cnvs;
		cnvs.load(TESTDATA("../cppNGS-TEST/data_in/panel_cnvs_clincnv.tsv"));
		BedpeFile svs;
		svs.load(TESTDATA("../cppNGS-TEST/data_in/panel_svs.bedpe"));
		PrsTable prs;
		prs.load(TESTDATA("../cppNGS-TEST/data_in/panel_prs.tsv"));
		ReportSettings report_settings;
		report_settings.report_type = "diagnostic variant";
		report_settings.min_depth = 20;
		report_settings.show_coverage_details = false;
		report_settings.roi_low_cov = false;
		report_settings.recalculate_avg_depth = false;
		report_settings.show_omim_table = false;
		report_settings.show_one_entry_in_omim_table = false;
		report_settings.show_class_details = false;
		FilterCascade filters;
		filters.add(QSharedPointer<FilterBase>(new FilterAlleleFrequency()));
		QMap<QByteArray, QByteArrayList> preferred_transcripts;
		preferred_transcripts.insert("SPG7", QByteArrayList() << "ENST00000268704");
		GermlineReportGeneratorData data(GenomeBuild::HG19, "NA12878_03", variants, cnvs, svs, prs, report_settings, filters, preferred_transcripts);
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
			report_settings.selected_variants.append(qMakePair(VariantType::SNVS_INDELS, 252)); //small variant - chr13:41367370 C>G (SPG7)
			ReportVariantConfiguration var_conf;
			var_conf.variant_type = VariantType::SNVS_INDELS;
			var_conf.variant_index = 252;
			var_conf.causal = true;
			var_conf.mosaic = true;
			var_conf.de_novo = true;
			var_conf.comp_het = false;
			var_conf.report_type = "diagnostic variant";
			report_settings.report_config->set(var_conf);

			report_settings.selected_variants.append(qMakePair(VariantType::SNVS_INDELS, 173)); //small variant - chr13:41367370 C>G (SLC25A15)
			var_conf.variant_type = VariantType::SNVS_INDELS;
			var_conf.variant_index = 173;
			var_conf.causal = false;
			var_conf.mosaic = false;
			var_conf.de_novo = false;
			var_conf.comp_het = true;
			var_conf.report_type = "diagnostic variant";
			report_settings.report_config->set(var_conf);

			report_settings.selected_variants.append(qMakePair(VariantType::CNVS, 0)); //CNV - het deletion
			var_conf.variant_type = VariantType::CNVS;
			var_conf.variant_index = 0;
			var_conf.causal = false;
			var_conf.mosaic = false;
			var_conf.de_novo = false;
			var_conf.comp_het = true;
			var_conf.report_type = "diagnostic variant";
			report_settings.report_config->set(var_conf);

			report_settings.selected_variants.append(qMakePair(VariantType::SVS, 0)); //SV - breakpoint
			var_conf.variant_type = VariantType::SVS;
			var_conf.variant_index = 0;
			var_conf.causal = false;
			var_conf.mosaic = false;
			var_conf.de_novo = false;
			var_conf.comp_het = false;
			var_conf.report_type = "diagnostic variant";
			report_settings.report_config->set(var_conf);

			report_settings.show_coverage_details = true;
			report_settings.roi_low_cov = true;
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


		//############################### TEST 4 - evaluation sheet ###############################
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
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

		QCoreApplication::setApplicationVersion("0.1-cppNGSD-TEST-Version"); //application version (is written into somatic xml report)
		//init
		NGSD db(true);
		db.init();
		db.executeQueriesFromFile(TESTDATA("data_in/NGSD_in1.sql"));
		//log in user
		LoginManager::login("ahmustm1", true);



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
		som_rep_conf.set(var1);
		som_rep_conf.set(var2);
		som_rep_conf.setCreatedBy("ahmustm1");
		som_rep_conf.setTargetRegionName("/path/to/somewhere.bed");
		som_rep_conf.setTumContentByMaxSNV(true);
		som_rep_conf.setTumContentByClonality(true);
		som_rep_conf.setTumContentByHistological(true);
		som_rep_conf.setMsiStatus(true);
		som_rep_conf.setCnvBurden(true);
		som_rep_conf.setHrdScore(4);
		som_rep_conf.setHrdStatement("undeterminable");
		som_rep_conf.setCnvLohCount(12);
		som_rep_conf.setCnvTaiCount(3);
		som_rep_conf.setCnvLstCount(43);
		som_rep_conf.setTmbReferenceText("Median: 1.70 Var/Mbp, Maximum: 10.80 Var/Mbp, Probenanzahl:65 (PMID: 28420421)");
		som_rep_conf.setQuality("DNA quantity too low");
		som_rep_conf.setFusionsDetected(true);

		som_rep_conf.setCinChromosomes({"chr1", "chr5", "chr9", "chrX", "chrY"});
		som_rep_conf.setLimitations("Due to low coverage we could not detect all variants for gene BRAF.");
		som_rep_conf.setFilter("somatic");



		SomaticReportVariantConfiguration cnv1;
		cnv1.variant_index = 2;
		cnv1.variant_type = VariantType::CNVS;
		cnv1.exclude_artefact = true;
		cnv1.exclude_other_reason = true;
		cnv1.comment = "This test somatic cnv shall be excluded.";
		som_rep_conf.set(cnv1);

		SomaticReportGermlineVariantConfiguration var1_germl, var2_germl;
		var1_germl.variant_index = 2;
		var1_germl.tum_freq = 0.7;
		var1_germl.tum_depth = 1210;

		var2_germl.variant_index = 4;
		var2_germl.tum_freq = 0.68;
		var2_germl.tum_depth = 1022;
		som_rep_conf.setGermline(var1_germl);
		som_rep_conf.setGermline(var2_germl);


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
		int config_id = db.setSomaticReportConfig(t_ps_id, n_ps_id, som_rep_conf, vl, cnvs, vl_germl, "ahmustm1"); //id will be 52 in test NGSD



		QStringList messages = {};

		//Test resolving report config
		SomaticReportConfiguration res_config = db.somaticReportConfig(t_ps_id, n_ps_id, vl, cnvs, vl_germl, messages);
		IS_TRUE(res_config.tumContentByMaxSNV());
		IS_TRUE(res_config.tumContentByClonality());
		IS_TRUE(res_config.tumContentByHistological());
		IS_TRUE(res_config.msiStatus());
		IS_TRUE(res_config.cnvBurden());
		I_EQUAL(res_config.hrdScore(), 4);
		S_EQUAL(res_config.hrdStatement(), "undeterminable");
		I_EQUAL(res_config.cnvLohCount(), 12);
		I_EQUAL(res_config.cnvTaiCount(), 3);
		I_EQUAL(res_config.cnvLstCount(), 43);

		S_EQUAL(res_config.tmbReferenceText(), "Median: 1.70 Var/Mbp, Maximum: 10.80 Var/Mbp, Probenanzahl:65 (PMID: 28420421)");
		S_EQUAL(res_config.quality(), "DNA quantity too low");
		IS_TRUE(res_config.fusionsDetected());
		S_EQUAL(res_config.cinChromosomes().join(','), "chr1,chr5,chr9,chrX,chrY");
		IS_THROWN(ArgumentException, som_rep_conf.setCinChromosomes({"chr1", "chr24"}));
		S_EQUAL(res_config.limitations(), "Due to low coverage we could not detect all variants for gene BRAF.");
		S_EQUAL(res_config.filter(), "somatic");

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
		S_EQUAL(res1.comment, "known test driver was not included in any db yet.");
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
		som_rep_conf.setTumContentByMaxSNV(false);
		som_rep_conf.setTumContentByClonality(false);
		som_rep_conf.setTumContentByHistological(false);
		som_rep_conf.setMsiStatus(false);
		som_rep_conf.setCnvBurden(false);
		som_rep_conf.setHrdScore(0);

		som_rep_conf.setHrdStatement("proof");
		som_rep_conf.setCnvLohCount(9);
		som_rep_conf.setCnvTaiCount(1);
		som_rep_conf.setCnvLstCount(23);


		som_rep_conf.setTmbReferenceText("An alternative tmb reference value.");
		som_rep_conf.setQuality("NON EXISTING IN SOMTATIC_REPORT_CONFIGURATION TABLE");
		som_rep_conf.setFusionsDetected(false);
		som_rep_conf.setCinChromosomes({"chr10","chr21"});
		som_rep_conf.setLimitations("With German umlauts: ???????");
		som_rep_conf.setFilter("");



		db.setSomaticReportConfig(t_ps_id, n_ps_id, som_rep_conf, vl, cnvs, vl_germl, "ahkerra1");

		SomaticReportConfiguration res_config_2 = db.somaticReportConfig(t_ps_id, n_ps_id, vl, cnvs, vl_germl, messages);
		IS_FALSE(res_config_2.tumContentByMaxSNV());
		IS_FALSE(res_config_2.tumContentByClonality());
		IS_FALSE(res_config_2.tumContentByHistological());
		IS_FALSE(res_config_2.msiStatus());
		IS_FALSE(res_config_2.cnvBurden());
		I_EQUAL(res_config_2.hrdScore(), 0);

		S_EQUAL(res_config_2.hrdStatement(), "proof");
		I_EQUAL(res_config_2.cnvLohCount(), 9);
		I_EQUAL(res_config_2.cnvTaiCount(), 1);
		I_EQUAL(res_config_2.cnvLstCount(), 23);

		S_EQUAL(res_config_2.tmbReferenceText(), "An alternative tmb reference value.");
		S_EQUAL(res_config_2.quality(), "");
		IS_FALSE(res_config_2.fusionsDetected());
		S_EQUAL(res_config_2.cinChromosomes().join(','), "chr10,chr21");
		S_EQUAL(res_config_2.limitations(), "With German umlauts: ???????");
		S_EQUAL(res_config_2.filter(), "");

		SomaticReportConfigurationData config_data_2 =  db.somaticReportConfigData(config_id);
		S_EQUAL(config_data_2.created_by, "Max Mustermann");
		S_EQUAL(config_data_2.last_edit_by, "Sarah Kerrigan");
		S_EQUAL(config_data_2.target_file, "else.bed");
		IS_TRUE(config_data_2.created_date == config_data_1.created_date);
		IS_TRUE(config_data_2.last_edit_date != "");

		//report config in case of no target file
		som_rep_conf.setTargetRegionName("");
		db.setSomaticReportConfig(t_ps_id, n_ps_id, som_rep_conf, vl, cnvs, vl_germl, "ahkerra1");
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


		//Test somatic XML report

		VariantList vl_filtered = SomaticReportSettings::filterVariants(vl, settings);
		VariantList vl_germl_filtered =  SomaticReportSettings::filterGermlineVariants(vl_germl, settings);
		CnvList cnvs_filtered = SomaticReportSettings::filterCnvs(cnvs,settings);

		SomaticXmlReportGeneratorData xml_data(GenomeBuild::HG19, settings, vl_filtered, vl_germl_filtered, cnvs_filtered);
		xml_data.processing_system_roi.load(TESTDATA("../cppNGSD-TEST/data_in/ssSC_test.bed"));
		xml_data.processing_system_genes = GeneSet::createFromFile(TESTDATA("../cppNGSD-TEST/data_in/ssSC_test_genes.txt"));
		IS_THROWN(ArgumentException, xml_data.check());

		xml_data.mantis_msi = 0.74;
		xml_data.tumor_content_histology = 0.6;
		xml_data.tumor_mutation_burden = 17.3;
		xml_data.tumor_content_clonality = 0.8;
		xml_data.tumor_content_snvs = 0.73;


		QString out = SomaticXmlReportGenerator::generateXML(xml_data, db, true);

		Helper::storeTextFile("out/somatic_report.xml", out.split("\n"));
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

		S_EQUAL(vicc_data1.created_by, "ahmustm1");
		S_EQUAL(vicc_data1.created_at.toString("yyyy-MM-dd hh:mm:ss"), "2020-11-05 13:06:13");
		S_EQUAL(vicc_data1.last_updated_by, "ahkerra1");
		S_EQUAL(vicc_data1.last_updated_at.toString("yyyy-MM-dd hh:mm:ss"), "2020-12-07 11:06:10");



		SomaticViccData vicc_data2 = db.getSomaticViccData( Variant("chr15", 43707808, 43707808, "A", "T" ) );
		S_EQUAL(vicc_data2.comment, "this variant was evaluated as variant of unclear significance");
		S_EQUAL(vicc_data2.last_updated_by, "ahkerra1");
		S_EQUAL(vicc_data2.last_updated_at.toString("yyyy-MM-dd hh:mm:ss"),  "2020-12-08 13:45:11");

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

	}

	//Test tumor only RTF report generation
	void report_tumor_only()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");

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
		config.ps = "DX000001_01";
		config.roi.name = "tum_only_target_filter";
		config.roi.genes = GeneSet::createFromStringList(QStringList() << "MITF");

		BedFile tum_only_roi_filter;
		tum_only_roi_filter.load(TESTDATA("data_in/tumor_only_target_region.bed"));
		config.roi.regions = tum_only_roi_filter;
		config.bam_file = TESTDATA("data_in/tumor_only.bam");
		config.include_coverage_per_gap = true;
		config.include_exon_number_per_gap = true;
		config.use_test_db = true;

		//create RTF report with 2 SNVs and two gaps
		TumorOnlyReportWorker report_worker(vl, config);
		report_worker.checkAnnotation(vl);
		report_worker.writeRtf("out/tumor_only_report.rtf");

		REMOVE_LINES("out/tumor_only_report.rtf", QRegExp(QDate::currentDate().toString("dd.MM.yyyy").toUtf8())); //today's date
		REMOVE_LINES("out/tumor_only_report.rtf", QRegExp(QCoreApplication::applicationName().toUtf8())); //application name and version

		COMPARE_FILES("out/tumor_only_report.rtf", TESTDATA("data_out/tumor_only_report.rtf"));
	}


	//Test for debugging (without initialization because of speed)
	/*
	void debug()
	{
		QString host = Settings::string("ngsd_test_host", true);
		if (host=="") SKIP("Test needs access to the NGSD test database!");
		NGSD db(true);

		//getProcessingSystem
		QString sys = db.getProcessingSystem("tumor_cnvs._03", NGSD::SHORT);
		S_EQUAL(sys, "hpHBOCv5");
	}
	*/
};
