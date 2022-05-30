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
Q_OBJECT

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
        t.setSource(Transcript::ENSEMBL);
        t.setStrand(Transcript::PLUS);

        BedFile regions;
        regions.append(BedLine("chr11", 65422774, 65423383));
        regions.append(BedLine("chr11", 65423627, 65426457));
        t.setRegions(regions);

        return t;
    }

private slots:
    void vcfToHgvsPlusStrand()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(5000, 3, 8, 8);
        Transcript t = trans_SLC51A();

        //SNV exon synonymous
        QVector<Sequence> alt;
        alt.push_back("G");
        VcfLine variant(Chromosome("chr3"), 196217926, "A", alt);
        HgvsNomenclature hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.123A>G");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Gln41=");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SYNONYMOUS_VARIANT));
        I_EQUAL(hgvs.exon_number, 2);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "G");

        //SNV exon stop gained
        alt.clear();
        alt.push_back("T");
        variant.setPos(196233116);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.940C>T");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Arg314Ter");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::STOP_GAINED));
        I_EQUAL(hgvs.exon_number, 9);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "T");

        //SNV exon stop lost
        alt.clear();
        alt.push_back("C");
        variant.setPos(196233197);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.1021T>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Ter341GlnextTer7");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::STOP_LOST));
        I_EQUAL(hgvs.exon_number, 9);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "C");

        //SNV exon splice region
        alt.clear();
        alt.push_back("G");
        variant.setPos(196217844);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.41A>G");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Tyr14Cys");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::MISSENSE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        I_EQUAL(hgvs.exon_number, 2);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "G");

        //SNV intron splice acceptor
        alt.clear();
        alt.push_back("G");
        variant.setPos(196232417);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.781-2A>G");
        S_EQUAL(hgvs.hgvs_p, "p.?");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 7);
        S_EQUAL(hgvs.allele, "G");

        //SNV intron splice donor
        alt.clear();
        alt.push_back("C");
        variant.setPos(196232526);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.886+2T>C");
        S_EQUAL(hgvs.hgvs_p, "p.?");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_DONOR_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 8);
        S_EQUAL(hgvs.allele, "C");

        //SNV intron
        alt.clear();
        alt.push_back("T");
        variant.setPos(196216796);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.38+46G>T");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 1);
        S_EQUAL(hgvs.allele, "T");

        //SNV intron
        alt.clear();
        alt.push_back("T");
        variant.setPos(196226922);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.134-43C>T");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 2);
        S_EQUAL(hgvs.allele, "T");

        //SNV 5 prime utr
        alt.clear();
        alt.push_back("G");
        variant.setPos(196216594);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-119A>G");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "G");

        //SNV 3 prime utr
        alt.clear();
        alt.push_back("C");
        variant.setPos(196233247);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*48A>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 9);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "C");

        // upstream gene variant
        alt.clear();
        alt.push_back("G");
        variant.setPos(196215616);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::UPSTREAM_GENE_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "G");

        // deletion intron
        alt.clear();
        alt.push_back("C");
        variant.setPos(196217223);
        variant.setRef("CCTCT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.38+474_38+477del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 1);
        S_EQUAL(hgvs.allele, "-");

        // deletion 5 prime utr
        alt.clear();
        alt.push_back("C");
        variant.setPos(196216610);
        variant.setRef("CA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-102del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // frameshift deletion
        alt.clear();
        alt.push_back("A");
        variant.setPos(196217907);
        variant.setRef("AG");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.105del");
        S_EQUAL(hgvs.hgvs_p, "p.Gln35HisfsTer9");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 2);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // frameshift deletion
        alt.clear();
        alt.push_back("G");
        variant.setPos(196229958);
        variant.setRef("GC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.678del");
        S_EQUAL(hgvs.hgvs_p, "p.Val227CysfsTer39");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 7);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // frameshift deletion
        alt.clear();
        alt.push_back("G");
        variant.setPos(196229983);
        variant.setRef("GACCC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.703_706del");
        S_EQUAL(hgvs.hgvs_p, "p.Thr235TrpfsTer30");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 7);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // inframe deletion (shifted to the right by 3 nt according to 3 prime rule)
        alt.clear();
        alt.push_back("A");
        variant.setPos(196217846);
        variant.setRef("ACAG");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.47_49del");
        S_EQUAL(hgvs.hgvs_p, "p.Ala16del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 2);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // insertion 5 prime utr
        alt.clear();
        alt.push_back("GA");
        variant.setPos(196216604);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-109_-108insA");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "A");

        // insertion intron
        alt.clear();
        alt.push_back("TCCCAGCCG");
        variant.setPos(196221844);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.133+3908_133+3909insCCCAGCCG");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 2);
        S_EQUAL(hgvs.allele, "CCCAGCCG");

        // frameshift insertion
        alt.clear();
        alt.push_back("CGGGTGACAGAGTGACACCATCTCTTGAAAGAGAGAGAGAGAGAGAG");
        variant.setPos(196230043);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.762_763insGGGTGACAGAGTGACACCATCTCTTGAAAGAGAGAGAGAGAGAGAG");
        S_EQUAL(hgvs.hgvs_p, "p.Lys255GlyfsTer2");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        //IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::STOP_GAINED));
        I_EQUAL(hgvs.exon_number, 7);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "GGGTGACAGAGTGACACCATCTCTTGAAAGAGAGAGAGAGAGAGAG");

        // inframe duplication
        alt.clear();
        alt.push_back("CTGCTGCTGC");
        variant.setPos(196228229);
        variant.setRef("CTGCTGC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.490_492dup");
        S_EQUAL(hgvs.hgvs_p, "p.Cys164dup");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_INSERTION));

        Transcript t_2 = trans_DECR1();

        // inframe deletion
        alt.clear();
        alt.push_back("T");
        variant.setChromosome(Chromosome("chr8"));
        variant.setPos(90006261);
        variant.setRef("TCCT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.27_29del");
        S_EQUAL(hgvs.hgvs_p, "p.Leu11del"); // obtained by shifting AA to the most C-terminal position possible
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 3);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // inframe deletion
        alt.clear();
        alt.push_back("T");
        variant.setPos(90044895);
        variant.setRef("TGAT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.759_761del");
        S_EQUAL(hgvs.hgvs_p, "p.Met253del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 10);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // inframe deletion
        alt.clear();
        alt.push_back("T");
        variant.setPos(90051678);
        variant.setRef("TCATTAA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.861_866del");
        S_EQUAL(hgvs.hgvs_p, "p.Ile288_Lys289del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 11);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // inframe deletion (shifted to the right by 3 nt according to 3 prime rule)
        alt.clear();
        alt.push_back("A");
        variant.setPos(90051864);
        variant.setRef("AGAA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.952_954del");
        S_EQUAL(hgvs.hgvs_p, "p.Glu318del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 12);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // frameshift insertion
        alt.clear();
        alt.push_back("GA");
        variant.setPos(90036937);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.635_636insA");
        S_EQUAL(hgvs.hgvs_p, "p.Ser212ArgfsTer7");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 8);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "A");

        // inframe insertion
        alt.clear();
        alt.push_back("TACA");
        variant.setPos(90044968);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.831_832insACA");
        S_EQUAL(hgvs.hgvs_p, "p.Ser277_Asp278insThr");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_INSERTION));
        I_EQUAL(hgvs.exon_number, 10);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "ACA");

        // inframe insertion, also shifted by 1 nt according to 3' rule
        alt.clear();
        alt.push_back("GTCACCG");
        variant.setPos(90044967);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.831_832insCACCGT");
        S_EQUAL(hgvs.hgvs_p, "p.Ser277_Asp278insHisArg");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_INSERTION));
        I_EQUAL(hgvs.exon_number, 10);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "TCACCG");

        // inframe insertion
        alt.clear();
        alt.push_back("GACACCG");
        variant.setPos(90044967);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.830_831insACACCG");
        S_EQUAL(hgvs.hgvs_p, "p.Ser277delinsArgHisArg");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_INSERTION));
        I_EQUAL(hgvs.exon_number, 10);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "ACACCG");

        // inframe insertion
        alt.clear();
        alt.push_back("AACG");
        variant.setPos(90044966);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.829_830insACG");
        S_EQUAL(hgvs.hgvs_p, "p.Ser277delinsAsnGly");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_INSERTION));
        I_EQUAL(hgvs.exon_number, 10);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "ACG");
    }

    void vcfToHgvsMinusStrand()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(5000, 3, 8, 8);
        Transcript t = trans_APOD();
        Transcript t_CALCA = trans_CALCA();

        //SNV exon start lost
        QVector<Sequence> alt;
        alt.push_back("G");
        VcfLine variant(Chromosome("chr11"), 14971192, "T", alt);
        HgvsNomenclature hgvs = var_hgvs_anno.variantToHgvs(t_CALCA, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.1A>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Met1?");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::START_LOST));
        I_EQUAL(hgvs.exon_number, 2);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "G");

        //SNV exon missense
        alt.clear();
        alt.push_back("A");
        variant.setChromosome(Chromosome("chr3"));
        variant.setPos(195573917);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.178C>T");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Arg60Cys");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::MISSENSE_VARIANT));
        I_EQUAL(hgvs.exon_number, 3);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "A");

        //SNV exon stop gained
        alt.clear();
        alt.push_back("T");
        variant.setPos(195569086);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.384T>A");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Tyr128Ter");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::STOP_GAINED));
        I_EQUAL(hgvs.exon_number, 5);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "T");

        //SNV intron splice acceptor
        alt.clear();
        alt.push_back("G");
        variant.setPos(195573973);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.124-2A>C");
        S_EQUAL(hgvs.hgvs_p, "p.?");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 2);
        S_EQUAL(hgvs.allele, "G");

        //SNV intron splice region next to acceptor
        alt.clear();
        alt.push_back("T");
        variant.setPos(195573974);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.124-3C>A");
        S_EQUAL(hgvs.hgvs_p, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        IS_FALSE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 2);
        S_EQUAL(hgvs.allele, "T");

        //SNV intron splice donor
        alt.clear();
        alt.push_back("G");
        variant.setPos(195583876);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-35+2T>C");
        S_EQUAL(hgvs.hgvs_p, "p.?");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_DONOR_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 1);
        S_EQUAL(hgvs.allele, "G");

        //SNV intron splice region
        alt.clear();
        alt.push_back("G");
        variant.setPos(195569141);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.335-6T>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 4);
        S_EQUAL(hgvs.allele, "G");

        //SNV intron
        alt.clear();
        alt.push_back("T");
        variant.setPos(195571119);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.334+158G>A");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 4);
        S_EQUAL(hgvs.allele, "T");

        //SNV intron
        alt.clear();
        alt.push_back("G");
        variant.setPos(195569688);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.335-553G>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 4);
        S_EQUAL(hgvs.allele, "G");

        //SNV 3 prime utr
        alt.clear();
        alt.push_back("T");
        variant.setPos(195568721);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*179C>A");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 5);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "T");

        //SNV 5 prime utr intron
        alt.clear();
        alt.push_back("C");
        variant.setPos(195579497);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-34-2A>G");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 1);
        S_EQUAL(hgvs.allele, "C");

        // upstream gene variant
        alt.clear();
        alt.push_back("A");
        variant.setPos(195585307);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::UPSTREAM_GENE_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "A");

        // downstream gene variant
        alt.clear();
        alt.push_back("A");
        variant.setPos(195563767);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::DOWNSTREAM_GENE_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "A");

        // deletion upstream
        alt.clear();
        alt.push_back("T");
        variant.setPos(195584151);
        variant.setRef("TTC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::UPSTREAM_GENE_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // deletion 5 prime utr
        alt.clear();
        alt.push_back("C");
        variant.setPos(195583887);
        variant.setRef("CAA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-46_-45del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // deletion 3 prime utr
        alt.clear();
        alt.push_back("G");
        variant.setPos(195568841);
        variant.setRef("GTA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*57_*58del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 5);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // deletion splice region
        alt.clear();
        alt.push_back("C");
        variant.setPos(195571369);
        variant.setRef("CA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.246-5del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 3);
        S_EQUAL(hgvs.allele, "-");

        // frameshift deletion
        alt.clear();
        alt.push_back("G");
        variant.setPos(195568927);
        variant.setRef("GT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.542del");
        S_EQUAL(hgvs.hgvs_p, "p.Asp181AlafsTer3");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 5);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // insertion 3 prime utr
        alt.clear();
        alt.push_back("TGGGGG");
        variant.setPos(195568842);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*57_*58insCCCCC");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 5);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "GGGGG");

        // inframe duplication
        alt.clear();
        alt.push_back("GTTCTCG");
        variant.setPos(195569089);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.376_381dup");
        S_EQUAL(hgvs.hgvs_p, "p.Glu126_Asn127dup");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_INSERTION));

        // inframe insertion
        alt.clear();
        alt.push_back("GATA");
        variant.setPos(195569089);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.380_381insTAT");
        S_EQUAL(hgvs.hgvs_p, "p.Asn127_Tyr128insIle");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_INSERTION));

        Transcript t_2 = trans_CALCA();

        // frameshift deletion
        alt.clear();
        alt.push_back("C");
        variant.setChromosome(Chromosome("chr11"));
        variant.setPos(14969949);
        variant.setRef("CTCTT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.209_212del");
        S_EQUAL(hgvs.hgvs_p, "p.Gln70ArgfsTer20");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 3);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // frameshift deletion
        alt.clear();
        alt.push_back("C");
        variant.setPos(14967720);
        variant.setRef("CT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.361del");
        S_EQUAL(hgvs.hgvs_p, "p.Arg121GlyfsTer19");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 4);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // inframe deletion
        alt.clear();
        alt.push_back("C");
        variant.setPos(14971128);
        variant.setRef("CTGCCTGCCTGCAACA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.50_64del");
        S_EQUAL(hgvs.hgvs_p, "p.Leu17_Ser22delinsArg");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 2);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");

        // frameshift insertion
        alt.clear();
        alt.push_back("CACTGA");
        variant.setPos(14970026);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.135_136insTCAGT");
        S_EQUAL(hgvs.hgvs_p, "p.Glu46SerfsTer18");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 3);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "ACTGA");

        // frameshift insertion
        alt.clear();
        alt.push_back("TACTGA");
        variant.setPos(14970025);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.136_137insTCAGT");
        S_EQUAL(hgvs.hgvs_p, "p.Glu46ValfsTer18");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 3);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "ACTGA");

        // frameshift insertion
        alt.clear();
        alt.push_back("TACTGA");
        variant.setPos(14970024);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.137_138insTCAGT");
        S_EQUAL(hgvs.hgvs_p, "p.Glu46AspfsTer3");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 3);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "ACTGA");
    }

    void vcfToHgvsDelIns()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(5000, 3, 8, 8);
        Transcript t = trans_APOD();

        //delins intron
        QVector<Sequence> alt;
        alt.push_back("TTCT");
        VcfLine variant(Chromosome("chr3"), 195580367, "TC", alt);
        HgvsNomenclature hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-34-873delinsAGA");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 1);
        S_EQUAL(hgvs.allele, "TCT");

        //delins 3 prime utr
        alt.clear();
        alt.push_back("AGGCG");
        variant.setPos(195568830);
        variant.setRef("AT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*69delinsCGCC");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));
        I_EQUAL(hgvs.exon_number, 5);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "GGCG");

        //delins intron
        t = trans_SLC51A();
        alt.clear();
        alt.push_back("CGTG");
        variant.setChromosome(Chromosome("chr3"));
        variant.setPos(196219096);
        variant.setRef("GGCA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.133+1160_133+1163delinsCGTG");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 2);
        S_EQUAL(hgvs.allele, "CGTG");

        //delins exon missense
        t = trans_CDKN1C();
        alt.clear();
        alt.push_back("CT");
        variant.setChromosome(Chromosome("chr11"));
        variant.setPos(2884791);
        variant.setRef("GC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.698_699delinsAG");
        S_EQUAL(hgvs.hgvs_p, "p.Arg233Gln");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::MISSENSE_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "CT");

        //delins exon missense
        alt.clear();
        alt.push_back("AA");
        variant.setPos(2884903);
        variant.setRef("GC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.586_587delinsTT");
        S_EQUAL(hgvs.hgvs_p, "p.Ala196Leu");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::MISSENSE_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "AA");

        //delins exon; more than one amino acid deleted
        alt.clear();
        alt.push_back("AA");
        variant.setPos(2884900);
        variant.setRef("GCCGC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.586_590delinsTT");
        S_EQUAL(hgvs.hgvs_p, "p.Ala196_Ala197delinsPhe");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "AA");

        //delins exon; more than one amino acid inserted
        alt.clear();
        alt.push_back("AAATT");
        variant.setPos(2884903);
        variant.setRef("GC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.586_587delinsAATTT");
        S_EQUAL(hgvs.hgvs_p, "p.Ala196delinsAsnLeu");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_INSERTION));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "AAATT");

        //delins exon; more than one amino acid deleted and inserted
        alt.clear();
        alt.push_back("AAATT");
        variant.setPos(2884900);
        variant.setRef("GCCGC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.586_590delinsAATTT");
        S_EQUAL(hgvs.hgvs_p, "p.Ala196_Ala197delinsAsnPhe");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::MISSENSE_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "AAATT");

        //delins exon frameshift
        alt.clear();
        alt.push_back("GTT");
        variant.setPos(2884858);
        variant.setRef("GC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.631delinsAA");
        S_EQUAL(hgvs.hgvs_p, "p.Ala211AsnfsTer30");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "TT");

        //delins exon frameshift
        alt.clear();
        alt.push_back("CC");
        variant.setPos(2885178);
        variant.setRef("CAG");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.310_311delinsG");
        S_EQUAL(hgvs.hgvs_p, "p.Leu104GlyfsTer168");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "C");
    }

    void vcfToHgvsUtrIntrons()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(5000, 3, 8, 8);
        Transcript t = trans_DECR1();

        //SNV 5 prime utr intron plus strand
        QVector<Sequence> alt;
        alt.push_back("A");
        VcfLine variant(Chromosome("chr8"), 90005382, "G", alt);
        HgvsNomenclature hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-598-66G>A");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 1);
        S_EQUAL(hgvs.allele, "A");

        Transcript t_2 = trans_CALCA();

        //SNV 5 prime utr intron minus strand
        alt.clear();
        alt.push_back("G");
        variant.setChromosome(Chromosome("chr11"));
        variant.setPos(14972146);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-10+75T>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 1);
        S_EQUAL(hgvs.allele, "G");

        //SNV 3 prime utr intron minus strand
        alt.clear();
        alt.push_back("G");
        variant.setPos(14967138);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*22-37A>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 4);
        S_EQUAL(hgvs.allele, "G");

        //SNV 3 prime utr intron minus strand
        alt.clear();
        alt.push_back("G");
        variant.setPos(14967656);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*21+18A>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 4);
        S_EQUAL(hgvs.allele, "G");
    }

    void vcfToHgvsNonCoding()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

		VariantHgvsAnnotator var_hgvs_anno(5000, 3, 8, 8);
        Transcript t = trans_NEAT1();

        // non-coding intron SNV
        QVector<Sequence> alt;
        alt.push_back("T");
        VcfLine variant(Chromosome("chr11"), 65423403, "C", alt);
        HgvsNomenclature hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "n.610+20C>T");
        S_EQUAL(hgvs.hgvs_p, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        I_EQUAL(hgvs.exon_number, -1);
        I_EQUAL(hgvs.intron_number, 1);
        S_EQUAL(hgvs.allele, "T");

        // non-coding exon SNV
        alt.clear();
        alt.push_back("T");
        variant.setPos(65423327);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "n.554C>T");
        S_EQUAL(hgvs.hgvs_p, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "T");

        // non-coding exon deletion
        alt.clear();
        alt.push_back("C");
        variant.setPos(65422860);
        variant.setRef("CAG");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "n.88_89del");
        S_EQUAL(hgvs.hgvs_p, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT));
        I_EQUAL(hgvs.exon_number, 1);
        I_EQUAL(hgvs.intron_number, -1);
        S_EQUAL(hgvs.allele, "-");
    }

    void translateDnaSequence()
    {
		VariantHgvsAnnotator annotator(5000, 3, 8, 8);
        Sequence dna_seq = "TTTTTCTTATTGCTTCTCCTACTGTCTTCCTCATCGAGTAGCTATTACTAATAG"
                           "TGATGTTGCTGGCCTCCCCCACCGCATCACCAACAGCGTCGCCGACGGAGAAGG"
                           "ATTATCATAATGACTACCACAACGAATAACAAAAAGGTTGTCGTAGTGGCTGCC"
                           "GCAGCGGATGACGAAGAGGGTGGCGGAGGG";

        QString aa_seq = "PhePheLeuLeuLeuLeuLeuLeuSerSerSerSerSerSerTyrTyrTerTer"
                         "TerCysCysTrpProProProProHisHisGlnGlnArgArgArgArgArgArg"
                         "IleIleIleMetThrThrThrThrAsnAsnLysLysValValValValAlaAla"
                         "AlaAlaAspAspGluGluGlyGlyGlyGly";

        S_EQUAL(annotator.translate(dna_seq, false, false), aa_seq);

        dna_seq = "TTTTTCTTATTGCTTCTCCTACTGTCTTCCTCATCGAGTAGCTATTACTAA";
        aa_seq = "PhePheLeuLeuLeuLeuLeuLeuSerSerSerSerSerSerTyrTyrTer";

        S_EQUAL(annotator.translate(dna_seq, false, true), aa_seq);
    }
};
