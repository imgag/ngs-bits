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
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		//insertion T left (TSV-style)
		QPair<int, int> pos = Variant::indelRegion("chr6", 110053824, 110053824, "-", "T", reference);
		I_EQUAL(pos.first, 110053825);
		I_EQUAL(pos.second, 110053837);

		//insertion T right (TSV-style)
		pos = Variant::indelRegion("chr6", 110053837, 110053837, "-", "T", reference);
		I_EQUAL(pos.first, 110053825);
		I_EQUAL(pos.second, 110053837);

		//insertion TT left (TSV-style)
		pos = Variant::indelRegion("chr6", 110053824, 110053824, "-", "TT", reference);
		I_EQUAL(pos.first, 110053825);
		I_EQUAL(pos.second, 110053837);

		//insertion TT right (TSV-style)
		pos = Variant::indelRegion("chr6", 110053837, 110053837, "-", "TT", reference);
		I_EQUAL(pos.first, 110053825);
		I_EQUAL(pos.second, 110053837);

		//deletion T left (TSV-style)
		pos = Variant::indelRegion("chr6", 110053825, 110053825, "T", "-", reference);
		I_EQUAL(pos.first, 110053825);
		I_EQUAL(pos.second, 110053837);

		//deletion T right (TSV-style)
		 pos = Variant::indelRegion("chr6", 110053837, 110053837, "T", "-", reference);
		I_EQUAL(pos.first, 110053825);
		I_EQUAL(pos.second, 110053837);

		//deletion TT left (TSV-style)
		pos = Variant::indelRegion("chr6", 110053825, 110053826, "TT", "-", reference);
		I_EQUAL(pos.first, 110053825);
		I_EQUAL(pos.second, 110053837);

		//deletion TT right (TSV-style)
		 pos = Variant::indelRegion("chr6", 110053836, 110053837, "TT", "-", reference);
		I_EQUAL(pos.first, 110053825);
		I_EQUAL(pos.second, 110053837);

		//deletion AG left (TSV-style)
		pos = Variant::indelRegion("chr14", 53513479, 53513480, "AG", "-", reference);
		I_EQUAL(pos.first, 53513479);
		I_EQUAL(pos.second, 53513484);

		//deletion AG right (TSV-style)
		pos = Variant::indelRegion("chr14", 53513483, 53513484, "AG", "-", reference);
		I_EQUAL(pos.first, 53513479);
		I_EQUAL(pos.second, 53513484);

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
		QString ref_file = Settings::string("reference_genome");
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
		v = Variant("chr1", 120611964, 120611964, "-", "G");
		S_EQUAL(v.toHGVS(genome_index), "g.120611964dup");
		v = Variant("chr1", 120611964, 120611964, "", "G");
		S_EQUAL(v.toHGVS(genome_index), "g.120611964dup");
		v = Variant("chr20", 49508924, 49508924, "-", "T");
		S_EQUAL(v.toHGVS(genome_index), "g.49508924dup");
		v = Variant("chr20", 49508924, 49508924, "", "T");
		S_EQUAL(v.toHGVS(genome_index), "g.49508924dup");
		v = Variant("chr20", 49508923, 49508923, "-", "T");
		S_EQUAL(v.toHGVS(genome_index), "g.49508924dup");
		v = Variant("chr20", 49508923, 49508923, "", "T");
		S_EQUAL(v.toHGVS(genome_index), "g.49508924dup");

		//DUP (multiple base)
		v = Variant("chr1", 120611966, 120611966, "-", "GCA");
		S_EQUAL(v.toHGVS(genome_index), "g.120611964_120611966dup");
		v = Variant("chr1", 120611966, 120611966, "", "GCA");
		S_EQUAL(v.toHGVS(genome_index), "g.120611964_120611966dup");
		v = Variant("chr1", 120611969, 120611969, "-", "GCA");
		S_EQUAL(v.toHGVS(genome_index), "g.120611967_120611969dup");
		v = Variant("chr1", 120611969, 120611969, "", "GCA");
		S_EQUAL(v.toHGVS(genome_index), "g.120611967_120611969dup");

		//INV
		v = Variant("chr1", 120611948, 120611952, "CATGC", "GCATG");
		S_EQUAL(v.toHGVS(genome_index), "g.120611948_120611952inv");
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
		//SNPs
		Variant v1 = Variant::fromString("chr1	1423281	1423281	G	A");
		S_EQUAL(v1.toString(), "chr1:1423281-1423281 G>A");

		//Insertion
		Variant v2 = Variant::fromString("chr14	23371255	23371255	-	GGC");
		S_EQUAL(v2.toString(), "chr14:23371255-23371255 ->GGC");

		//Deletion
		Variant v3 = Variant::fromString("chr11	111742146	111742146	G	-");
		S_EQUAL(v3.toString(), "chr11:111742146-111742146 G>-");
	}



	void leftAlign()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") SKIP("Test needs the reference genome!");

		Variant v("chr4", 88536883, 88536900, "TAGCAGTGACAGCAGCAA", "-");
		v.leftAlign(ref_file);
	}
};
