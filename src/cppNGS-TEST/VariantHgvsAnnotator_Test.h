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

private slots:
    void vcfToHgvsPlusStrand()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

        VariantHgvsAnnotator var_hgvs_anno;
        Transcript t = trans_SLC51A();

        //SNV exon
        QVector<Sequence> alt;
        alt.push_back("G");
        VcfLine variant(Chromosome("chr3"), 195944797, "A", alt);
        HgvsNomenclature hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.123A>G");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::CODING_SEQUENCE_VARIANT);
        /*QTextStream out(stdout);
        out << hgvs.hgvs_c << endl;*/

        //SNV intron
        alt.clear();
        alt.push_back("T");
        variant.setPos(195943667);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.38+46G>T");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::INTRON_VARIANT);

        //SNV intron
        alt.clear();
        alt.push_back("T");
        variant.setPos(195953793);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.134-43C>T");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::INTRON_VARIANT);

        //SNV 5 prime utr
        alt.clear();
        alt.push_back("G");
        variant.setPos(195943377);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-207A>G");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::FIVE_PRIME_UTR_VARIANT);

        //SNV 3 prime utr
        alt.clear();
        alt.push_back("C");
        variant.setPos(195960118);
        variant.setRef("A");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*48A>C");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::THREE_PRIME_UTR_VARIANT);
    }

    void vcfToHgvsMinusStrand()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

        VariantHgvsAnnotator var_hgvs_anno;
        Transcript t = trans_APOD();

        //QTextStream out(stdout);

        //SNV exon
        QVector<Sequence> alt;
        alt.push_back("A");
        VcfLine variant(Chromosome("chr3"), 195300788, "G", alt);
        HgvsNomenclature hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.178C>T");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::CODING_SEQUENCE_VARIANT);

        //SNV intron
        alt.clear();
        alt.push_back("T");
        variant.setPos(195297990);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.334+158G>A");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::INTRON_VARIANT);

        //SNV intron
        alt.clear();
        alt.push_back("G");
        variant.setPos(195296559);
        variant.setRef("C");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.335-553G>C");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::INTRON_VARIANT);

        //SNV 3 prime utr
        alt.clear();
        alt.push_back("T");
        variant.setPos(195295592);
        variant.setRef("G");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*179C>A");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::THREE_PRIME_UTR_VARIANT);

        //SNV 5 prime utr intron
        alt.clear();
        alt.push_back("C");
        variant.setPos(195306368);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.-34-2A>G");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::INTRON_VARIANT);
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
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::INTRON_VARIANT);

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
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::INTRON_VARIANT);

        //SNV 3 prime utr intron minus strand
        alt.clear();
        alt.push_back("G");
        variant.setPos(14988684);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*22-37A>C");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::INTRON_VARIANT);

        //SNV 3 prime utr intron minus strand
        alt.clear();
        alt.push_back("G");
        variant.setPos(14989202);
        variant.setRef("T");
        variant.setAlt(alt.toList());
        hgvs = var_hgvs_anno.variantToHgvs(t_2, variant, reference);
        S_EQUAL(hgvs.hgvs_c, "c.*21+18A>C");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::INTRON_VARIANT);
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
