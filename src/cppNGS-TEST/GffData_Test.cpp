#include "TestFramework.h"
#include "GffData.h"

TEST_CLASS(GffData_Test)
{
private:

	TEST_METHOD(loadGffFile_unzipped)
	{
		GffSettings settings;
		settings.print_to_stdout = false;

		//do not skip GENCODE basic
		settings.include_all = true;
        GffData gff = GffData::load(TESTDATA("data_in/NGSHelper_loadGffFile_in1.gff3"), settings);

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
        gff = GffData::load(TESTDATA("data_in/NGSHelper_loadGffFile_in1.gff3"), settings);

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
        GffData gff = GffData::load(TESTDATA("data_in/NGSHelper_loadGffFile_in2.gff3.gz"), settings);

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
        GffData gff = GffData::load(TESTDATA("data_in/NGSHelper_loadGffFile_in3.gff3.gz"), settings);

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
        GffData gff = GffData::load(TESTDATA("data_in/NGSHelper_loadGffFile_in3.gff3.gz"), settings);

		I_EQUAL(gff.transcripts.count(), 13);
		I_EQUAL(gff.transcripts.geneCount(), 2);
		I_EQUAL(gff.transcripts.transcriptCount("BRCA2"), 6);
		I_EQUAL(gff.transcripts.transcriptCount("RFC1"), 7);

		IS_TRUE(gff.transcripts.contains("NM_001204747"));
		IS_TRUE(gff.transcripts.contains("XR_007057951")); //predicted by Gnomon
	}

};
