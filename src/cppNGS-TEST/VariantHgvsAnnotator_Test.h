#include "TestFramework.h"
#include "VariantHgvsAnnotator.h"
#include "Transcript.h"
#include "VcfFile.h"
#include "Sequence.h"
#include "Settings.h"
#include <QTextStream>
#include <QVector>
#include "Helper.h"

TEST_CLASS(VariantHgvsAnnotator_Test)
{
private:
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

    Transcript trans_DECR1()
    {
        Transcript t;

        t.setGene("DECR1");
        t.setName("ENST00000522161");
        t.setSource(Transcript::ENSEMBL);
        t.setStrand(Transcript::PLUS);

        BedFile regions;
        regions.append(BedLine("chr8", 90001477, 90001561));
        regions.append(BedLine("chr8", 90005448, 90005845));
        regions.append(BedLine("chr8", 90006036, 90006277));
        regions.append(BedLine("chr8", 90017124, 90017326));
        regions.append(BedLine("chr8", 90018909, 90018966));
        regions.append(BedLine("chr8", 90019086, 90019172));
        regions.append(BedLine("chr8", 90020909, 90021056));
        regions.append(BedLine("chr8", 90036841, 90036940));
        regions.append(BedLine("chr8", 90042728, 90042800));
        regions.append(BedLine("chr8", 90044849, 90044995));
        regions.append(BedLine("chr8", 90051677, 90051739));
        regions.append(BedLine("chr8", 90051838, 90051999));
        t.setRegions(regions, 90006236, 90051897);

        return t;

    }

    Transcript trans_CALCA()
    {
        Transcript t;

        t.setGene("CALCA");
        t.setName("ENST00000361010");
        t.setSource(Transcript::ENSEMBL);
        t.setStrand(Transcript::MINUS);

        BedFile regions;
        regions.append(BedLine("chr11", 14966669, 14967101));
        regions.append(BedLine("chr11", 14967674, 14967854));
        regions.append(BedLine("chr11", 14969935, 14970075));
        regions.append(BedLine("chr11", 14971107, 14971201));
        regions.append(BedLine("chr11", 14972221, 14972286));
        t.setRegions(regions, 14971192, 14967695);

        return t;
    }

	Transcript trans_CDKN1C()
	{
		Transcript t;
		t.setGene("CDKN1C");
		t.setName("ENST00000414822");
		t.setVersion(8);
		t.setSource(Transcript::ENSEMBL);
		t.setStrand(Transcript::MINUS);

		BedFile regions;
		regions.append(BedLine("chr11", 2883213, 2883910));
		regions.append(BedLine("chr11", 2883999, 2884134));
		regions.append(BedLine("chr11", 2884670, 2885771));
		t.setRegions(regions, 2885489, 2884004);

		return t;
	}

	Transcript trans_NEAT1()
	{
		Transcript t;
		t.setGene("NEAT1");
		t.setName("ENST00000499732");
		t.setVersion(3);
		t.setSource(Transcript::ENSEMBL);
		t.setStrand(Transcript::PLUS);

		BedFile regions;
		regions.append(BedLine("chr11", 65422774, 65423383));
		regions.append(BedLine("chr11", 65423627, 65426457));
		t.setRegions(regions);

		return t;
	}

	Transcript trans_SPTBN1()
	{
		Transcript t;
		t.setGene("SPTBN1");
		t.setName("ENST00000356805");
		t.setVersion(9);
		t.setSource(Transcript::ENSEMBL);
		t.setStrand(Transcript::PLUS);

		BedFile regions;
		regions.append(BedLine("chr2", 54456327, 54456518));
		regions.append(BedLine("chr2", 54526372, 54526566));
		regions.append(BedLine("chr2", 54599092, 54599243));
		regions.append(BedLine("chr2", 54612161, 54612334));
		regions.append(BedLine("chr2", 54616207, 54616298));
		regions.append(BedLine("chr2", 54617608, 54617688));
		regions.append(BedLine("chr2", 54618078, 54618193));
		regions.append(BedLine("chr2", 54621400, 54621512));
		regions.append(BedLine("chr2", 54622300, 54622487));
		regions.append(BedLine("chr2", 54623479, 54623596));
		regions.append(BedLine("chr2", 54624804, 54624962));
		regions.append(BedLine("chr2", 54625932, 54626234));
		regions.append(BedLine("chr2", 54628097, 54628250));
		regions.append(BedLine("chr2", 54628933, 54629803));
		regions.append(BedLine("chr2", 54629892, 54630029));
		regions.append(BedLine("chr2", 54630855, 54631611));
		regions.append(BedLine("chr2", 54632566, 54632768));
		regions.append(BedLine("chr2", 54637713, 54637803));
		regions.append(BedLine("chr2", 54642983, 54643129));
		regions.append(BedLine("chr2", 54644323, 54644586));
		regions.append(BedLine("chr2", 54645229, 54645453));
		regions.append(BedLine("chr2", 54645928, 54646017));
		regions.append(BedLine("chr2", 54646194, 54646475));
		regions.append(BedLine("chr2", 54647131, 54647261));
		regions.append(BedLine("chr2", 54648986, 54649190));
		regions.append(BedLine("chr2", 54649615, 54649989));
		regions.append(BedLine("chr2", 54653609, 54653853));
		regions.append(BedLine("chr2", 54655070, 54655208));
		regions.append(BedLine("chr2", 54655914, 54655998));
		regions.append(BedLine("chr2", 54657850, 54658046));
		regions.append(BedLine("chr2", 54659154, 54659266));
		regions.append(BedLine("chr2", 54659936, 54659999));
		regions.append(BedLine("chr2", 54664453, 54664691));
		regions.append(BedLine("chr2", 54665915, 54666088));
		regions.append(BedLine("chr2", 54667604, 54667646));
		regions.append(BedLine("chr2", 54668351, 54671446));

		t.setRegions(regions, 54526419, 54668569);

		return t;
	}

	Transcript trans_GLMN()
	{
		Transcript t;
		t.setGene("GLMN");
		t.setName("ENST00000370360");
		t.setVersion(8);
		t.setSource(Transcript::ENSEMBL);
		t.setStrand(Transcript::MINUS);

		BedFile regions;
		regions.append(BedLine("chr1", 92246402, 92246646));
		regions.append(BedLine("chr1", 92247062, 92247144));
		regions.append(BedLine("chr1", 92247878, 92247989));
		regions.append(BedLine("chr1", 92262863, 92262926));
		regions.append(BedLine("chr1", 92263623, 92263732));
		regions.append(BedLine("chr1", 92264554, 92264638));
		regions.append(BedLine("chr1", 92266419, 92266492));
		regions.append(BedLine("chr1", 92266700, 92266741));
		regions.append(BedLine("chr1", 92267913, 92268002));
		regions.append(BedLine("chr1", 92268105, 92268135));
		regions.append(BedLine("chr1", 92269723, 92269776));
		regions.append(BedLine("chr1", 92271465, 92271652));
		regions.append(BedLine("chr1", 92286490, 92286592));
		regions.append(BedLine("chr1", 92288914, 92289151));
		regions.append(BedLine("chr1", 92290198, 92290306));
		regions.append(BedLine("chr1", 92291418, 92291537));
		regions.append(BedLine("chr1", 92297404, 92297529));
		regions.append(BedLine("chr1", 92297961, 92298029));
		regions.append(BedLine("chr1", 92298925, 92298987));
		t.setRegions(regions, 92297999, 92246530);

		return t;
	}

	Transcript trans_BCL11A()
	{
		Transcript t;
		t.setGene("BCL11A");
		t.setName("ENST00000642384");
		t.setVersion(2);
		t.setSource(Transcript::ENSEMBL);
		t.setStrand(Transcript::MINUS);

		BedFile regions;
		regions.append(BedLine("chr2", 60457194, 60462424));
		regions.append(BedLine("chr2", 60468732, 60468833));
		regions.append(BedLine("chr2", 60545971, 60546300));
		regions.append(BedLine("chr2", 60553216, 60553654));
		t.setRegions(regions, 60553270, 60460404);

		return t;
	}

	Transcript trans_CTU2()
	{
		Transcript t;
		t.setGene("CTU2");
		t.setName("ENST00000453996");
		t.setVersion(7);
		t.setSource(Transcript::ENSEMBL);
		t.setStrand(Transcript::PLUS);

		BedFile regions;
		regions.append(BedLine("chr16", 88706503, 88706598));
		regions.append(BedLine("chr16", 88707136, 88707210));
		regions.append(BedLine("chr16", 88709938, 88710016));
		regions.append(BedLine("chr16", 88710223, 88710282));
		regions.append(BedLine("chr16", 88711635, 88711695));
		regions.append(BedLine("chr16", 88712274, 88712383));
		regions.append(BedLine("chr16", 88712622, 88712905));
		regions.append(BedLine("chr16", 88713312, 88713447));
		regions.append(BedLine("chr16", 88713647, 88713778));
		regions.append(BedLine("chr16", 88714136, 88714227));
		regions.append(BedLine("chr16", 88714383, 88714486));
		regions.append(BedLine("chr16", 88714587, 88714737));
		regions.append(BedLine("chr16", 88714860, 88714926));
		regions.append(BedLine("chr16", 88715048, 88715106));
		regions.append(BedLine("chr16", 88715182, 88715396));
		t.setRegions(regions, 88706531, 88715251);

		return t;
	}

	Transcript trans_DNLZ()
	{
		Transcript t;
		t.setGene("DNLZ");
		t.setName("ENST00000371738");
		t.setVersion(4);
		t.setSource(Transcript::ENSEMBL);
		t.setStrand(Transcript::MINUS);

		BedFile regions;
		regions.append(BedLine("chr9", 136359483, 136362180));
		regions.append(BedLine("chr9", 136362989, 136363128));
		regions.append(BedLine("chr9", 136363487, 136363744));
		t.setRegions(regions, 136363714, 136362012);

		return t;
	}

private:

	void annotate_plus_strand()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(reference, VariantHgvsAnnotator::Parameters(5000, 3, 8, 8));
        Transcript t = trans_SLC51A();

		//SNV exon synonymous
		VcfLine variant(Chromosome("chr3"), 196217926, "A", QList<Sequence>() << "G");
		VariantConsequence hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.123A>G");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Gln41=");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SYNONYMOUS_VARIANT));
        I_EQUAL(hgvs.exon_number, 2);
		I_EQUAL(hgvs.intron_number, -1);
		S_EQUAL(variantImpactToString(hgvs.impact), "LOW");

		//SNV exon stop gained
        variant.setPos(196233116);
		variant.setRef("C");
		variant.setSingleAlt("T");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.940C>T");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Arg314Ter");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::STOP_GAINED));
        I_EQUAL(hgvs.exon_number, 9);
		I_EQUAL(hgvs.intron_number, -1);
		S_EQUAL(variantImpactToString(hgvs.impact), "HIGH");

		//SNV exon stop lost
        variant.setPos(196233197);
		variant.setRef("T");
		variant.setSingleAlt("C");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.1021T>C");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Ter341GlnextTer7");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::STOP_LOST));
        I_EQUAL(hgvs.exon_number, 9);
		I_EQUAL(hgvs.intron_number, -1);

		//SNV exon splice region
        variant.setPos(196217844);
        variant.setRef("A");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.41A>G");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Tyr14Cys");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::MISSENSE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        I_EQUAL(hgvs.exon_number, 2);
		I_EQUAL(hgvs.intron_number, -1);
		S_EQUAL(variantImpactToString(hgvs.impact), "MODERATE");


		//SNV intron splice acceptor
        variant.setPos(196232417);
        variant.setRef("A");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.781-2A>G");
        S_EQUAL(hgvs.hgvs_p, "p.?");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 7);

		//SNV intron splice donor
        variant.setPos(196232526);
        variant.setRef("T");
		variant.setSingleAlt("C");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.886+2T>C");
        S_EQUAL(hgvs.hgvs_p, "p.?");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_DONOR_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 8);

		//SNV intron
        variant.setPos(196216796);
        variant.setRef("G");
		variant.setSingleAlt("T");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.38+46G>T");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 1);

		//SNV intron
        variant.setPos(196226922);
        variant.setRef("C");
		variant.setSingleAlt("T");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.134-43C>T");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 2);

		//SNV 5 prime utr
        variant.setPos(196216594);
        variant.setRef("A");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.-119A>G");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);

		//SNV 3 prime utr
        variant.setPos(196233247);
        variant.setRef("A");
		variant.setSingleAlt("C");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.*48A>C");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 9);
		I_EQUAL(hgvs.intron_number, -1);

		// upstream gene variant
        variant.setPos(196215616);
        variant.setRef("A");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::UPSTREAM_GENE_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, -1);
		S_EQUAL(variantImpactToString(hgvs.impact), "MODIFIER");

		// deletion intron
        variant.setPos(196217223);
        variant.setRef("CCTCT");
		variant.setSingleAlt("C");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.38+474_38+477del");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 1);

		// deletion 5 prime utr
        variant.setPos(196216610);
        variant.setRef("CA");
		variant.setSingleAlt("C");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.-102del");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);

		// frameshift deletion
        variant.setPos(196217907);
        variant.setRef("AG");
		variant.setSingleAlt("A");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.105del");
        S_EQUAL(hgvs.hgvs_p, "p.Gln35HisfsTer9");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 2);
		I_EQUAL(hgvs.intron_number, -1);

		// frameshift deletion
        variant.setPos(196229958);
        variant.setRef("GC");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.678del");
        S_EQUAL(hgvs.hgvs_p, "p.Val227CysfsTer39");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 7);
		I_EQUAL(hgvs.intron_number, -1);

		// frameshift deletion
        variant.setPos(196229983);
        variant.setRef("GACCC");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.703_706del");
        S_EQUAL(hgvs.hgvs_p, "p.Thr235TrpfsTer30");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 7);
		I_EQUAL(hgvs.intron_number, -1);

		// inframe deletion (shifted to the right by 3 nt according to 3 prime rule)
        variant.setPos(196217846);
        variant.setRef("ACAG");
		variant.setSingleAlt("A");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.47_49del");
        S_EQUAL(hgvs.hgvs_p, "p.Ala16del");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 2);
		I_EQUAL(hgvs.intron_number, -1);

		// insertion 5 prime utr
        variant.setPos(196216604);
        variant.setRef("G");
		variant.setSingleAlt("GA");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.-109_-108insA");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);

		// insertion intron
        variant.setPos(196221844);
        variant.setRef("T");
		variant.setSingleAlt("TCCCAGCCG");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.133+3908_133+3909insCCCAGCCG");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 2);

		// frameshift insertion
        variant.setPos(196230043);
        variant.setRef("C");
		variant.setSingleAlt("CGGGTGACAGAGTGACACCATCTCTTGAAAGAGAGAGAGAGAGAGAG");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.762_763insGGGTGACAGAGTGACACCATCTCTTGAAAGAGAGAGAGAGAGAGAG");
        S_EQUAL(hgvs.hgvs_p, "p.Lys255GlyfsTer2");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        //IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::STOP_GAINED));
        I_EQUAL(hgvs.exon_number, 7);
		I_EQUAL(hgvs.intron_number, -1);

		// inframe duplication
        variant.setPos(196228229);
        variant.setRef("CTGCTGC");
		variant.setSingleAlt("CTGCTGCTGC");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.490_492dup");
        S_EQUAL(hgvs.hgvs_p, "p.Cys164dup");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_INSERTION));

        Transcript t_2 = trans_DECR1();

		// inframe deletion
        variant.setChromosome(Chromosome("chr8"));
        variant.setPos(90006261);
        variant.setRef("TCCT");
		variant.setSingleAlt("T");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.27_29del");
        S_EQUAL(hgvs.hgvs_p, "p.Leu11del"); // obtained by shifting AA to the most C-terminal position possible
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 3);
		I_EQUAL(hgvs.intron_number, -1);

		// inframe deletion
        variant.setPos(90044895);
        variant.setRef("TGAT");
		variant.setSingleAlt("T");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.759_761del");
        S_EQUAL(hgvs.hgvs_p, "p.Met253del");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 10);
		I_EQUAL(hgvs.intron_number, -1);

		// inframe deletion
        variant.setPos(90051678);
        variant.setRef("TCATTAA");
		variant.setSingleAlt("T");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.861_866del");
        S_EQUAL(hgvs.hgvs_p, "p.Ile288_Lys289del");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 11);
		I_EQUAL(hgvs.intron_number, -1);

		// inframe deletion (shifted to the right by 3 nt according to 3 prime rule)
        variant.setPos(90051864);
        variant.setRef("AGAA");
		variant.setSingleAlt("A");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.952_954del");
        S_EQUAL(hgvs.hgvs_p, "p.Glu318del");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 12);
		I_EQUAL(hgvs.intron_number, -1);

		// frameshift insertion
        variant.setPos(90036937);
        variant.setRef("G");
		variant.setSingleAlt("GA");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.635_636insA");
        S_EQUAL(hgvs.hgvs_p, "p.Ser212ArgfsTer7");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 8);
		I_EQUAL(hgvs.intron_number, -1);

		// inframe insertion
        variant.setPos(90044968);
        variant.setRef("T");
		variant.setSingleAlt("TACA");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.831_832insACA");
        S_EQUAL(hgvs.hgvs_p, "p.Ser277_Asp278insThr");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_INSERTION));
        I_EQUAL(hgvs.exon_number, 10);
		I_EQUAL(hgvs.intron_number, -1);

		// inframe insertion, also shifted by 1 nt according to 3' rule
        variant.setPos(90044967);
        variant.setRef("G");
		variant.setSingleAlt("GTCACCG");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.831_832insCACCGT");
        S_EQUAL(hgvs.hgvs_p, "p.Ser277_Asp278insHisArg");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_INSERTION));
        I_EQUAL(hgvs.exon_number, 10);
		I_EQUAL(hgvs.intron_number, -1);

		// inframe insertion
        variant.setPos(90044967);
        variant.setRef("G");
		variant.setSingleAlt("GACACCG");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.830_831insACACCG");
        S_EQUAL(hgvs.hgvs_p, "p.Ser277delinsArgHisArg");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_INSERTION));
        I_EQUAL(hgvs.exon_number, 10);
		I_EQUAL(hgvs.intron_number, -1);

		// inframe insertion
        variant.setPos(90044966);
        variant.setRef("A");
		variant.setSingleAlt("AACG");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.829_830insACG");
        S_EQUAL(hgvs.hgvs_p, "p.Ser277delinsAsnGly");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_INSERTION));
        I_EQUAL(hgvs.exon_number, 10);
		I_EQUAL(hgvs.intron_number, -1);
    }

	void annotate_minus_strand()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(reference, VariantHgvsAnnotator::Parameters(5000, 3, 8, 8));
        Transcript t = trans_APOD();
        Transcript t_CALCA = trans_CALCA();

		//SNV exon start lost
		VcfLine variant(Chromosome("chr11"), 14971192, "T", QList<Sequence>() << "G");
		VariantConsequence hgvs = var_hgvs_anno.annotate(t_CALCA, variant);
        S_EQUAL(hgvs.hgvs_c, "c.1A>C");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Met1?");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::START_LOST));
        I_EQUAL(hgvs.exon_number, 2);
		I_EQUAL(hgvs.intron_number, -1);

		//SNV exon missense
        variant.setChromosome(Chromosome("chr3"));
        variant.setPos(195573917);
        variant.setRef("G");
		variant.setSingleAlt("A");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.178C>T");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Arg60Cys");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::MISSENSE_VARIANT));
        I_EQUAL(hgvs.exon_number, 3);
		I_EQUAL(hgvs.intron_number, -1);
		S_EQUAL(variantImpactToString(hgvs.impact), "MODERATE");

		//SNV exon stop gained
        variant.setPos(195569086);
        variant.setRef("A");
		variant.setSingleAlt("T");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.384T>A");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Tyr128Ter");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::STOP_GAINED));
        I_EQUAL(hgvs.exon_number, 5);
		I_EQUAL(hgvs.intron_number, -1);
		S_EQUAL(variantImpactToString(hgvs.impact), "HIGH");

		//SNV intron splice acceptor
        variant.setPos(195573973);
        variant.setRef("T");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.124-2A>C");
        S_EQUAL(hgvs.hgvs_p, "p.?");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 2);

		//SNV intron splice region next to acceptor
        variant.setPos(195573974);
        variant.setRef("G");
		variant.setSingleAlt("T");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.124-3C>A");
        S_EQUAL(hgvs.hgvs_p, "");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
		IS_FALSE(hgvs.types.contains(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 2);

		//SNV intron splice donor
        variant.setPos(195583876);
        variant.setRef("A");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.-35+2T>C");
        S_EQUAL(hgvs.hgvs_p, "p.?");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_DONOR_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 1);

		//SNV intron splice region
        variant.setPos(195569141);
        variant.setRef("A");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.335-6T>C");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 4);

		//SNV intron
        variant.setPos(195571119);
        variant.setRef("C");
		variant.setSingleAlt("T");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.334+158G>A");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 4);

		//SNV intron
        variant.setPos(195569688);
        variant.setRef("C");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.335-553G>C");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 4);

		//SNV 3 prime utr
        variant.setPos(195568721);
        variant.setRef("G");
		variant.setSingleAlt("T");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.*179C>A");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 5);
		I_EQUAL(hgvs.intron_number, -1);
		S_EQUAL(variantImpactToString(hgvs.impact), "MODIFIER");

		//SNV 5 prime utr intron
        variant.setPos(195579497);
        variant.setRef("T");
		variant.setSingleAlt("C");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.-34-2A>G");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 1);

		// upstream gene variant
        variant.setPos(195585307);
        variant.setRef("C");
		variant.setSingleAlt("A");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::UPSTREAM_GENE_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, -1);
		S_EQUAL(variantImpactToString(hgvs.impact), "MODIFIER");

		// downstream gene variant
        variant.setPos(195563767);
        variant.setRef("C");
		variant.setSingleAlt("A");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::DOWNSTREAM_GENE_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, -1);
		S_EQUAL(variantImpactToString(hgvs.impact), "MODIFIER");

		// deletion upstream
        variant.setPos(195584151);
        variant.setRef("TTC");
		variant.setSingleAlt("T");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::UPSTREAM_GENE_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, -1);
		S_EQUAL(variantImpactToString(hgvs.impact), "MODIFIER");

		// deletion 5 prime utr
        variant.setPos(195583887);
        variant.setRef("CAA");
		variant.setSingleAlt("C");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.-46_-45del");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);

		// deletion 3 prime utr
        variant.setPos(195568841);
        variant.setRef("GTA");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.*57_*58del");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 5);
		I_EQUAL(hgvs.intron_number, -1);

		// deletion splice region
        variant.setPos(195571369);
        variant.setRef("CA");
		variant.setSingleAlt("C");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.246-5del");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 3);

		// frameshift deletion
        variant.setPos(195568927);
        variant.setRef("GT");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.542del");
        S_EQUAL(hgvs.hgvs_p, "p.Asp181AlafsTer3");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 5);
		I_EQUAL(hgvs.intron_number, -1);

		// insertion 3 prime utr
        variant.setPos(195568842);
        variant.setRef("T");
		variant.setSingleAlt("TGGGGG");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.*57_*58insCCCCC");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 5);
		I_EQUAL(hgvs.intron_number, -1);

		// inframe duplication
        variant.setPos(195569089);
        variant.setRef("G");
		variant.setSingleAlt("GTTCTCG");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.376_381dup");
        S_EQUAL(hgvs.hgvs_p, "p.Glu126_Asn127dup");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_INSERTION));

		// inframe insertion
        variant.setPos(195569089);
        variant.setRef("G");
		variant.setSingleAlt("GATA");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.380_381insTAT");
        S_EQUAL(hgvs.hgvs_p, "p.Asn127_Tyr128insIle");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_INSERTION));

        Transcript t_2 = trans_CALCA();

		// frameshift deletion
        variant.setChromosome(Chromosome("chr11"));
        variant.setPos(14969949);
        variant.setRef("CTCTT");
		variant.setSingleAlt("C");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.209_212del");
        S_EQUAL(hgvs.hgvs_p, "p.Gln70ArgfsTer20");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 3);
		I_EQUAL(hgvs.intron_number, -1);

		// frameshift deletion
        variant.setPos(14967720);
        variant.setRef("CT");
		variant.setSingleAlt("C");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.361del");
        S_EQUAL(hgvs.hgvs_p, "p.Arg121GlyfsTer19");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 4);
		I_EQUAL(hgvs.intron_number, -1);

		// inframe deletion
        variant.setPos(14971128);
        variant.setRef("CTGCCTGCCTGCAACA");
		variant.setSingleAlt("C");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.50_64del");
        S_EQUAL(hgvs.hgvs_p, "p.Leu17_Ser22delinsArg");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 2);
		I_EQUAL(hgvs.intron_number, -1);

		// frameshift insertion
        variant.setPos(14970026);
        variant.setRef("C");
		variant.setSingleAlt("CACTGA");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.135_136insTCAGT");
        S_EQUAL(hgvs.hgvs_p, "p.Glu46SerfsTer18");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 3);
		I_EQUAL(hgvs.intron_number, -1);

		// frameshift insertion
        variant.setPos(14970025);
        variant.setRef("T");
		variant.setSingleAlt("TACTGA");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.136_137insTCAGT");
        S_EQUAL(hgvs.hgvs_p, "p.Glu46ValfsTer18");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 3);
		I_EQUAL(hgvs.intron_number, -1);

		// frameshift insertion
        variant.setPos(14970024);
        variant.setRef("T");
		variant.setSingleAlt("TACTGA");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.137_138insTCAGT");
        S_EQUAL(hgvs.hgvs_p, "p.Glu46AspfsTer3");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 3);
		I_EQUAL(hgvs.intron_number, -1);
    }

    void vcfToHgvsDelIns()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(reference, VariantHgvsAnnotator::Parameters(5000, 3, 8, 8));
        Transcript t = trans_APOD();

        //delins intron
		VcfLine variant(Chromosome("chr3"), 195580367, "TC", QList<Sequence>() << "TTCT");
		VariantConsequence hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.-34-873delinsAGA");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 1);

		//delins 3 prime utr
        variant.setPos(195568830);
        variant.setRef("AT");
		variant.setSingleAlt("AGGCG");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.*69delinsCGCC");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 5);
		I_EQUAL(hgvs.intron_number, -1);

        //delins intron
		t = trans_SLC51A();
        variant.setChromosome(Chromosome("chr3"));
        variant.setPos(196219096);
        variant.setRef("GGCA");
		variant.setSingleAlt("CGTG");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.133+1160_133+1163delinsCGTG");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 2);

        //delins exon missense
		t = trans_CDKN1C();
        variant.setChromosome(Chromosome("chr11"));
        variant.setPos(2884791);
        variant.setRef("GC");
		variant.setSingleAlt("CT");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.698_699delinsAG");
        S_EQUAL(hgvs.hgvs_p, "p.Arg233Gln");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::MISSENSE_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);

		//delins exon missense
        variant.setPos(2884903);
        variant.setRef("GC");
		variant.setSingleAlt("AA");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.586_587delinsTT");
        S_EQUAL(hgvs.hgvs_p, "p.Ala196Leu");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::MISSENSE_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);

		//delins exon; more than one amino acid deleted
        variant.setPos(2884900);
        variant.setRef("GCCGC");
		variant.setSingleAlt("AA");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.586_590delinsTT");
        S_EQUAL(hgvs.hgvs_p, "p.Ala196_Ala197delinsPhe");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);

		//delins exon; more than one amino acid inserted
        variant.setPos(2884903);
        variant.setRef("GC");
		variant.setSingleAlt("AAATT");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.586_587delinsAATTT");
        S_EQUAL(hgvs.hgvs_p, "p.Ala196delinsAsnLeu");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INFRAME_INSERTION));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);

		//delins exon; more than one amino acid deleted and inserted
        variant.setPos(2884900);
        variant.setRef("GCCGC");
		variant.setSingleAlt("AAATT");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.586_590delinsAATTT");
        S_EQUAL(hgvs.hgvs_p, "p.Ala196_Ala197delinsAsnPhe");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::MISSENSE_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);

		//delins exon frameshift
        variant.setPos(2884858);
        variant.setRef("GC");
		variant.setSingleAlt("GTT");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.631delinsAA");
        S_EQUAL(hgvs.hgvs_p, "p.Ala211AsnfsTer30");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);

		//delins exon frameshift
        variant.setPos(2885178);
        variant.setRef("CAG");
		variant.setSingleAlt("CC");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.310_311delinsG");
        S_EQUAL(hgvs.hgvs_p, "p.Leu104GlyfsTer168");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);
    }

    void vcfToHgvsUtrIntrons()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(reference, VariantHgvsAnnotator::Parameters(5000, 3, 8, 8));
        Transcript t = trans_DECR1();

		//SNV 5 prime utr intron plus strand
		VcfLine variant(Chromosome("chr8"), 90005382, "G", QList<Sequence>() << "A");
		VariantConsequence hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "c.-598-66G>A");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 1);

        Transcript t_2 = trans_CALCA();

		//SNV 5 prime utr intron minus strand
        variant.setChromosome(Chromosome("chr11"));
        variant.setPos(14972146);
        variant.setRef("A");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.-10+75T>C");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 1);

		//SNV 3 prime utr intron minus strand
        variant.setPos(14967138);
        variant.setRef("T");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.*22-37A>C");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 4);

		//SNV 3 prime utr intron minus strand
        variant.setPos(14967656);
        variant.setRef("T");
		variant.setSingleAlt("G");
		hgvs = var_hgvs_anno.annotate(t_2, variant);
        S_EQUAL(hgvs.hgvs_c, "c.*21+18A>C");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 4);
    }

    void vcfToHgvsNonCoding()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(reference, VariantHgvsAnnotator::Parameters(5000, 3, 8, 8));
        Transcript t = trans_NEAT1();

        // non-coding intron SNV
		VcfLine variant(Chromosome("chr11"), 65423403, "C", QList<Sequence>() << "T");
		VariantConsequence hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "n.610+20C>T");
        S_EQUAL(hgvs.hgvs_p, "");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
		I_EQUAL(hgvs.intron_number, 1);

		// non-coding exon SNV
        variant.setPos(65423327);
		variant.setRef("C");
		variant.setSingleAlt("T");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "n.554C>T");
        S_EQUAL(hgvs.hgvs_p, "");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);

		// non-coding exon deletion
        variant.setPos(65422860);
        variant.setRef("CAG");
		variant.setSingleAlt("C");
		hgvs = var_hgvs_anno.annotate(t, variant);
        S_EQUAL(hgvs.hgvs_c, "n.88_89del");
        S_EQUAL(hgvs.hgvs_p, "");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);
    }

	void bug_complex_indel()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(reference, VariantHgvsAnnotator::Parameters(5000, 3, 8, 8));

		Variant variant("chr2", 54649982, 54649989, "GCACACAG", "ACACAC");

		Transcript t = trans_SPTBN1();
		VariantConsequence hgvs = var_hgvs_anno.annotate(t, variant);
		S_EQUAL(hgvs.hgvs_c, "c.5570_5577delinsACACAC");
		S_EQUAL(hgvs.hgvs_p, "p.Gly1857AspfsTer20");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
		S_EQUAL(variantImpactToString(hgvs.impact), "HIGH");
		I_EQUAL(hgvs.exon_number, 26);
		I_EQUAL(hgvs.intron_number, -1);
	}

	void five_prime_utr_deletion_with_shift()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(reference, VariantHgvsAnnotator::Parameters(5000, 3, 8, 8));

		Variant variant("chr2", 54526405, 54526407, "TGA", "T");

		Transcript t = trans_SPTBN1();
		VariantConsequence hgvs = var_hgvs_anno.annotate(t, variant);
		S_EQUAL(hgvs.hgvs_c, "c.-11_-10del");
		S_EQUAL(hgvs.hgvs_p, "");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));
		S_EQUAL(variantImpactToString(hgvs.impact), "MODIFIER");
		I_EQUAL(hgvs.exon_number, 2);
		I_EQUAL(hgvs.intron_number, -1);
	}

	void bug_deletion_of_exonic_bases_just_after_splice_site()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(reference, VariantHgvsAnnotator::Parameters(5000, 3, 8, 8));

		Variant variant("chr1", 92262863, 92262866, "TTGA", "-");

		Transcript t = trans_GLMN();
		VariantConsequence hgvs = var_hgvs_anno.annotate(t, variant);
		S_EQUAL(hgvs.hgvs_c, "c.1470_1473del");
		S_EQUAL(hgvs.hgvs_p, "p.Asn490LysfsTer16");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
		S_EQUAL(variantImpactToString(hgvs.impact), "HIGH");
		I_EQUAL(hgvs.exon_number, 16);
		I_EQUAL(hgvs.intron_number, -1);
	}

	void bug_insertion_just_after_splice_site()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(reference, VariantHgvsAnnotator::Parameters(5000, 3, 8, 8));

		VcfLine variant("chr2", 60553215, "C", QList<Sequence>() << "CA");

		Transcript t = trans_BCL11A();
		VariantConsequence hgvs = var_hgvs_anno.annotate(t, variant);
		S_EQUAL(hgvs.hgvs_c, "c.55_55+1insT");
		S_EQUAL(hgvs.hgvs_p, "p.Pro19LeufsTer5");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
		S_EQUAL(variantImpactToString(hgvs.impact), "HIGH");
		I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);
	}

	void bug_insertion_just_after_splice_site_case2()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(reference, VariantHgvsAnnotator::Parameters(5000, 3, 8, 8));

		VcfLine variant("chr16", 88714920, "G", QList<Sequence>() << "GGACTT");

		Transcript t = trans_CTU2();
		VariantConsequence hgvs = var_hgvs_anno.annotate(t, variant);
		S_EQUAL(hgvs.hgvs_c, "c.1415_1419dup");
		S_EQUAL(hgvs.hgvs_p, "p.Pro474ThrfsTer33");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
		S_EQUAL(variantImpactToString(hgvs.impact), "HIGH");
		I_EQUAL(hgvs.exon_number, 13);
		I_EQUAL(hgvs.intron_number, -1);
	}

	void bug_insertion_in_5_prime_UTR()
	{
		QString ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") SKIP("Test needs the reference genome!");
		FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(reference, VariantHgvsAnnotator::Parameters(5000, 3, 8, 8));

		VcfLine variant("chr9", 136363734, "C", QList<Sequence>() << "CGCCCTGCCCCG");

		Transcript t = trans_DNLZ();
		VariantConsequence hgvs = var_hgvs_anno.annotate(t, variant);
		S_EQUAL(hgvs.hgvs_c, "c.-21_-20insCGGGGCAGGGC");
		S_EQUAL(hgvs.hgvs_p, "");
		IS_TRUE(hgvs.types.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));
		S_EQUAL(variantImpactToString(hgvs.impact), "MODIFIER");
		I_EQUAL(hgvs.exon_number, 1);
		I_EQUAL(hgvs.intron_number, -1);
	}

	//TODO Marc: Error processing variant chr7:157009949 A>CGCGGCGGCG and transcript ENST00000252971.11: Coding sequence length must be multiple of three. (1 times, e.g. in DNA2206556A1_02)
	//TODO Marc: Error processing variant chr17:31229232 CGTA>TGTC: Coding sequence length must be multiple of three
	//TODO Marc: Error processing variant chr7:4781213-4781216 GGAT>TGCTGTAAACTGTAACTGTAAA: Coding sequence length must be multiple of three
};
