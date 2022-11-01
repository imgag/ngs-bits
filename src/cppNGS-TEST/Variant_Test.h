#include "TestFramework.h"
#include "VariantList.h"
#include "Settings.h"

TEST_CLASS(Variant_Test)
{
Q_OBJECT
private slots:

	void constructor()
	{
		Variant variant("chr11", 5, 10, " r ", " o ");
		S_EQUAL(variant.chr().str(), "chr11");
		I_EQUAL(variant.start(), 5);
		I_EQUAL(variant.end(), 10);
		S_EQUAL(variant.ref(), "r");
		S_EQUAL(variant.obs(), "o");
	}

	void static_normalize()
	{
		Sequence ref = "A";
		Sequence obs = "AGG";
		int pos = 17;
		Variant::normalize(pos, ref, obs);
		S_EQUAL(ref, Sequence(""));
		S_EQUAL(obs, Sequence("GG"));
		I_EQUAL(pos, 18);

		ref = "ATG";
		obs = "AGGTG";
		pos = 17;
		Variant::normalize(pos, ref, obs);
		S_EQUAL(ref, Sequence(""));
		S_EQUAL(obs, Sequence("GG"));
		I_EQUAL(pos, 18);

		ref = "TT";
		obs = "";
		pos = 17;
		Variant::normalize(pos, ref, obs);
		S_EQUAL(ref, Sequence("TT"));
		S_EQUAL(obs, Sequence(""));
		I_EQUAL(pos, 17);

		ref = "TAT";
		obs = "TT";
		pos = 17;
		Variant::normalize(pos, ref, obs);
		S_EQUAL(ref, Sequence("A"));
		S_EQUAL(obs, Sequence(""));
		I_EQUAL(pos, 18);

		ref = "TCCAT";
		obs = "TCCT";
		pos = 17;
		Variant::normalize(pos, ref, obs);
		S_EQUAL(ref, Sequence("A"));
		S_EQUAL(obs, Sequence(""));
		I_EQUAL(pos, 20);

		ref = "TCCT";
		obs = "TCCT";
		pos = 17;
		Variant::normalize(pos, ref, obs);
		S_EQUAL(ref, Sequence("C"));
		S_EQUAL(obs, Sequence("C"));
		I_EQUAL(pos, 18);

		ref = "T";
		obs = "T";
		pos = 17;
		Variant::normalize(pos, ref, obs);
		S_EQUAL(ref, Sequence("T"));
		S_EQUAL(obs, Sequence("T"));
		I_EQUAL(pos, 17);
	}

	void static_minBlock()
	{
		S_EQUAL(Variant::minBlock("ACACAC"), Sequence("AC"));
		S_EQUAL(Variant::minBlock("ACAC"), Sequence("AC"));
		S_EQUAL(Variant::minBlock("AC"), Sequence("AC"));
		S_EQUAL(Variant::minBlock("AAA"), Sequence("A"));
		S_EQUAL(Variant::minBlock("CC"), Sequence("C"));
		S_EQUAL(Variant::minBlock("ACGTACGT"), Sequence("ACGT"));
		S_EQUAL(Variant::minBlock("ACGT"), Sequence("ACGT"));
	}

	void static_indelRegion()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		//insertion T left (TSV-style)
		QPair<int, int> pos = Variant::indelRegion("chr6", 110054493, 110054493, "-", "T", reference);
		I_EQUAL(pos.first, 110054494);
		I_EQUAL(pos.second, 110054499);

		//insertion T right (TSV-style)
		pos = Variant::indelRegion("chr6", 110054499, 110054499, "-", "T", reference);
		I_EQUAL(pos.first, 110054494);
		I_EQUAL(pos.second, 110054499);

		//insertion TT left (TSV-style)
		pos = Variant::indelRegion("chr6", 110054493, 110054493, "-", "TT", reference);
		I_EQUAL(pos.first, 110054494);
		I_EQUAL(pos.second, 110054499);

		//insertion TT right (TSV-style)
		pos = Variant::indelRegion("chr6", 110054499, 110054499, "-", "TT", reference);
		I_EQUAL(pos.first, 110054494);
		I_EQUAL(pos.second, 110054499);

		//deletion T left (TSV-style)
		pos = Variant::indelRegion("chr6", 110054494, 110054494, "T", "-", reference);
		I_EQUAL(pos.first, 110054494);
		I_EQUAL(pos.second, 110054499);

		//deletion T right (TSV-style)
		 pos = Variant::indelRegion("chr6", 110054499, 110054499, "T", "-", reference);
		I_EQUAL(pos.first, 110054494);
		I_EQUAL(pos.second, 110054499);

		//deletion TT left (TSV-style)
		pos = Variant::indelRegion("chr6", 110054494, 110054495, "TT", "-", reference);
		I_EQUAL(pos.first, 110054494);
		I_EQUAL(pos.second, 110054499);

		//deletion TT right (TSV-style)
		 pos = Variant::indelRegion("chr6", 110054498, 110054499, "TT", "-", reference);
		I_EQUAL(pos.first, 110054494);
		I_EQUAL(pos.second, 110054499);

		//deletion AG left (TSV-style)
		pos = Variant::indelRegion("chr14", 53513603, 53513604, "AG", "-", reference);
		I_EQUAL(pos.first, 53513603);
		I_EQUAL(pos.second, 53513606);

		//deletion AG right (TSV-style)
		pos = Variant::indelRegion("chr14", 53513605, 53513606, "AG", "-", reference);
		I_EQUAL(pos.first, 53513603);
		I_EQUAL(pos.second, 53513606);

		//insertion outside repeat => no region
		pos = Variant::indelRegion("chr14", 53207583, 53207583, "-", "CACCAAAGCCACCAGTGCCGAAACCAGCTCCGAAGCCGCCGG", reference);
		I_EQUAL(pos.first, 53207583);
		I_EQUAL(pos.second, 53207583);

		//deletion outside repeat => no region
		pos = Variant::indelRegion("chr14", 53207582, 53207584, "AAC", "-", reference);
		I_EQUAL(pos.first, 53207582);
		I_EQUAL(pos.second, 53207584);

		//complex indel => no region
		pos = Variant::indelRegion("chr14", 53513479, 53513480, "AG", "TT", reference);
		I_EQUAL(pos.first, 53513479);
		I_EQUAL(pos.second, 53513480);

		//SNV => no region
		pos = Variant::indelRegion("chr14", 53513479, 53513479, "A", "G", reference);
		I_EQUAL(pos.first, 53513479);
		I_EQUAL(pos.second, 53513479);
	}

	void overlapsWithComplete()
	{
		Variant variant("chr1", 5, 10, "r", "o");
		IS_TRUE(!variant.overlapsWith("chr2", 5, 10));
		IS_TRUE(!variant.overlapsWith("chr1", 1, 4));
		IS_TRUE(!variant.overlapsWith("chr1", 11, 20));
		IS_TRUE(variant.overlapsWith("chr1", 1, 5));
		IS_TRUE(variant.overlapsWith("chr1", 5, 10));
		IS_TRUE(variant.overlapsWith("chr1", 6, 8));
		IS_TRUE(variant.overlapsWith("chr1", 10, 20));
		IS_TRUE(variant.overlapsWith("chr1", 1, 20));
	}

	void overlapsWithPosition()
	{
		Variant variant("chr1", 5, 10, "r", "o");
		IS_TRUE(variant.overlapsWith(5, 10));
		IS_TRUE(!variant.overlapsWith(1, 4));
		IS_TRUE(!variant.overlapsWith(11, 20));
		IS_TRUE(variant.overlapsWith(1, 5));
		IS_TRUE(variant.overlapsWith(5, 10));
		IS_TRUE(variant.overlapsWith(6, 8));
		IS_TRUE(variant.overlapsWith(10, 20));
		IS_TRUE(variant.overlapsWith(1, 20));
	}


	void overlapsWithBedLine()
	{
		Variant variant("chr1", 5, 10, "r", "o");
		IS_TRUE(!variant.overlapsWith(BedLine("chr2", 5, 10)));
		IS_TRUE(!variant.overlapsWith(BedLine("chr1", 1, 4)));
		IS_TRUE(!variant.overlapsWith(BedLine("chr1", 11, 20)));
		IS_TRUE(variant.overlapsWith(BedLine("chr1", 1, 5)));
		IS_TRUE(variant.overlapsWith(BedLine("chr1", 5, 10)));
		IS_TRUE(variant.overlapsWith(BedLine("chr1", 6, 8)));
		IS_TRUE(variant.overlapsWith(BedLine("chr1", 10, 20)));
		IS_TRUE(variant.overlapsWith(BedLine("chr1", 1, 20)));
	}

	void operator_lessthan()
	{
		IS_FALSE(Variant("chr1", 1, 20, "r", "o") < Variant("chr1", 1, 20, "r", "o"));
		IS_TRUE(Variant("chr1", 1, 20, "r", "o") < Variant("chr1", 5, 20, "r", "o"));
		IS_FALSE(Variant("chr2", 1, 20, "r", "o") < Variant("chr1", 1, 20, "r", "o"));
		IS_TRUE(Variant("chr1", 1, 20, "r", "o") < Variant("chr2", 5, 20, "r", "o"));
	}

	void normalize()
	{
		//tests with empty empty sequence
		Variant v = Variant("chr1", 17, 17, "A", "AGG");
		v.normalize();
		S_EQUAL(v.ref(), Sequence(""));
		S_EQUAL(v.obs(), Sequence("GG"));
		I_EQUAL(v.start(), 18);
		I_EQUAL(v.end(), 18);

		v = Variant("chr1", 17, 17, "ATG", "AGGTG");
		v.normalize("");
		S_EQUAL(v.ref(), Sequence(""));
		S_EQUAL(v.obs(), Sequence("GG"));
		I_EQUAL(v.start(), 18);
		I_EQUAL(v.end(), 18);

		v = Variant("chr1", 17, 18, "TT", "");
		v.normalize("");
		S_EQUAL(v.ref(), Sequence("TT"));
		S_EQUAL(v.obs(), Sequence(""));
		I_EQUAL(v.start(), 17);
		I_EQUAL(v.end(), 18);

		v = Variant("chr1", 17, 19, "TAT", "TT");
		v.normalize("");
		S_EQUAL(v.ref(), Sequence("A"));
		S_EQUAL(v.obs(), Sequence(""));
		I_EQUAL(v.start(), 18);
		I_EQUAL(v.end(), 18);

		//tests with non-empty empty sequence
		v = Variant("chr1", 17, 17, "A", "AGG");
		v.normalize("-");
		S_EQUAL(v.ref(), Sequence("-"));
		S_EQUAL(v.obs(), Sequence("GG"));
		I_EQUAL(v.start(), 18);
		I_EQUAL(v.end(), 18);

		v = Variant("chr1", 17, 17, "ATG", "AGGTG");
		v.normalize("-");
		S_EQUAL(v.ref(), Sequence("-"));
		S_EQUAL(v.obs(), Sequence("GG"));
		I_EQUAL(v.start(), 18);
		I_EQUAL(v.end(), 18);
		v = Variant("chr1", 17, 17, "ATG", "AGGTG");
		v.normalize("-", true);
		S_EQUAL(v.ref(), Sequence("-"));
		S_EQUAL(v.obs(), Sequence("GG"));
		I_EQUAL(v.start(), 17);
		I_EQUAL(v.end(), 17);

		v = Variant("chr1", 17, 18, "TT", "");
		v.normalize("-");
		S_EQUAL(v.ref(), Sequence("TT"));
		S_EQUAL(v.obs(), Sequence("-"));
		I_EQUAL(v.start(), 17);
		I_EQUAL(v.end(), 18);

		v = Variant("chr1", 17, 19, "TAT", "TT");
		v.normalize("-");
		X_EQUAL(v.ref(), Sequence("A"));
		X_EQUAL(v.obs(), Sequence("-"));
		I_EQUAL(v.start(), 18);
		I_EQUAL(v.end(), 18);
		//
		v = Variant("chr1", 17, 19, "TAT", "TT");
		v.normalize("-", true);
		X_EQUAL(v.ref(), Sequence("A"));
		X_EQUAL(v.obs(), Sequence("-"));
		I_EQUAL(v.start(), 18);
		I_EQUAL(v.end(), 18);

		v = Variant("chr18", 65, 65, "A", "AA");
		v.normalize("-");
		X_EQUAL(v.ref(), Sequence("-"));
		X_EQUAL(v.obs(), Sequence("A"));
		I_EQUAL(v.start(), 66);
		I_EQUAL(v.end(), 66);

		v = Variant("chr18", 65, 65, "A", "ATA");
		v.normalize("-");
		X_EQUAL(v.ref(), Sequence("-"));
		X_EQUAL(v.obs(), Sequence("TA"));
		I_EQUAL(v.start(), 66);
		I_EQUAL(v.end(), 66);
	}

	void toHGVS()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex genome_index(ref_file);

		//SNP
		Variant v = Variant("chr1", 120611964, 120611964, "G", "C");
		S_EQUAL(v.toHGVS(genome_index), "g.120611964G>C");

		//INS
		v = Variant("chr1", 866509, 866509, "-", "CCCT");
		S_EQUAL(v.toHGVS(genome_index), "g.866509_866510insCCCT");
		v = Variant("chr1", 866509, 866509, "", "CCCT");
		S_EQUAL(v.toHGVS(genome_index), "g.866509_866510insCCCT");

		//DEL (single base)
		v = Variant("chr9", 98232224, 98232224, "A", "-");
		S_EQUAL(v.toHGVS(genome_index), "g.98232224del");
		v = Variant("chr9", 98232224, 98232224, "A", "");
		S_EQUAL(v.toHGVS(genome_index), "g.98232224del");

		//DEL (multiple base)
		v = Variant("chr9", 98232224, 98232225, "AA", "-");
		S_EQUAL(v.toHGVS(genome_index), "g.98232224_98232225del");
		v = Variant("chr9", 98232224, 98232225, "AA", "");
		S_EQUAL(v.toHGVS(genome_index), "g.98232224_98232225del");

		//DUP (single base)
		v = Variant("chr1", 120069350, 120069350, "-", "G");
		S_EQUAL(v.toHGVS(genome_index), "g.120069350dup");
		v = Variant("chr1", 120069350, 120069350, "", "G");
		S_EQUAL(v.toHGVS(genome_index), "g.120069350dup");
		v = Variant("chr20", 50892387, 50892387, "-", "T");
		S_EQUAL(v.toHGVS(genome_index), "g.50892387dup");
		v = Variant("chr20", 50892387, 50892387, "", "T");
		S_EQUAL(v.toHGVS(genome_index), "g.50892387dup");
		v = Variant("chr20", 50892386, 50892386, "-", "T");
		S_EQUAL(v.toHGVS(genome_index), "g.50892387dup");
		v = Variant("chr20", 50892386, 50892386, "", "T");
		S_EQUAL(v.toHGVS(genome_index), "g.50892387dup");

		//DUP (multiple base)
		v = Variant("chr1", 120069352, 120069352, "-", "GCA");
		S_EQUAL(v.toHGVS(genome_index), "g.120069350_120069352dup");
		v = Variant("chr1", 120069352, 120069352, "", "GCA");
		S_EQUAL(v.toHGVS(genome_index), "g.120069350_120069352dup");
		v = Variant("chr1", 120069355, 120069355, "-", "GCA");
		S_EQUAL(v.toHGVS(genome_index), "g.120069353_120069355dup");
		v = Variant("chr1", 120069355, 120069355, "", "GCA");
		S_EQUAL(v.toHGVS(genome_index), "g.120069353_120069355dup");

		//INV
		v = Variant("chr1", 120069334, 120069338, "CATGC", "GCATG");
		S_EQUAL(v.toHGVS(genome_index), "g.120069334_120069338inv");
	}

	void toVCF()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex genome_index(ref_file);

		VcfLine v_rep;
		v_rep = Variant("chr1", 47110766, 47110766, "-", "C").toVCF(genome_index);
		S_EQUAL(v_rep.chr().str(), "chr1");
		I_EQUAL(v_rep.start(), 47110766);
		S_EQUAL(v_rep.ref(), "C");
		S_EQUAL(v_rep.altString(), "CC");

		v_rep = Variant("chr1", 47133811, 47133811, "T", "C").toVCF(genome_index);
		S_EQUAL(v_rep.chr().str(), "chr1");
		I_EQUAL(v_rep.start(), 47133811);
		S_EQUAL(v_rep.ref(), "T");
		S_EQUAL(v_rep.altString(), "C");

		v_rep = Variant("chr1", 47181921, 47181921, "A", "-").toVCF(genome_index);
		S_EQUAL(v_rep.chr().str(), "chr1");
		I_EQUAL(v_rep.start(), 47181920);
		S_EQUAL(v_rep.ref(), "CA");
		S_EQUAL(v_rep.altString(), "C");

		v_rep = Variant("chr1", 47181921, 47181922, "AA", "GC").toVCF(genome_index);
		S_EQUAL(v_rep.chr().str(), "chr1");
		I_EQUAL(v_rep.start(), 47181920);
		S_EQUAL(v_rep.ref(), "CAA");
		S_EQUAL(v_rep.altString(), "CGC");

		v_rep = Variant("chr1", 47181921, 47181921, "A", "TGC").toVCF(genome_index);
		S_EQUAL(v_rep.chr().str(), "chr1");
		I_EQUAL(v_rep.start(), 47181920);
		S_EQUAL(v_rep.ref(), "CA");
		S_EQUAL(v_rep.altString(), "CTGC");

		v_rep = Variant("chr1", 47181921, 47181925, "AAAAA", "GCT").toVCF(genome_index);
		S_EQUAL(v_rep.chr().str(), "chr1");
		I_EQUAL(v_rep.start(), 47181920);
		S_EQUAL(v_rep.ref(), "CAAAAA");
		S_EQUAL(v_rep.altString(), "CGCT");

		v_rep = Variant("chr1", 47181921, 47181925, "AAAAA", "GCTGCTGCT").toVCF(genome_index);
		S_EQUAL(v_rep.chr().str(), "chr1");
		I_EQUAL(v_rep.start(), 47181920);
		S_EQUAL(v_rep.ref(), "CAAAAA");
		S_EQUAL(v_rep.altString(), "CGCTGCTGCT");
	}

	void addFilter()
	{
		Variant v = Variant("chr1", 120611964, 120611964, "G", "C");
		v.annotations().append("");
		I_EQUAL(v.filters().count(), 0);

		v.addFilter("off-target", 0);

		I_EQUAL(v.filters().count(), 1);
		S_EQUAL(v.annotations()[0], "off-target");

		v.addFilter("off-target2", 0);

		I_EQUAL(v.filters().count(), 2);
		S_EQUAL(v.annotations()[0], "off-target;off-target2");
	}


	void fromString()
	{
		//GSvar format (with tabs)
		Variant v1 = Variant::fromString("chr1	1423281	1423281	G	A");
		S_EQUAL(v1.toString(), "chr1:1423281-1423281 G>A");
		Variant v2 = Variant::fromString("chr14	23371255	23371255	-	GGC");
		S_EQUAL(v2.toString(), "chr14:23371255-23371255 ->GGC");
		Variant v3 = Variant::fromString("chr11	111742146	111742146	G	-");
		S_EQUAL(v3.toString(), "chr11:111742146-111742146 G>-");

		//GSvar format (human readable)
		Variant v4 = Variant::fromString("chr17:41258507-41258507  G > A");
		S_EQUAL(v4.toString(), "chr17:41258507-41258507 G>A");
		Variant v5 = Variant::fromString("chr17:41251845-41251846 AG  > -");
		S_EQUAL(v5.toString(), "chr17:41251845-41251846 AG>-");
		Variant v6 = Variant::fromString("chr17:41256250-41256250 - >  T");
		S_EQUAL(v6.toString(), "chr17:41256250-41256250 ->T");
		Variant v7 = Variant::fromString("chr17:41256250-41256250->T");
		S_EQUAL(v7.toString(), "chr17:41256250-41256250 ->T");
		Variant v8 = Variant::fromString("chr17:41256250-41256250T>-");
		S_EQUAL(v8.toString(), "chr17:41256250-41256250 T>-");
	}

	void checkValid()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		Variant v;
		v = Variant ("chr4", 88536883, 88536900, "AAATTATTTTCTGCCTGG", "-");
		v.checkValid(ref_file);

		v = Variant("chr5", 76841292, 76841292, "A", "-");
		v.checkValid(ref_file);

		v = Variant("chr5", 76841292, 76841292, "T", "-");
		IS_THROWN(ArgumentException, v.checkValid(ref_file));

		v = Variant("chr5", 76841292, 76841292, "T", "T");
		IS_THROWN(ArgumentException, v.checkValid(ref_file));

		v = Variant("chr5", 76841292, 76841292, "TT", "-");
		IS_THROWN(ArgumentException, v.checkValid(ref_file));
	}

	void leftAlign()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");

		Variant v;

		//SNP > no change
		v = Variant("chr17", 43094517, 43094517, "T", "A");
		v.leftAlign(ref_file);
		I_EQUAL(v.start(), 43094517);
		I_EQUAL(v.end(), 43094517);
		S_EQUAL(v.ref(), "T");
		S_EQUAL(v.obs(), "A");

		//INS (one base block shift)
		v = Variant("chr17", 43094517, 43094517, "-", "T");
		v.leftAlign(ref_file);
		I_EQUAL(v.start(), 43094514);
		I_EQUAL(v.end(), 43094514);
		S_EQUAL(v.ref(), "-");
		S_EQUAL(v.obs(), "T");

		//INS (no block shift)
		v = Variant("chr3", 195580369, 195580369, "-", "TTC");
		v.leftAlign(ref_file);
		I_EQUAL(v.start(), 195580369);
		I_EQUAL(v.end(), 195580369);
		S_EQUAL(v.ref(), "-");
		S_EQUAL(v.obs(), "TTC");

		//DEL (two base block shift)
		v = Variant("chr3", 196229876, 196229877, "AG", "-");
		v.leftAlign(ref_file);
		I_EQUAL(v.start(), 196229856);
		I_EQUAL(v.end(), 196229857);
		S_EQUAL(v.ref(), "AG");
		S_EQUAL(v.obs(), "-");

		//DEL (no block shift, but part of sequence before and after match)
		v = Variant("chr4", 87615731, 87615748, "TAGCAGTGACAGCAGCAA", "-");
		v.leftAlign(ref_file);
		I_EQUAL(v.start(), 87615717);
		I_EQUAL(v.end(), 87615734);
		S_EQUAL(v.ref(), "AGTGACAGCAGCAATAGC");
		S_EQUAL(v.obs(), "-");
	}
};
