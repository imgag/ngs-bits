#include "TestFramework.h"
#include "Transcript.h"

TEST_CLASS(Transcript_Test)
{
Q_OBJECT

	Transcript trans_SLC51A()
	{
		Transcript t;

		t.setName("ENST00000296327");
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

		t.setName("ENST00000343267");
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

	void hgvsToVariant()
	{
		//plus strand (SLC51A)
		Transcript t = trans_APOD();
		Variant variant = t.hgvsToVariant("c.245+45T>C");
		S_EQUAL(variant.chr().str(), "chr3");
		I_EQUAL(variant.start(), 195300676);
		I_EQUAL(variant.end(), 195300676);
		S_EQUAL(variant.ref(), "A");
		S_EQUAL(variant.obs(), "G");

		//TODO chr3	195295702	195295702	-	GGC	hom	off-target	QUAL=414;DP=19;AF=0.95;MQM=60	APOD	3'UTR	APOD:ENST00000343267:3_prime_UTR_variant:MODIFIER:exon5/5:c.*68_*69insGCC::	regulatory_region_variant:promoter_flanking_region					rs146445395										3.78						596	824	50 / 89					APOD (inh=n/a oe_syn=0.96 oe_mis=0.95 oe_lof=0.68)
		//TODO chr3	195295708	195295708	-	T	hom	off-target	QUAL=414;DP=19;AF=0.95;MQM=60	APOD	3'UTR	APOD:ENST00000343267:3_prime_UTR_variant:MODIFIER:exon5/5:c.*62_*63insA::	regulatory_region_variant:promoter_flanking_region					rs142189206	0.2240									1.21						n/a (AF>5%)	n/a (AF>5%)	n/a (AF>5%)					APOD (inh=n/a oe_syn=0.96 oe_mis=0.95 oe_lof=0.68)

	}


};
