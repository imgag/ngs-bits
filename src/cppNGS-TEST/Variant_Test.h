#include "TestFramework.h"
#include "VariantFilter.h"
#include "VariantList.h"
#include "Settings.h"

class Variant_Test
		: public QObject
{
	Q_OBJECT

private slots:

	void static_normalize()
	{
		QString ref = "A";
		QString obs = "AGG";
		int pos = 17;
		Variant::normalize(pos, ref, obs);
		QCOMPARE(ref, QString(""));
		QCOMPARE(obs, QString("GG"));
		QCOMPARE(pos, 18);

		ref = "ATG";
		obs = "AGGTG";
		pos = 17;
		Variant::normalize(pos, ref, obs);
		QCOMPARE(ref, QString(""));
		QCOMPARE(obs, QString("GG"));
		QCOMPARE(pos, 18);

		ref = "TT";
		obs = "";
		pos = 17;
		Variant::normalize(pos, ref, obs);
		QCOMPARE(ref, QString("TT"));
		QCOMPARE(obs, QString(""));
		QCOMPARE(pos, 17);

		ref = "TAT";
		obs = "TT";
		pos = 17;
		Variant::normalize(pos, ref, obs);
		QCOMPARE(ref, QString("A"));
		QCOMPARE(obs, QString(""));
		QCOMPARE(pos, 18);

		ref = "TCCAT";
		obs = "TCCT";
		pos = 17;
		Variant::normalize(pos, ref, obs);
		QCOMPARE(ref, QString("A"));
		QCOMPARE(obs, QString(""));
		QCOMPARE(pos, 20);

		ref = "TCCT";
		obs = "TCCT";
		pos = 17;
		Variant::normalize(pos, ref, obs);
		QCOMPARE(ref, QString("C"));
		QCOMPARE(obs, QString("C"));
		QCOMPARE(pos, 18);

		ref = "T";
		obs = "T";
		pos = 17;
		Variant::normalize(pos, ref, obs);
		QCOMPARE(ref, QString("T"));
		QCOMPARE(obs, QString("T"));
		QCOMPARE(pos, 17);
	}

	void static_minBlock()
	{
		QCOMPARE(Variant::minBlock("ACACAC"), QString("AC"));
		QCOMPARE(Variant::minBlock("ACAC"), QString("AC"));
		QCOMPARE(Variant::minBlock("AC"), QString("AC"));
		QCOMPARE(Variant::minBlock("AAA"), QString("A"));
		QCOMPARE(Variant::minBlock("CC"), QString("C"));
		QCOMPARE(Variant::minBlock("ACGTACGT"), QString("ACGT"));
		QCOMPARE(Variant::minBlock("ACGT"), QString("ACGT"));
	}

	void static_indelRegion()
	{
		QString ref_file = Settings::string("reference_genome");
		if (ref_file=="") QSKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		//insertion T left (TSV-style)
		QPair<int, int> pos = Variant::indelRegion("chr6", 110053824, 110053824, "-", "T", reference);
		QCOMPARE(pos.first, 110053825);
		QCOMPARE(pos.second, 110053837);

		//insertion T right (TSV-style)
		pos = Variant::indelRegion("chr6", 110053837, 110053837, "-", "T", reference);
		QCOMPARE(pos.first, 110053825);
		QCOMPARE(pos.second, 110053837);

		//insertion TT left (TSV-style)
		pos = Variant::indelRegion("chr6", 110053824, 110053824, "-", "TT", reference);
		QCOMPARE(pos.first, 110053825);
		QCOMPARE(pos.second, 110053837);

		//insertion TT right (TSV-style)
		pos = Variant::indelRegion("chr6", 110053837, 110053837, "-", "TT", reference);
		QCOMPARE(pos.first, 110053825);
		QCOMPARE(pos.second, 110053837);

		//deletion T left (TSV-style)
		pos = Variant::indelRegion("chr6", 110053825, 110053825, "T", "-", reference);
		QCOMPARE(pos.first, 110053825);
		QCOMPARE(pos.second, 110053837);

		//deletion T right (TSV-style)
		 pos = Variant::indelRegion("chr6", 110053837, 110053837, "T", "-", reference);
		QCOMPARE(pos.first, 110053825);
		QCOMPARE(pos.second, 110053837);

		//deletion TT left (TSV-style)
		pos = Variant::indelRegion("chr6", 110053825, 110053826, "TT", "-", reference);
		QCOMPARE(pos.first, 110053825);
		QCOMPARE(pos.second, 110053837);

		//deletion TT right (TSV-style)
		 pos = Variant::indelRegion("chr6", 110053836, 110053837, "TT", "-", reference);
		QCOMPARE(pos.first, 110053825);
		QCOMPARE(pos.second, 110053837);

		//deletion AG left (TSV-style)
		pos = Variant::indelRegion("chr14", 53513479, 53513480, "AG", "-", reference);
		QCOMPARE(pos.first, 53513479);
		QCOMPARE(pos.second, 53513484);

		//deletion AG right (TSV-style)
		pos = Variant::indelRegion("chr14", 53513483, 53513484, "AG", "-", reference);
		QCOMPARE(pos.first, 53513479);
		QCOMPARE(pos.second, 53513484);

		//insertion outside repeat => no region
		pos = Variant::indelRegion("chr14", 53207583, 53207583, "-", "CACCAAAGCCACCAGTGCCGAAACCAGCTCCGAAGCCGCCGG", reference);
		QCOMPARE(pos.first, 53207583);
		QCOMPARE(pos.second, 53207583);

		//deletion outside repeat => no region
		pos = Variant::indelRegion("chr14", 53207582, 53207584, "AAC", "-", reference);
		QCOMPARE(pos.first, 53207582);
		QCOMPARE(pos.second, 53207584);

		//complex indel => no region
		pos = Variant::indelRegion("chr14", 53513479, 53513480, "AG", "TT", reference);
		QCOMPARE(pos.first, 53513479);
		QCOMPARE(pos.second, 53513480);

		//SNV => no region
		pos = Variant::indelRegion("chr14", 53513479, 53513479, "A", "G", reference);
		QCOMPARE(pos.first, 53513479);
		QCOMPARE(pos.second, 53513479);
	}

	void overlapsWithComplete()
	{
		Variant line1("chr1", 5, 10, "r", "o");
		QVERIFY(!line1.overlapsWith("chr2", 5, 10));
		QVERIFY(!line1.overlapsWith("chr1", 1, 4));
		QVERIFY(!line1.overlapsWith("chr1", 11, 20));
		QVERIFY(line1.overlapsWith("chr1", 1, 5));
		QVERIFY(line1.overlapsWith("chr1", 5, 10));
		QVERIFY(line1.overlapsWith("chr1", 6, 8));
		QVERIFY(line1.overlapsWith("chr1", 10, 20));
		QVERIFY(line1.overlapsWith("chr1", 1, 20));
	}

	void overlapsWithPosition()
	{
		Variant line1("chr1", 5, 10, "r", "o");
		QVERIFY(line1.overlapsWith(5, 10));
		QVERIFY(!line1.overlapsWith(1, 4));
		QVERIFY(!line1.overlapsWith(11, 20));
		QVERIFY(line1.overlapsWith(1, 5));
		QVERIFY(line1.overlapsWith(5, 10));
		QVERIFY(line1.overlapsWith(6, 8));
		QVERIFY(line1.overlapsWith(10, 20));
		QVERIFY(line1.overlapsWith(1, 20));
	}

	void operator_lessthan()
	{
		QCOMPARE(Variant("chr1", 1, 20, "r", "o")<Variant("chr1", 1, 20, "r", "o"), false);
		QCOMPARE(Variant("chr1", 1, 20, "r", "o")<Variant("chr1", 5, 20, "r", "o"), true);
		QCOMPARE(Variant("chr2", 1, 20, "r", "o")<Variant("chr1", 1, 20, "r", "o"), false);
		QCOMPARE(Variant("chr1", 1, 20, "r", "o")<Variant("chr2", 5, 20, "r", "o"), true);
	}

	void normalize()
	{
		//tests with empty empty sequence
		Variant v = Variant("chr1", 17, 17, "A", "AGG");
		v.normalize();
		QCOMPARE(v.ref(), QString(""));
		QCOMPARE(v.obs(), QString("GG"));
		QCOMPARE(v.start(), 18);
		QCOMPARE(v.end(), 18);

		v = Variant("chr1", 17, 17, "ATG", "AGGTG");
		v.normalize("");
		QCOMPARE(v.ref(), QString(""));
		QCOMPARE(v.obs(), QString("GG"));
		QCOMPARE(v.start(), 18);
		QCOMPARE(v.end(), 18);

		v = Variant("chr1", 17, 18, "TT", "");
		v.normalize("");
		QCOMPARE(v.ref(), QString("TT"));
		QCOMPARE(v.obs(), QString(""));
		QCOMPARE(v.start(), 17);
		QCOMPARE(v.end(), 18);

		v = Variant("chr1", 17, 19, "TAT", "TT");
		v.normalize("");
		QCOMPARE(v.ref(), QString("A"));
		QCOMPARE(v.obs(), QString(""));
		QCOMPARE(v.start(), 18);
		QCOMPARE(v.end(), 18);

		//tests with non-empty empty sequence
		v = Variant("chr1", 17, 17, "A", "AGG");
		v.normalize("-");
		QCOMPARE(v.ref(), QString("-"));
		QCOMPARE(v.obs(), QString("GG"));
		QCOMPARE(v.start(), 18);
		QCOMPARE(v.end(), 18);

		v = Variant("chr1", 17, 17, "ATG", "AGGTG");
		v.normalize("-");
		QCOMPARE(v.ref(), QString("-"));
		QCOMPARE(v.obs(), QString("GG"));
		QCOMPARE(v.start(), 18);
		QCOMPARE(v.end(), 18);

		v = Variant("chr1", 17, 18, "TT", "");
		v.normalize("-");
		QCOMPARE(v.ref(), QString("TT"));
		QCOMPARE(v.obs(), QString("-"));
		QCOMPARE(v.start(), 17);
		QCOMPARE(v.end(), 18);

		v = Variant("chr1", 17, 19, "TAT", "TT");
		v.normalize("-");
		QCOMPARE(v.ref(), QString("A"));
		QCOMPARE(v.obs(), QString("-"));
		QCOMPARE(v.start(), 18);
		QCOMPARE(v.end(), 18);

		v = Variant("chr18", 65, 65, "A", "AA");
		v.normalize("-");
		QCOMPARE(v.ref(), QString("-"));
		QCOMPARE(v.obs(), QString("A"));
		QCOMPARE(v.start(), 66);
		QCOMPARE(v.end(), 66);

		v = Variant("chr18", 65, 65, "A", "ATA");
		v.normalize("-");
		QCOMPARE(v.ref(), QString("-"));
		QCOMPARE(v.obs(), QString("TA"));
		QCOMPARE(v.start(), 66);
		QCOMPARE(v.end(), 66);
	}

};

TFW_DECLARE(Variant_Test)
