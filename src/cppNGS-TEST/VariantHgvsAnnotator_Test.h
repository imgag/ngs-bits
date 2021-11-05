#include "TestFramework.h"
#include "VariantHgvsAnnotator.h"
#include "Transcript.h"
#include "VcfFile.h"
#include "Sequence.h"
#include "Settings.h"
#include <QTextStream>
#include <QVector>

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

        t.setGene("APOD");
        t.setName("ENST00000343267"); //NM_001647
        t.setSource(Transcript::ENSEMBL);
        t.setStrand(Transcript::MINUS);

        BedFile regions;
        regions.append(BedLine("chr3", 195295573, 195296006));
        regions.append(BedLine("chr3", 195298148, 195298236));
        regions.append(BedLine("chr3", 195300721, 195300842));
        regions.append(BedLine("chr3", 195306210, 195306366));
        regions.append(BedLine("chr3", 195310749, 195311076));
        t.setRegions(regions, 195306332, 195295771);

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
        regions.append(BedLine("chr8", 91013705, 91013789));
        regions.append(BedLine("chr8", 91017676, 91018073));
        regions.append(BedLine("chr8", 91018264, 91018505));
        regions.append(BedLine("chr8", 91029352, 91029554));
        regions.append(BedLine("chr8", 91031137, 91031194));
        regions.append(BedLine("chr8", 91031314, 91031400));
        regions.append(BedLine("chr8", 91033137, 91033284));
        regions.append(BedLine("chr8", 91049069, 91049168));
        regions.append(BedLine("chr8", 91054956, 91055028));
        regions.append(BedLine("chr8", 91057077, 91057223));
        regions.append(BedLine("chr8", 91063905, 91063967));
        regions.append(BedLine("chr8", 91064066, 91064227));
        t.setRegions(regions, 91018464, 91064125);

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
        regions.append(BedLine("chr11", 14988215, 14988647));
        regions.append(BedLine("chr11", 14989220, 14989400));
        regions.append(BedLine("chr11", 14991481, 14991621));
        regions.append(BedLine("chr11", 14992653, 14992747));
        regions.append(BedLine("chr11", 14993767, 14993832));
        t.setRegions(regions, 14992738, 14989241);

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
        regions.append(BedLine("chr11", 65190245, 65190854));
        regions.append(BedLine("chr11", 65191098, 65192232));
        t.setRegions(regions);

        return t;
    }

private slots:
    void vcfToHgvsPlusStrand()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

        VariantHgvsAnnotator var_hgvs_anno;
        Transcript t = trans_SLC51A();

        //SNV exon synonymous
        QVector<Sequence> alt;
        alt.push_back("G");
        VcfLine variant(Chromosome("chr3"), 195944797, "A", alt);
        HgvsNomenclature hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.123A>G");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Gln41=");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SYNONYMOUS_VARIANT));

        //SNV exon stop gained
        alt.clear();
        alt.push_back("T");
        variant.setPos(195959987);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.940C>T");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Arg314Ter");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::STOP_GAINED));

        //SNV exon stop lost
        alt.clear();
        alt.push_back("C");
        variant.setPos(195960068);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.1021T>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Ter341Gln");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::STOP_LOST));

        //SNV exon splice region
        alt.clear();
        alt.push_back("G");
        variant.setPos(195944715);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.41A>G");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Tyr14Cys");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::MISSENSE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));

        //SNV intron splice acceptor
        alt.clear();
        alt.push_back("G");
        variant.setPos(195959288);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.781-2A>G");
        S_EQUAL(hgvs.hgvs_p, "p.?");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT));

        //SNV intron splice donor
        alt.clear();
        alt.push_back("C");
        variant.setPos(195959397);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.886+2T>C");
        S_EQUAL(hgvs.hgvs_p, "p.?");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_DONOR_VARIANT));

        //SNV intron
        alt.clear();
        alt.push_back("T");
        variant.setPos(195943667);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.38+46G>T");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));

        //SNV intron
        alt.clear();
        alt.push_back("T");
        variant.setPos(195953793);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.134-43C>T");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));

        //SNV 5 prime utr
        alt.clear();
        alt.push_back("G");
        variant.setPos(195943377);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-207A>G");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));

        //SNV 3 prime utr
        alt.clear();
        alt.push_back("C");
        variant.setPos(195960118);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*48A>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));

        // upstream gene variant
        alt.clear();
        alt.push_back("G");
        variant.setPos(195942487);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::UPSTREAM_GENE_VARIANT));

        // deletion intron
        alt.clear();
        alt.push_back("C");
        variant.setPos(195944094);
        variant.setRef("CCTCT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.38+474_38+477del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));

        // deletion 5 prime utr
        alt.clear();
        alt.push_back("C");
        variant.setPos(195943481);
        variant.setRef("CA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-102del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));

        // frameshift deletion
        alt.clear();
        alt.push_back("A");
        variant.setPos(195944778);
        variant.setRef("AG");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.105del");
        S_EQUAL(hgvs.hgvs_p, "p.Gln35fs");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));

        // frameshift deletion
        alt.clear();
        alt.push_back("G");
        variant.setPos(195956829);
        variant.setRef("GC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.678del");
        S_EQUAL(hgvs.hgvs_p, "p.Val227fs");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));

        // frameshift deletion
        alt.clear();
        alt.push_back("G");
        variant.setPos(195956854);
        variant.setRef("GACCC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.703_706del");
        S_EQUAL(hgvs.hgvs_p, "p.Thr235fs");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));

        // inframe deletion (shifted to the right by 3 nt according to 3 prime rule)
        alt.clear();
        alt.push_back("A");
        variant.setPos(195944717);
        variant.setRef("ACAG");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        //S_EQUAL(hgvs.hgvs_c, "c.47_49del");
        S_EQUAL(hgvs.hgvs_p, "p.Ala16del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));

        Transcript t_2 = trans_DECR1();

        // inframe deletion
        alt.clear();
        alt.push_back("T");
        variant.setChromosome(Chromosome("chr8"));
        variant.setPos(91018489);
        variant.setRef("TCCT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.27_29del");
        S_EQUAL(hgvs.hgvs_p, "p.Leu11del"); // obtained by shifting AA to the most C-terminal position possible
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));

        // inframe deletion
        alt.clear();
        alt.push_back("T");
        variant.setPos(91057123);
        variant.setRef("TGAT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.759_761del");
        S_EQUAL(hgvs.hgvs_p, "p.Met253del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));

        // inframe deletion
        alt.clear();
        alt.push_back("T");
        variant.setPos(91063906);
        variant.setRef("TCATTAA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.861_866del");
        S_EQUAL(hgvs.hgvs_p, "p.Ile288_Lys289del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));

        // inframe deletion (shifted to the right by 3 nt according to 3 prime rule)
        alt.clear();
        alt.push_back("A");
        variant.setPos(91064092);
        variant.setRef("AGAA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        //S_EQUAL(hgvs.hgvs_c, "c.952_954del");
        S_EQUAL(hgvs.hgvs_p, "p.Glu318del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));
    }

    void vcfToHgvsMinusStrand()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

        VariantHgvsAnnotator var_hgvs_anno;
        Transcript t = trans_APOD();
        Transcript t_CALCA = trans_CALCA();

        //SNV exon start lost
        QVector<Sequence> alt;
        alt.push_back("G");
        VcfLine variant(Chromosome("chr11"), 14992738, "T", alt);
        HgvsNomenclature hgvs = var_hgvs_anno.variantToHgvs(t_CALCA, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.1A>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Met1?");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::START_LOST));

        //SNV exon missense
        alt.clear();
        alt.push_back("A");
        variant.setChromosome(Chromosome("chr3"));
        variant.setPos(195300788);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.178C>T");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Arg60Cys");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::MISSENSE_VARIANT));

        //SNV exon stop gained
        alt.clear();
        alt.push_back("T");
        variant.setPos(195295957);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.384T>A");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        S_EQUAL(hgvs.hgvs_p, "p.Tyr128Ter");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::STOP_GAINED));

        //SNV intron splice acceptor
        alt.clear();
        alt.push_back("T");
        variant.setPos(195296008);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.335-2T>A");
        S_EQUAL(hgvs.hgvs_p, "p.?");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT));

        //SNV intron splice donor
        alt.clear();
        alt.push_back("G");
        variant.setPos(195310747);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-35+2T>C");
        S_EQUAL(hgvs.hgvs_p, "p.?");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_DONOR_VARIANT));

        //SNV intron splice region
        alt.clear();
        alt.push_back("G");
        variant.setPos(195296012);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.335-6T>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));

        //SNV intron
        alt.clear();
        alt.push_back("T");
        variant.setPos(195297990);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.334+158G>A");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));

        //SNV intron
        alt.clear();
        alt.push_back("G");
        variant.setPos(195296559);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.335-553G>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));

        //SNV 3 prime utr
        alt.clear();
        alt.push_back("T");
        variant.setPos(195295592);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*179C>A");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));

        //SNV 5 prime utr intron
        alt.clear();
        alt.push_back("C");
        variant.setPos(195306368);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-34-2A>G");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));

        // upstream gene variant
        alt.clear();
        alt.push_back("A");
        variant.setPos(195312178);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::UPSTREAM_GENE_VARIANT));

        // downstream gene variant
        alt.clear();
        alt.push_back("A");
        variant.setPos(195290637);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::DOWNSTREAM_GENE_VARIANT));

        // deletion 5 prime utr
        alt.clear();
        alt.push_back("T");
        variant.setPos(195311022);
        variant.setRef("TTC");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-310_-309del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FIVE_PRIME_UTR_VARIANT));

        // deletion 3 prime utr
        alt.clear();
        alt.push_back("G");
        variant.setPos(195295712);
        variant.setRef("GTA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*57_*58del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::THREE_PRIME_UTR_VARIANT));

        // deletion splice region
        alt.clear();
        alt.push_back("C");
        variant.setPos(195298240);
        variant.setRef("CA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.246-5del");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::SPLICE_REGION_VARIANT));

        // frameshift deletion
        alt.clear();
        alt.push_back("G");
        variant.setPos(195295798);
        variant.setRef("GT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.542del");
        S_EQUAL(hgvs.hgvs_p, "p.Asp181fs");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));

        Transcript t_2 = trans_CALCA();

        // frameshift deletion
        alt.clear();
        alt.push_back("C");
        variant.setChromosome(Chromosome("chr11"));
        variant.setPos(14991495);
        variant.setRef("CTCTT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.209_212del");
        S_EQUAL(hgvs.hgvs_p, "p.Gln70fs");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));

        // frameshift deletion
        alt.clear();
        alt.push_back("C");
        variant.setPos(14989266);
        variant.setRef("CT");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.361del");
        S_EQUAL(hgvs.hgvs_p, "p.Arg121fs");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::FRAMESHIFT_VARIANT));

        // inframe deletion
        alt.clear();
        alt.push_back("C");
        variant.setPos(14992674);
        variant.setRef("CTGCCTGCCTGCAACA");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.50_64del");
        S_EQUAL(hgvs.hgvs_p, "p.Leu17_Ser22delinsArg");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::CODING_SEQUENCE_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::PROTEIN_ALTERING_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INFRAME_DELETION));
    }

    void vcfToHgvsUtrIntrons()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

        VariantHgvsAnnotator var_hgvs_anno;
        Transcript t = trans_DECR1();

        //SNV 5 prime utr intron plus strand
        QVector<Sequence> alt;
        alt.push_back("A");
        VcfLine variant(Chromosome("chr8"), 91017610, "G", alt);
        HgvsNomenclature hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-598-66G>A");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));

        Transcript t_2 = trans_CALCA();

        //SNV 5 prime utr intron minus strand
        alt.clear();
        alt.push_back("G");
        variant.setChromosome(Chromosome("chr11"));
        variant.setPos(14993692);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-10+75T>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));

        //SNV 3 prime utr intron minus strand
        alt.clear();
        alt.push_back("G");
        variant.setPos(14988684);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*22-37A>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));

        //SNV 3 prime utr intron minus strand
        alt.clear();
        alt.push_back("G");
        variant.setPos(14989202);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*21+18A>C");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));
    }

    void vcfToHgvsNonCoding()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

        VariantHgvsAnnotator var_hgvs_anno;
        Transcript t = trans_NEAT1();

        // non-coding intron SNV
        QVector<Sequence> alt;
        alt.push_back("T");
        VcfLine variant(Chromosome("chr11"), 65190874, "C", alt);
        HgvsNomenclature hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "n.610+20C>T");
        S_EQUAL(hgvs.hgvs_p, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::INTRON_VARIANT));

        // non-coding exon SNV
        alt.clear();
        alt.push_back("T");
        variant.setPos(65190798);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "n.554C>T");
        S_EQUAL(hgvs.hgvs_p, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT));

        // non-coding exon deletion
        alt.clear();
        alt.push_back("C");
        variant.setPos(65190331);
        variant.setRef("CAG");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "n.88_89del");
        S_EQUAL(hgvs.hgvs_p, "");
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT));
        IS_TRUE(hgvs.variant_consequence_type.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT));
    }

    void translateDnaSequence()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

        VariantHgvsAnnotator annotator;
        Transcript t_1 = trans_SLC51A();
        QString aa_seq_1("MetGluProGlyArgThrGlnIleLysLeuAspProArgTyrThrAlaAspLeuLeuGlu"
                         "ValLeuLysThrAsnTyrGlyIleProSerAlaCysPheSerGlnProProThrAlaAla"
                         "GlnLeuLeuArgAlaLeuGlyProValGluLeuAlaLeuThrSerIleLeuThrLeuLeu"
                         "AlaLeuGlySerIleAlaIlePheLeuGluAspAlaValTyrLeuTyrLysAsnThrLeu"
                         "CysProIleLysArgArgThrLeuLeuTrpLysSerSerAlaProThrValValSerVal"
                         "LeuCysCysPheGlyLeuTrpIleProArgSerLeuValLeuValGluMetThrIleThr"
                         "SerPheTyrAlaValCysPheTyrLeuLeuMetLeuValMetValGluGlyPheGlyGly"
                         "LysGluAlaValLeuArgThrLeuArgAspThrProMetMetValHisThrGlyProCys"
                         "CysCysCysCysProCysCysProArgLeuLeuLeuThrArgLysLysLeuGlnLeuLeu"
                         "MetLeuGlyProPheGlnTyrAlaPheLeuLysIleThrLeuThrLeuValGlyLeuPhe"
                         "LeuValProAspGlyIleTyrAspProAlaAspIleSerGluGlySerThrAlaLeuTrp"
                         "IleAsnThrPheLeuGlyValSerThrLeuLeuAlaLeuTrpThrLeuGlyIleIleSer"
                         "ArgGlnAlaArgLeuHisLeuGlyGluGlnAsnMetGlyAlaLysPheAlaLeuPheGln"
                         "ValLeuLeuIleLeuThrAlaLeuGlnProSerIlePheSerValLeuAlaAsnGlyGly"
                         "GlnIleAlaCysSerProProTyrSerSerLysThrArgSerGlnValMetAsnCysHis"
                         "LeuLeuIleLeuGluThrPheLeuMetThrValLeuThrArgMetTyrTyrArgArgLys"
                         "AspHisLysValGlyTyrGluThrPheSerSerProAspLeuAspLeuAsnLeuLysAlaTer");
        S_EQUAL(annotator.translate(annotator.getCodingSequence(t_1, reference)), aa_seq_1);

        Transcript t_2 = trans_APOD();
        QString aa_seq_2("MetValMetLeuLeuLeuLeuLeuSerAlaLeuAlaGlyLeuPheGlyAlaAlaGluGly"
                         "GlnAlaPheHisLeuGlyLysCysProAsnProProValGlnGluAsnPheAspValAsn"
                         "LysTyrLeuGlyArgTrpTyrGluIleGluLysIleProThrThrPheGluAsnGlyArg"
                         "CysIleGlnAlaAsnTyrSerLeuMetGluAsnGlyLysIleLysValLeuAsnGlnGlu"
                         "LeuArgAlaAspGlyThrValAsnGlnIleGluGlyGluAlaThrProValAsnLeuThr"
                         "GluProAlaLysLeuGluValLysPheSerTrpPheMetProSerAlaProTyrTrpIle"
                         "LeuAlaThrAspTyrGluAsnTyrAlaLeuValTyrSerCysThrCysIleIleGlnLeu"
                         "PheHisValAspPheAlaTrpIleLeuAlaArgAsnProAsnLeuProProGluThrVal"
                         "AspSerLeuLysAsnIleLeuThrSerAsnAsnIleAspValLysLysMetThrValThr"
                         "AspGlnValAsnCysProLysLeuSerTer");
        S_EQUAL(annotator.translate(annotator.getCodingSequence(t_2, reference)), aa_seq_2);
    }
};
