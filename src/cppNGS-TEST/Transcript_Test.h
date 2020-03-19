#include "TestFramework.h"
#include "Transcript.h"
#include "Settings.h"

TEST_CLASS(Transcript_Test)
{
Q_OBJECT

	Transcript trans_SLC51A()
	{
		Transcript t;

		t.setName("ENST00000296327"); //NM_152672
		t.setSource(Transcript::ENSEMBL);
		t.setStrand(Transcript::PLUS);

		BedFile regions;
		regions.append(BedLine("chr3", 195943375, 195943621));
		regions.append(BedLine("chr3", 195944713, 195944807));
		regions.append(BedLine("chr3", 195953836, 195953990));
		regions.append(BedLine("chr3", 195954535, 195954608));
		regions.append(BedLine("chr3", 195954986, 195955144));
		regions.append(BedLine("chr3", 195955680, 195955791));
		regions.append(BedLine("chr3", 195956786, 195956932));
		regions.append(BedLine("chr3", 195959290, 195959395));
		regions.append(BedLine("chr3", 195959934, 195960301));
		t.setRegions(regions, 195943584, 195960070);

		return t;
	}

	Transcript trans_APOD()
	{
		Transcript t;

		t.setName("ENST00000343267"); //NM_001647
		t.setSource(Transcript::ENSEMBL);
		t.setStrand(Transcript::MINUS);

		BedFile regions;
		regions.append(BedLine("chr3", 195295573, 195296006));
		regions.append(BedLine("chr3", 195298148, 195298236));
		regions.append(BedLine("chr3", 195300721, 195300842));
		regions.append(BedLine("chr3", 195306210, 195306366));
		regions.append(BedLine("chr3", 195310749, 195311076));
		t.setRegions(regions, 195295771, 195306332);

		return t;
	}

private slots:

	void initialization()
	{
		//plus strand (SLC51A)
		Transcript t = trans_SLC51A();
		S_EQUAL(t.name(), "ENST00000296327");
		I_EQUAL(t.strand(), Transcript::PLUS);
		I_EQUAL(t.source(), Transcript::ENSEMBL);
		I_EQUAL(t.regions().count(), 9);
		I_EQUAL(t.regions().baseCount(), 1463);
		I_EQUAL(t.codingRegions().count(), 9);
		I_EQUAL(t.codingRegions().baseCount(), 1023);

		//minus strand (APOD)
		t = trans_APOD();
		I_EQUAL(t.regions().count(), 5);
		I_EQUAL(t.regions().baseCount(), 1130);
		I_EQUAL(t.codingRegions().count(), 4);
		I_EQUAL(t.codingRegions().baseCount(), 570);
	}

	void cDnaToGenomic()
	{
		//plus strand (SLC51A)
		Transcript t = trans_SLC51A();
		IS_THROWN(ArgumentException, t.cDnaToGenomic(0));
		I_EQUAL(t.cDnaToGenomic(1), 195943584); //exon 1, start codon
		I_EQUAL(t.cDnaToGenomic(4), 195943587); //exon 1, first coding base
		I_EQUAL(t.cDnaToGenomic(38), 195943621); //exon 1, last base
		I_EQUAL(t.cDnaToGenomic(39), 195944713); //exon 2, first base
		I_EQUAL(t.cDnaToGenomic(1020), 195960067); //exon 9, last coding base
		I_EQUAL(t.cDnaToGenomic(1021), 195960068); //exon 9, stop codon base 1
		I_EQUAL(t.cDnaToGenomic(1023), 195960070); //exon 9, stop codon base 3
		IS_THROWN(ArgumentException, t.cDnaToGenomic(1024));

		//minus strand (APOD)
		t = trans_APOD();
		IS_THROWN(ArgumentException, t.cDnaToGenomic(0));
		I_EQUAL(t.cDnaToGenomic(1), 195306332); //exon 1, start codon
		I_EQUAL(t.cDnaToGenomic(4), 195306329); //exon 1, first coding base
		I_EQUAL(t.cDnaToGenomic(123), 195306210); //exon 1, last base
		I_EQUAL(t.cDnaToGenomic(124), 195300842); //exon 2, first base
		I_EQUAL(t.cDnaToGenomic(567), 195295774); //exon 5, last coding base
		I_EQUAL(t.cDnaToGenomic(568), 195295773); //exon 5, stop codon base 1
		I_EQUAL(t.cDnaToGenomic(570), 195295771); //exon 5, stop codon base 2
		IS_THROWN(ArgumentException, t.cDnaToGenomic(571));
	}

	void hgvsToVariant_plus_strand()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		Transcript t = trans_SLC51A();

		Variant variant = t.hgvsToVariant("c.123A>G", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195944797);
		I_EQUAL(variant.end(), 195944797);
		S_EQUAL(variant.ref(), "A");
		S_EQUAL(variant.obs(), "G");

		variant = t.hgvsToVariant("c.38+46G>T", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195943667);
		I_EQUAL(variant.end(), 195943667);
		S_EQUAL(variant.ref(), "G");
		S_EQUAL(variant.obs(), "T");

		variant = t.hgvsToVariant("c.134-43C>T", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195953793);
		I_EQUAL(variant.end(), 195953793);
		S_EQUAL(variant.ref(), "C");
		S_EQUAL(variant.obs(), "T");

		variant = t.hgvsToVariant("c.-207A>G", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195943377);
		I_EQUAL(variant.end(), 195943377);
		S_EQUAL(variant.ref(), "A");
		S_EQUAL(variant.obs(), "G");

		variant = t.hgvsToVariant("c.*48A>C", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195960118);
		I_EQUAL(variant.end(), 195960118);
		S_EQUAL(variant.ref(), "A");
		S_EQUAL(variant.obs(), "C");

		variant = t.hgvsToVariant("c.39-286dup", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195944418);
		I_EQUAL(variant.end(), 195944418);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "A");

		variant = t.hgvsToVariant("c.289-102_289-100dup", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195954431);
		I_EQUAL(variant.end(), 195954431);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "CCT");

		variant = t.hgvsToVariant("c.134-3651del", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195950185);
		I_EQUAL(variant.end(), 195950185);
		S_EQUAL(variant.ref(), "A");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.134-1926_134-1925del", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195951897);
		I_EQUAL(variant.end(), 195951898);
		S_EQUAL(variant.ref(), "TT");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.-103_-102insG", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195943481);
		I_EQUAL(variant.end(), 195943481);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "G");

		variant = t.hgvsToVariant("c.133+2481_133+2482insTTTCACTGAAAATACAATCTCTACA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195947288);
		I_EQUAL(variant.end(), 195947288);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "TTTCACTGAAAATACAATCTCTACA");

		variant = t.hgvsToVariant("c.-103_-102insG", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195943481);
		I_EQUAL(variant.end(), 195943481);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "G");

		variant = t.hgvsToVariant("c.798delinsAA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195959307);
		I_EQUAL(variant.end(), 195959307);
		S_EQUAL(variant.ref(), "T");
		S_EQUAL(variant.obs(), "AA");

		variant = t.hgvsToVariant("c.787_789delinsAAAA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195959296);
		I_EQUAL(variant.end(), 195959298);
		S_EQUAL(variant.ref(), "CTC");
		S_EQUAL(variant.obs(), "AAAA");
	}


	void hgvsToVariant_minus_strand()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		Transcript t = trans_APOD();

		Variant variant = t.hgvsToVariant("c.*179C>A", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195295592);
		I_EQUAL(variant.end(), 195295592);
		S_EQUAL(variant.ref(), "G");
		S_EQUAL(variant.obs(), "T");

		variant = t.hgvsToVariant("c.335-553G>C", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195296559);
		I_EQUAL(variant.end(), 195296559);
		S_EQUAL(variant.ref(), "C");
		S_EQUAL(variant.obs(), "G");

		variant = t.hgvsToVariant("c.334+158G>A", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195297990);
		I_EQUAL(variant.end(), 195297990);
		S_EQUAL(variant.ref(), "C");
		S_EQUAL(variant.obs(), "T");

		variant = t.hgvsToVariant("c.-34-2A>G", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195306368);
		I_EQUAL(variant.end(), 195306368);
		S_EQUAL(variant.ref(), "T");
		S_EQUAL(variant.obs(), "C");

		variant = t.hgvsToVariant("c.178C>T", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195300788);
		I_EQUAL(variant.end(), 195300788);
		S_EQUAL(variant.ref(), "G");
		S_EQUAL(variant.obs(), "A");

		variant = t.hgvsToVariant("c.246-13dup", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195298248);
		I_EQUAL(variant.end(), 195298248);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "A");

		variant = t.hgvsToVariant("c.*65_*69dup", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195295701);
		I_EQUAL(variant.end(), 195295701);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "TGGGG");

		variant = t.hgvsToVariant("c.*68del", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195295703);
		I_EQUAL(variant.end(), 195295703);
		S_EQUAL(variant.ref(), "G");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.335-239_335-238del", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195296244);
		I_EQUAL(variant.end(), 195296245);
		S_EQUAL(variant.ref(), "AG");
		S_EQUAL(variant.obs(), "-");

		variant = t.hgvsToVariant("c.*62_*63insA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195295708);
		I_EQUAL(variant.end(), 195295708);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "T");

		variant = t.hgvsToVariant("c.-34-875_-34-874insGAA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195307240);
		I_EQUAL(variant.end(), 195307240);
		S_EQUAL(variant.ref(), "-");
		S_EQUAL(variant.obs(), "TTC");

		variant = t.hgvsToVariant("c.*69delinsCCCCCC", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195295702);
		I_EQUAL(variant.end(), 195295702);
		S_EQUAL(variant.ref(), "T");
		S_EQUAL(variant.obs(), "GGGGGG");

		variant = t.hgvsToVariant("c.-4_-7delinsAA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195306336);
		I_EQUAL(variant.end(), 195306339);
		S_EQUAL(variant.ref(), "GGGG");
		S_EQUAL(variant.obs(), "TT");

//TODO The case that an UTR is split in several regions is not correctly handled yet. Implement test for both UTRs and both strands > MARC
/*
		variant = t.hgvsToVariant("c.-318_-320delinsACACACA", reference);
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195311030);
		I_EQUAL(variant.end(), 195311032);
		S_EQUAL(variant.ref(), "CGC");
		S_EQUAL(variant.obs(), "ACACACA");
*/
	}

};
