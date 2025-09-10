#include "TestFramework.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"
#include "Settings.h"
#include <iostream>

TEST_CLASS(NGSHelper_Test)
{
private:

	TEST_METHOD(getKnownVariants)
	{
		VcfFile list = NGSHelper::getKnownVariants(GenomeBuild::HG19, false);
		I_EQUAL(list.count(), 102467);

		//only SNPs
		list = NGSHelper::getKnownVariants(GenomeBuild::HG19, true);
		I_EQUAL(list.count(), 97469);

		//only SNPs, AF<80%
		list = NGSHelper::getKnownVariants(GenomeBuild::HG19, true, 0.0, 0.8);
		I_EQUAL(list.count(), 91186);

		//only SNPs, AF>20%
		list = NGSHelper::getKnownVariants(GenomeBuild::HG19, true, 0.2);
		I_EQUAL(list.count(), 36022);

		//only SNPs, AF>20%, AF<80%
		list = NGSHelper::getKnownVariants(GenomeBuild::HG19, true, 0.2, 0.8);
		I_EQUAL(list.count(), 29739);

		//only SNPs on chrX
		BedFile roi_chrx("chrX", 1, 155270560);
		list = NGSHelper::getKnownVariants(GenomeBuild::HG19, true, roi_chrx, 0.0, 1.0);
		I_EQUAL(list.count(), 1948);
	}

	TEST_METHOD(getKnownVariants_hg38)
	{
		VcfFile list = NGSHelper::getKnownVariants(GenomeBuild::HG38, false);
		I_EQUAL(list.count(), 100779);

		//only SNPs, AF<50% on chrX
		BedFile roi_chrx("chrX", 1, 155270560);
		list = NGSHelper::getKnownVariants(GenomeBuild::HG38, true, roi_chrx, 0.0, 0.5);
		I_EQUAL(list.count(), 1548);
	}

	TEST_METHOD(softClipAlignment)
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al;

		//first soft-clip al
		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}

		NGSHelper::softClipAlignment(al,146992, 147014);
		S_EQUAL(al.cigarDataAsString(), "23S128M");

		//second soft-clip same al
		NGSHelper::softClipAlignment(al,147015,147139);
		S_EQUAL(al.cigarDataAsString(), "148S3M");

		//next alignment
		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}
		//third soft-clip different al1
		NGSHelper::softClipAlignment(al,147234,147269);
		NGSHelper::softClipAlignment(al,147378,147384);
		S_EQUAL(al.cigarDataAsString(), "36S108M7S");
	}

	TEST_METHOD(softClipAlignment2)
	{
		BamReader reader(TESTDATA("data_in/bamclipoverlap.bam"));
		BamAlignment al;

		//first soft-clip al
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}

		S_EQUAL(al.name(), "PC0226:55:000000000-A5CV9:1:1101:2110:14905");
		NGSHelper::softClipAlignment(al,33038615,33038624);
		S_EQUAL(al.cigarDataAsString(), "5H10S141M");
		I_EQUAL(al.start(), 33038625);

		NGSHelper::softClipAlignment(al,33038756,33038765);
		S_EQUAL(al.cigarDataAsString(), "5H10S131M10S");

		reader.getNextAlignment(al);
		reader.getNextAlignment(al);

		S_EQUAL(al.name(), "PC0226:55:000000000-A5CV9:1:1101:2110:14905");
		NGSHelper::softClipAlignment(al,33038659,33038668);
		S_EQUAL(al.cigarDataAsString(), "10S141M5H");
		I_EQUAL(al.start(), 33038669);

		NGSHelper::softClipAlignment(al,33038800,33038809);
		S_EQUAL(al.cigarDataAsString(), "10S131M10S5H");
	}

	TEST_METHOD(pseudoAutosomalRegion)
	{
		BedFile par = NGSHelper::pseudoAutosomalRegion(GenomeBuild::HG19);
		I_EQUAL(par.count(), 4);
		I_EQUAL(par.baseCount(), 5938074);
	}

	TEST_METHOD(cytoBand)
	{
		S_EQUAL(NGSHelper::cytoBand(GenomeBuild::HG19, "chrY", 34847524), "Yq12");
		S_EQUAL(NGSHelper::cytoBand(GenomeBuild::HG19, "chr1", 76992611), "1p31.1");
	}

	TEST_METHOD(cytoBandToRange)
	{
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(GenomeBuild::HG19, ""));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(GenomeBuild::HG19, "Zr36.33"));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1r36.33"));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1p36.33-"));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1p36.33-5q21.2"));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1p36.33-1p36.32-1p36.31"));

		S_EQUAL(NGSHelper::cytoBandToRange(GenomeBuild::HG19, "chr1p36.33").toString(true), "chr1:1-2300000");
		S_EQUAL(NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1p36.33").toString(true), "chr1:1-2300000");
		S_EQUAL(NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1p36.33-1p36.32").toString(true), "chr1:1-5400000");
		S_EQUAL(NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1p36.32-1p36.33").toString(true), "chr1:1-5400000");
	}

	TEST_METHOD(impringGenes)
	{
		QMap<QByteArray, ImprintingInfo> imp_genes = NGSHelper::imprintingGenes();

		I_EQUAL(imp_genes.count(), 247);
		S_EQUAL(imp_genes["NPAP1"].expressed_allele, "paternal");
		S_EQUAL(imp_genes["NPAP1"].status, "imprinted");
		S_EQUAL(imp_genes["NTM"].expressed_allele, "maternal");
		S_EQUAL(imp_genes["NTM"].status, "imprinted");
		S_EQUAL(imp_genes["SALL1"].expressed_allele, "maternal");
		S_EQUAL(imp_genes["SALL1"].status, "predicted");
	}

	TEST_METHOD(centromeres)
	{
		BedFile centros = NGSHelper::centromeres(GenomeBuild::HG19);
		I_EQUAL(centros.count(), 24);
		S_EQUAL(centros[1].toString(true), "chr2:92326171-95326171");
		S_EQUAL(centros[11].toString(true), "chr12:34856694-37856694");

		BedFile centros4 = NGSHelper::centromeres(GenomeBuild::HG38);
		I_EQUAL(centros4.count(), 24);
		S_EQUAL(centros4[0].toString(true), "chr1:121700000-125100000");
	}

	TEST_METHOD(telomeres)
	{
		BedFile telos1 = NGSHelper::telomeres(GenomeBuild::HG19);
		I_EQUAL(telos1.count(), 46);
		S_EQUAL(telos1[45].toString(true), "chrY:59363566-59373566");

		BedFile telos2 = NGSHelper::telomeres(GenomeBuild::HG38);
		I_EQUAL(telos2.count(), 48);
		S_EQUAL(telos2[32].toString(true), "chr17:1-10000");
		S_EQUAL(telos2[45].toString(true), "chrX:156030895-156040895");
	}

	TEST_METHOD(populationCodeToHumanReadable)
	{
		S_EQUAL(NGSHelper::populationCodeToHumanReadable(""), "");
		S_EQUAL(NGSHelper::populationCodeToHumanReadable("EUR"), "European");
		S_EQUAL(NGSHelper::populationCodeToHumanReadable("AFR"), "African");
		S_EQUAL(NGSHelper::populationCodeToHumanReadable("SAS"), "South asian");
		S_EQUAL(NGSHelper::populationCodeToHumanReadable("EAS"), "East asian");
		S_EQUAL(NGSHelper::populationCodeToHumanReadable("ADMIXED/UNKNOWN"), "Admixed/Unknown");
	}

	TEST_METHOD(transcriptMatches)
	{
		//HG19
		auto matches = NGSHelper::transcriptMatches(GenomeBuild::HG19);
		IS_FALSE(matches.contains("ENST00000644374"));
		IS_TRUE(matches.contains("ENST00000004921"));
		I_EQUAL(matches["ENST00000004921"].count(), 2);
		IS_TRUE(matches["ENST00000004921"].contains("CCDS11306"));
		IS_TRUE(matches["ENST00000004921"].contains("NM_002988"));
		IS_TRUE(matches["CCDS11306"].contains("ENST00000004921"));
		IS_TRUE(matches["NM_002988"].contains("ENST00000004921"));

		//HG38
		matches = NGSHelper::transcriptMatches(GenomeBuild::HG38);
		IS_TRUE(matches.contains("ENST00000644374"));
		IS_FALSE(matches.contains("ENST00000004921"));
		I_EQUAL(matches["ENST00000644374"].count(), 2);
		IS_TRUE(matches["ENST00000644374"].contains("NM_004447"));
		IS_TRUE(matches["ENST00000644374"].contains("CCDS31753"));
		IS_TRUE(matches["CCDS31753"].contains("ENST00000644374"));
		IS_TRUE(matches["NM_004447"].contains("ENST00000644374"));
	}

	TEST_METHOD(loadGffFile_unzipped)
	{
		GffSettings settings;
		settings.print_to_stdout = false;

		//do not skip GENCODE basic
		settings.include_all = true;
		GffData gff = NGSHelper::loadGffFile(TESTDATA("data_in/NGSHelper_loadGffFile_in1.gff3"), settings);

		I_EQUAL(gff.transcripts.count(), 21);
		IS_TRUE(gff.transcripts.contains("ENST00000578049")); //first valid
		IS_TRUE(gff.transcripts.contains("ENST00000643044")); //last valid
		IS_FALSE(gff.transcripts.contains("ENST00000613230")); //special chromosome > skipped
		IS_FALSE(gff.transcripts.contains("ENST00000671898")); //not name and no HGNC-ID > skipped

		gff.transcripts.sortByPosition(); //order is not defined because we use a hash while parsing

		S_EQUAL(gff.transcripts[0].name(), "ENST00000578049");
		I_EQUAL(gff.transcripts[0].version(), 4);
		S_EQUAL(gff.transcripts[0].nameCcds(), "CCDS83523.1");
		I_EQUAL(gff.transcripts[0].biotype(), Transcript::PROTEIN_CODING);
		S_EQUAL(gff.transcripts[0].gene(), "SEC22B");
		S_EQUAL(gff.transcripts[0].geneId(), "ENSG00000265808");
		S_EQUAL(gff.transcripts[0].hgncId(), "HGNC:10700");
		I_EQUAL(gff.transcripts[0].regions().count(), 5);
		I_EQUAL(gff.transcripts[0].regions().baseCount(), 6927);
		I_EQUAL(gff.transcripts[0].codingRegions().count(), 5);
		I_EQUAL(gff.transcripts[0].codingRegions().baseCount(), 648);
		IS_TRUE(gff.transcripts[0].isGencodeBasicTranscript());
		IS_TRUE(gff.transcripts[0].isEnsemblCanonicalTranscript());
		IS_TRUE(gff.transcripts[0].isManeSelectTranscript());
		IS_FALSE(gff.transcripts[0].isManePlusClinicalTranscript());

		S_EQUAL(gff.transcripts[1].name(), "ENST00000618538");
		IS_FALSE(gff.transcripts[1].isGencodeBasicTranscript());
		IS_FALSE(gff.transcripts[1].isEnsemblCanonicalTranscript());
		IS_FALSE(gff.transcripts[1].isManeSelectTranscript());
		IS_FALSE(gff.transcripts[1].isManePlusClinicalTranscript());

		S_EQUAL(gff.transcripts[2].name(), "ENST00000643391");
		IS_TRUE(gff.transcripts[2].isGencodeBasicTranscript());
		IS_FALSE(gff.transcripts[2].isEnsemblCanonicalTranscript());
		IS_FALSE(gff.transcripts[2].isManeSelectTranscript());
		IS_TRUE(gff.transcripts[2].isManePlusClinicalTranscript());

		//skip GENCODE basic
		settings.include_all = false;
		gff = NGSHelper::loadGffFile(TESTDATA("data_in/NGSHelper_loadGffFile_in1.gff3"), settings);

		I_EQUAL(gff.transcripts.count(), 11);
		IS_TRUE(gff.transcripts.contains("ENST00000578049")); //first valid
		IS_TRUE(gff.transcripts.contains("ENST00000643044")); //last valid
		IS_FALSE(gff.transcripts.contains("ENST00000613230")); //special chromosome > skipped
		IS_FALSE(gff.transcripts.contains("ENST00000671898")); //not name and no HGNC-ID > skipped
	}

	TEST_METHOD(loadGffFile_gzipped)
	{
		GffSettings settings;
		settings.print_to_stdout = false;

		//do not skip GENCODE basic
		settings.include_all = true;
		GffData gff = NGSHelper::loadGffFile(TESTDATA("data_in/NGSHelper_loadGffFile_in2.gff3.gz"), settings);

		I_EQUAL(gff.transcripts.count(), 21);
		IS_TRUE(gff.transcripts.contains("ENST00000578049")); //first valid
		IS_TRUE(gff.transcripts.contains("ENST00000643044")); //last valid
	}

	TEST_METHOD(loadGffFile_refseq)
	{
		GffSettings settings;
		settings.source = "refseq";
		settings.print_to_stdout = false;
		settings.include_all = false;
		GffData gff = NGSHelper::loadGffFile(TESTDATA("data_in/NGSHelper_loadGffFile_in3.gff3.gz"), settings);

		I_EQUAL(gff.transcripts.count(), 10);
		I_EQUAL(gff.transcripts.geneCount(), 2);
		I_EQUAL(gff.transcripts.transcriptCount("BRCA2"), 6);
		I_EQUAL(gff.transcripts.transcriptCount("RFC1"), 4);
		IS_TRUE(gff.transcripts.contains("NM_001204747"));
		IS_FALSE(gff.transcripts.contains("XR_007057951")); //predicted by Gnomon

		Transcript trans = gff.transcripts.getTranscript("NM_001204747");
		IS_TRUE(trans.isValid());
		S_EQUAL(trans.gene(), "RFC1");
		S_EQUAL(trans.name(), "NM_001204747");
		I_EQUAL(trans.version(), 2);
		S_EQUAL(trans.geneId(), "gene-RFC1");
		S_EQUAL(trans.hgncId(), "HGNC:9969");
		S_EQUAL(trans.nameCcds(), "");
		I_EQUAL(trans.source(), Transcript::ENSEMBL);
		I_EQUAL(trans.strand(), Transcript::MINUS);
		I_EQUAL(trans.biotype(), Transcript::PROTEIN_CODING);
		S_EQUAL(trans.chr().str(), "chr4");
		I_EQUAL(trans.start(), 39287456);
		I_EQUAL(trans.end(), 39366362);
		IS_FALSE(trans.isPreferredTranscript());
		IS_FALSE(trans.isGencodeBasicTranscript());
		IS_FALSE(trans.isEnsemblCanonicalTranscript());
		IS_FALSE(trans.isManeSelectTranscript());
		IS_FALSE(trans.isManePlusClinicalTranscript());
		IS_TRUE(trans.isCoding());
		I_EQUAL(trans.regions().count(), 25);
		I_EQUAL(trans.regions().baseCount(), 4873);
		I_EQUAL(trans.codingRegions().count(), 25);
		I_EQUAL(trans.codingRegions().baseCount(), 3447);
		I_EQUAL(trans.codingStart(), 39366241);
		I_EQUAL(trans.codingEnd(), 39288761);
		I_EQUAL(trans.utr3prime().count(), 1);
		I_EQUAL(trans.utr3prime().baseCount(), 1305);
		I_EQUAL(trans.utr5prime().count(), 1);
		I_EQUAL(trans.utr5prime().baseCount(), 121);
	}

	TEST_METHOD(loadGffFile_refseq_all)
	{
		GffSettings settings;
		settings.source = "refseq";
		settings.print_to_stdout = false;
		settings.include_all = true;
		GffData gff = NGSHelper::loadGffFile(TESTDATA("data_in/NGSHelper_loadGffFile_in3.gff3.gz"), settings);

		I_EQUAL(gff.transcripts.count(), 13);
		I_EQUAL(gff.transcripts.geneCount(), 2);
		I_EQUAL(gff.transcripts.transcriptCount("BRCA2"), 6);
		I_EQUAL(gff.transcripts.transcriptCount("RFC1"), 7);

		IS_TRUE(gff.transcripts.contains("NM_001204747"));
		IS_TRUE(gff.transcripts.contains("XR_007057951")); //predicted by Gnomon
	}

	TEST_METHOD(maxEntScanImpact)
	{
		QByteArrayList score_pairs;
		MaxEntScanImpact impact;
		QByteArray score_pairs_with_impact;

		//only native splice site - no effect
		score_pairs = QByteArrayList() << "";
		impact = NGSHelper::maxEntScanImpact(score_pairs, score_pairs_with_impact, false);
		I_EQUAL(impact, MaxEntScanImpact::LOW);
		S_EQUAL(score_pairs_with_impact, "-");

		//only native splice site - no effect
		score_pairs = QByteArrayList() << "9.5>8.5";
		impact = NGSHelper::maxEntScanImpact(score_pairs, score_pairs_with_impact, false);
		I_EQUAL(impact, MaxEntScanImpact::LOW);
		S_EQUAL(score_pairs_with_impact, "9.5>8.5");

		//only native splice site - moderate effect
		score_pairs = QByteArrayList() << "9.5>8.2";
		impact = NGSHelper::maxEntScanImpact(score_pairs, score_pairs_with_impact, false);
		I_EQUAL(impact, MaxEntScanImpact::MODERATE);
		S_EQUAL(score_pairs_with_impact, "9.5>8.2(MODERATE)");

		//only native splice site - moderate effect
		score_pairs = QByteArrayList() << "7.1>6.1";
		impact = NGSHelper::maxEntScanImpact(score_pairs, score_pairs_with_impact, false);
		I_EQUAL(impact, MaxEntScanImpact::MODERATE);
		S_EQUAL(score_pairs_with_impact, "7.1>6.1(MODERATE)");

		//only native splice site - high effect
		score_pairs = QByteArrayList() << "8.5>6.1";
		impact = NGSHelper::maxEntScanImpact(score_pairs, score_pairs_with_impact, false);
		I_EQUAL(impact, MaxEntScanImpact::HIGH);
		S_EQUAL(score_pairs_with_impact, "8.5>6.1(HIGH)");

		//intronic prediction - native splice site missing - no effect
		score_pairs = QByteArrayList() << "" << "-3.4>4.5" << "2.7>3.3";
		impact = NGSHelper::maxEntScanImpact(score_pairs, score_pairs_with_impact, false);
		I_EQUAL(impact, MaxEntScanImpact::LOW);
		S_EQUAL(score_pairs_with_impact, "- / -3.4>4.5 / 2.7>3.3");

		//intronic prediction - moderate effect
		score_pairs = QByteArrayList() << "9.5>8.5" << "-3.4>6.5" << "2.7>6.7";
		impact = NGSHelper::maxEntScanImpact(score_pairs, score_pairs_with_impact, false);
		I_EQUAL(impact, MaxEntScanImpact::MODERATE);
		S_EQUAL(score_pairs_with_impact, "9.5>8.5 / -3.4>6.5(MODERATE) / 2.7>6.7(MODERATE)");

		//intronic prediction - high effect
		score_pairs = QByteArrayList() << "9.5>8.5" << "-3.4>8.7" << "2.7>8.6";
		impact = NGSHelper::maxEntScanImpact(score_pairs, score_pairs_with_impact, false);
		I_EQUAL(impact, MaxEntScanImpact::HIGH);
		S_EQUAL(score_pairs_with_impact, "9.5>8.5 / -3.4>8.7(HIGH) / 2.7>8.6(HIGH)");
	}

	TEST_METHOD(maxSpliceAiScore)
	{
		//old format
		I_EQUAL(NGSHelper::maxSpliceAiScore(""), -1.0);
		I_EQUAL(NGSHelper::maxSpliceAiScore("0.55"), 0.55);

		//new format
		I_EQUAL(NGSHelper::maxSpliceAiScore("BABAM1|0.03|0.00|0.01|0.00|-2|2|41|2"), 0.03);
		I_EQUAL(NGSHelper::maxSpliceAiScore("BABAM1|0.88|0.00|0.01|0.00|-2|2|41|2,CTD-2278I10.6|0.99|0.00|0.01|0.00|-2|2|41|2"), 0.99);
		I_EQUAL(NGSHelper::maxSpliceAiScore("BABAM1|0.88|0.00|0.01|0.00|-2|2|41|2,CTD-2278I10.6|0.77|0.00|0.01|0.00|-2|2|41|2"), 0.88);
		I_EQUAL(NGSHelper::maxSpliceAiScore("BABAM1|.|.|.|.|-2|2|41|2,CTD-2278I10.6|.|.|.|.|-2|2|41|2"), -1.0);
	}
};
