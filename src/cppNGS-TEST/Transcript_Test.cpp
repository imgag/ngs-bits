#include "TestFramework.h"
#include "Transcript.h"
#include "Settings.h"
#include "ChromosomalIndex.h"
#include <iostream>

TEST_CLASS(Transcript_Test)
{

	Transcript trans_SLC51A()
	{
		Transcript t;

		t.setGene("SLC51A");
		t.setName("ENST00000296327"); //NM_152672
		t.setSource(Transcript::ENSEMBL);
		t.setStrand(Transcript::PLUS);

		BedFile regions;
		regions.append(BedLine("chr3", 196216534, 196216750));
		regions.append(BedLine("chr3", 196217842, 196217936));
		regions.append(BedLine("chr3", 196226965, 196227119));
		regions.append(BedLine("chr3", 196227664, 196227737));
		regions.append(BedLine("chr3", 196228115, 196228273));
		regions.append(BedLine("chr3", 196228809, 196228920));
		regions.append(BedLine("chr3", 196229915, 196230061));
		regions.append(BedLine("chr3", 196232419, 196232524));
		regions.append(BedLine("chr3", 196233063, 196233427));
		t.setRegions(regions, 196216713, 196233199);

		return t;
	}

	Transcript trans_APOD()
	{
		Transcript t;

		t.setGene("APOD");
		t.setName("ENST00000343267"); //NM_001647
		t.setSource(Transcript::ENSEMBL);
		t.setStrand(Transcript::MINUS);

		BedFile regions;
		regions.append(BedLine("chr3", 195568705, 195569135));
		regions.append(BedLine("chr3", 195571277, 195571365));
		regions.append(BedLine("chr3", 195573850, 195573971));
		regions.append(BedLine("chr3", 195579339, 195579495));
		regions.append(BedLine("chr3", 195583878, 195583940));
		t.setRegions(regions, 195579461, 195568900);

		return t;
	}

private:

	TEST_METHOD(setRegions)
	{
		//plus strand (SLC51A)
		Transcript t = trans_SLC51A();
		S_EQUAL(t.gene(), "SLC51A");
		S_EQUAL(t.name(), "ENST00000296327");
		I_EQUAL(t.strand(), Transcript::PLUS);
		I_EQUAL(t.source(), Transcript::ENSEMBL);
		I_EQUAL(t.chr().num(), 3);
		I_EQUAL(t.start(), 196216534);
		I_EQUAL(t.end(), 196233427);
		I_EQUAL(t.regions().count(), 9);
		I_EQUAL(t.regions().baseCount(), 1430);
		I_EQUAL(t.codingRegions().count(), 9);
		I_EQUAL(t.codingRegions().baseCount(), 1023);
		I_EQUAL(t.utr3prime().count(), 1);
		I_EQUAL(t.utr3prime().baseCount(), 228);
		I_EQUAL(t.utr5prime().count(), 1);
		I_EQUAL(t.utr5prime().baseCount(), 179);

		//minus strand (APOD)
		t = trans_APOD();
		S_EQUAL(t.gene(), "APOD");
		S_EQUAL(t.name(), "ENST00000343267");
		I_EQUAL(t.strand(), Transcript::MINUS);
		I_EQUAL(t.source(), Transcript::ENSEMBL);
		I_EQUAL(t.chr().num(), 3);
		I_EQUAL(t.start(), 195568705);
		I_EQUAL(t.end(), 195583940);
		I_EQUAL(t.regions().count(), 5);
		I_EQUAL(t.regions().baseCount(), 862);
		I_EQUAL(t.codingRegions().count(), 4);
		I_EQUAL(t.codingRegions().baseCount(), 570);
		I_EQUAL(t.utr3prime().count(), 1);
		I_EQUAL(t.utr3prime().baseCount(), 195);
		I_EQUAL(t.utr5prime().count(), 2);
		I_EQUAL(t.utr5prime().baseCount(), 97);
		I_EQUAL(t.utr5prime()[0].length(), 34);
		I_EQUAL(t.utr5prime()[1].length(), 63);
	}

	TEST_METHOD(cDnaToGenomic)
	{
		//plus strand (SLC51A)
		Transcript t = trans_SLC51A();
		IS_THROWN(ArgumentException, t.cDnaToGenomic(0));
		I_EQUAL(t.cDnaToGenomic(1), 196216713); //exon 1, start codon
		I_EQUAL(t.cDnaToGenomic(4), 196216716); //exon 1, first coding base
		I_EQUAL(t.cDnaToGenomic(38), 196216750); //exon 1, last base
		I_EQUAL(t.cDnaToGenomic(39), 196217842); //exon 2, first base
		I_EQUAL(t.cDnaToGenomic(1020), 196233196); //exon 9, last coding base
		I_EQUAL(t.cDnaToGenomic(1021), 196233197); //exon 9, stop codon base 1
		I_EQUAL(t.cDnaToGenomic(1023), 196233199); //exon 9, stop codon base 3
		IS_THROWN(ArgumentException, t.cDnaToGenomic(1204));

		//minus strand (APOD)
		t = trans_APOD();
		IS_THROWN(ArgumentException, t.cDnaToGenomic(0));
		I_EQUAL(t.cDnaToGenomic(1), 195579461); //exon 1, start codon
		I_EQUAL(t.cDnaToGenomic(4), 195579458); //exon 1, first coding base
		I_EQUAL(t.cDnaToGenomic(123), 195579339); //exon 1, last base
		I_EQUAL(t.cDnaToGenomic(124), 195573971); //exon 2, first base
		I_EQUAL(t.cDnaToGenomic(567), 195568903); //exon 5, last coding base
		I_EQUAL(t.cDnaToGenomic(568), 195568902); //exon 5, stop codon base 1
		I_EQUAL(t.cDnaToGenomic(570), 195568900); //exon 5, stop codon base 2
		IS_THROWN(ArgumentException, t.cDnaToGenomic(571));
	}

	TEST_METHOD(hgvsToVariant_plus_strand)
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		Transcript t = trans_SLC51A();

		Variant variant = t.hgvsToVariant("c.123A>G", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196217926);
		I_EQUAL(variant.end(), 196217926);
		S_EQUAL(variant.ref(), "A");
		S_EQUAL(variant.obs(), "G");

		variant = t.hgvsToVariant("c.38+46G>T", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196216796);
		I_EQUAL(variant.end(), 196216796);
		S_EQUAL(variant.ref(), "G");
		S_EQUAL(variant.obs(), "T");



		variant = t.hgvsToVariant("c.134-43C>T", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196226922);
		I_EQUAL(variant.end(), 196226922);
		S_EQUAL(variant.ref(), "C");
		S_EQUAL(variant.obs(), "T");


		variant = t.hgvsToVariant("c.-207A>G", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196216506);
		I_EQUAL(variant.end(), 196216506);
		S_EQUAL(variant.ref(), "A");
		S_EQUAL(variant.obs(), "G");

		variant = t.hgvsToVariant("c.*48A>C", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196233247);
		I_EQUAL(variant.end(), 196233247);
		S_EQUAL(variant.ref(), "A");
		S_EQUAL(variant.obs(), "C");

		variant = t.hgvsToVariant("c.39-286dup", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196217547);
		I_EQUAL(variant.end(), 196217547);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "A");

		variant = t.hgvsToVariant("c.39-286dupA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196217547);
		I_EQUAL(variant.end(), 196217547);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "A");

		variant = t.hgvsToVariant("c.289-102_289-100dup", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196227560);
		I_EQUAL(variant.end(), 196227560);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "CCT");

		variant = t.hgvsToVariant("c.289-102_289-100dupCCT", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196227560);
		I_EQUAL(variant.end(), 196227560);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "CCT");

		variant = t.hgvsToVariant("c.134-3651del", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196223314);
		I_EQUAL(variant.end(), 196223314);
		S_EQUAL(variant.ref(), "A");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.134-3651delA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196223314);
		I_EQUAL(variant.end(), 196223314);
		S_EQUAL(variant.ref(), "A");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.134-1926_134-1925del", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196225026);
		I_EQUAL(variant.end(), 196225027);
		S_EQUAL(variant.ref(), "TT");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.134-1926_134-1925delTT", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196225026);
		I_EQUAL(variant.end(), 196225027);
		S_EQUAL(variant.ref(), "TT");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.134-1926_134-1925del2", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196225026);
		I_EQUAL(variant.end(), 196225027);
		S_EQUAL(variant.ref(), "TT");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.-103_-102insG", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196216610);
		I_EQUAL(variant.end(), 196216610);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "G");

		variant = t.hgvsToVariant("c.133+2481_133+2482insTTTCACTGAAAATACAATCTCTACA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196220417);
		I_EQUAL(variant.end(), 196220417);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "TTTCACTGAAAATACAATCTCTACA");

		variant = t.hgvsToVariant("c.-103_-102insG", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196216610);
		I_EQUAL(variant.end(), 196216610);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "G");

		variant = t.hgvsToVariant("c.798delinsAA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196232436);
		I_EQUAL(variant.end(), 196232436);
		S_EQUAL(variant.ref(), "T");
		S_EQUAL(variant.obs(), "AA");

		variant = t.hgvsToVariant("c.787_789delinsAAAA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196232425);
		I_EQUAL(variant.end(), 196232427);
		S_EQUAL(variant.ref(), "CTC");
		S_EQUAL(variant.obs(), "AAAA");

		variant = t.hgvsToVariant("c.787_789delinsAAAA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 196232425);
		I_EQUAL(variant.end(), 196232427);
		S_EQUAL(variant.ref(), "CTC");
		S_EQUAL(variant.obs(), "AAAA");
	}


	TEST_METHOD(hgvsToVariant_minus_strand)
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		Transcript t = trans_APOD();

		Variant variant = t.hgvsToVariant("c.*179C>A", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195568721);
		I_EQUAL(variant.end(), 195568721);
		S_EQUAL(variant.ref(), "G");
		S_EQUAL(variant.obs(), "T");

		variant = t.hgvsToVariant("c.335-553G>C", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195569688);
		I_EQUAL(variant.end(), 195569688);
		S_EQUAL(variant.ref(), "C");
		S_EQUAL(variant.obs(), "G");

		variant = t.hgvsToVariant("c.334+158G>A", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195571119);
		I_EQUAL(variant.end(), 195571119);
		S_EQUAL(variant.ref(), "C");
		S_EQUAL(variant.obs(), "T");

		variant = t.hgvsToVariant("c.-34-2A>G", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195579497);
		I_EQUAL(variant.end(), 195579497);
		S_EQUAL(variant.ref(), "T");
		S_EQUAL(variant.obs(), "C");

		variant = t.hgvsToVariant("c.178C>T", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195573917);
		I_EQUAL(variant.end(), 195573917);
		S_EQUAL(variant.ref(), "G");
		S_EQUAL(variant.obs(), "A");

		variant = t.hgvsToVariant("c.246-13dup", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195571377);
		I_EQUAL(variant.end(), 195571377);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "A");

		variant = t.hgvsToVariant("c.*65_*69dup", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195568830);
		I_EQUAL(variant.end(), 195568830);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "TGGGG");

		variant = t.hgvsToVariant("c.*68del", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195568832);
		I_EQUAL(variant.end(), 195568832);
		S_EQUAL(variant.ref(), "G");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.335-239_335-238del", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195569373);
		I_EQUAL(variant.end(), 195569374);
		S_EQUAL(variant.ref(), "AG");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.335-239_335-238delAG", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195569373);
		I_EQUAL(variant.end(), 195569374);
		S_EQUAL(variant.ref(), "AG");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.335-239_335-238del2", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195569373);
		I_EQUAL(variant.end(), 195569374);
		S_EQUAL(variant.ref(), "AG");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.*62_*63insA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195568837);
		I_EQUAL(variant.end(), 195568837);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "T");

		variant = t.hgvsToVariant("c.-34-875_-34-874insGAA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195580369);
		I_EQUAL(variant.end(), 195580369);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "TTC");

		variant = t.hgvsToVariant("c.*69delinsCCCCCC", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195568831);
		I_EQUAL(variant.end(), 195568831);
		S_EQUAL(variant.ref(), "T");
		S_EQUAL(variant.obs(), "GGGGGG");

		variant = t.hgvsToVariant("c.-4_-7delinsAA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195579465);
		I_EQUAL(variant.end(), 195579468);
		S_EQUAL(variant.ref(), "GGGG");
		S_EQUAL(variant.obs(), "TT");

		variant = t.hgvsToVariant("c.123+2_123+6delinsC", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195579333);
		I_EQUAL(variant.end(), 195579337);
		S_EQUAL(variant.ref(), "TTTTA");
		S_EQUAL(variant.obs(), "G");

		variant = t.hgvsToVariant("c.-34delinsACACACA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.end(), 195579495);
		I_EQUAL(variant.start(), 195579495);
		S_EQUAL(variant.ref(), "G");
		S_EQUAL(variant.obs(), "TGTGTGT");

		variant = t.hgvsToVariant("c.-35delinsACACACA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.end(), 195583878);
		I_EQUAL(variant.start(), 195583878);
		S_EQUAL(variant.ref(), "C");
		S_EQUAL(variant.obs(), "TGTGTGT");

		variant = t.hgvsToVariant("c.-35delGinsACACACA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.end(), 195583878);
		I_EQUAL(variant.start(), 195583878);
		S_EQUAL(variant.ref(), "C");
		S_EQUAL(variant.obs(), "TGTGTGT");
	}

	TEST_METHOD(exonNumber)
	{
		Transcript trans = trans_SLC51A();
		I_EQUAL(trans.exonNumber(196216531, 196216533), -1);
		I_EQUAL(trans.exonNumber(196216534, 196216534), 1);
		I_EQUAL(trans.exonNumber(196216750, 196216750), 1);
		I_EQUAL(trans.exonNumber(196217842, 196217936), 2);
		I_EQUAL(trans.exonNumber(196233063, 196233427), 9);
		I_EQUAL(trans.exonNumber(196233428, 196233430), -1);
		I_EQUAL(trans.exonNumber(196216534, 196217936), -2);

		Transcript trans2 = trans_APOD();
		I_EQUAL(trans2.exonNumber(195568701, 195568704), -1);
		I_EQUAL(trans2.exonNumber(195568705, 195569135), 5);
		I_EQUAL(trans2.exonNumber(195583878, 195583940), 1);
		I_EQUAL(trans2.exonNumber(195583941, 195583944), -1);
		I_EQUAL(trans2.exonNumber(195568705, 195583878), -2);
	}

	TEST_METHOD(check_ChromosomalIndex_works)
	{
		TranscriptList trans_list;
		trans_list << trans_APOD() << trans_SLC51A();

		ChromosomalIndex<TranscriptList> index(trans_list);

		I_EQUAL(index.matchingIndex("chr3", 195568705, 195569135), 0);
		I_EQUAL(index.matchingIndex("chr3", 196216534, 196216750), 1);
	}
};
