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

private slots:
    void vcfToHgvsPlusStrand()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");
        FastaFileIndex reference(ref_file);

        VariantHgvsAnnotator var_hgvs_anno;
        Transcript t = trans_SLC51A();

        QVector<Sequence> alt;
        alt.push_back("G");

        VcfLine variant(Chromosome("chr3"), 195944797, "A", alt);

        HgvsNomenclature hgvs = var_hgvs_anno.variantToHgvs(t, variant, reference);
        X_EQUAL(hgvs.hgvs_c, "c.123A>G");
        X_EQUAL(hgvs.variant_consequence_type.at(0), VariantConsequenceType::CODING_SEQUENCE_VARIANT);
        /*QTextStream out(stdout);
        out << hgvs.hgvs_c << endl;*/
    }
};
