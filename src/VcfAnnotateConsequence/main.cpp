#include "ToolBase.h"
#include "Settings.h"
#include "NGSHelper.h"
#include "VcfFile.h"
#include "VariantHgvsAnnotator.h"

class ConcreteTool
        : public ToolBase
{
    Q_OBJECT

private:
    void annotateVcfFile(const QString& in_file, const QString& out_file, const ChromosomalIndex<TranscriptList>& transcript_index,
                         const FastaFileIndex& ref_file, const int& max_dist_to_trans, const int& splice_region_ex, const int& splice_region_in)
    {
        TranscriptList transcripts = transcript_index.container();

        VcfFile vcf;
        vcf.load(in_file);

        VariantHgvsAnnotator hgvs_anno(max_dist_to_trans, splice_region_ex, splice_region_in);


        for(int i = 0; i < vcf.vcfHeader().infoLines().count(); i++)
        {
            if(vcf.vcfHeader().infoLines().at(i).id == "CSQ")
            {
                vcf.vcfHeader().removeInfoLine(i);
            }
        }

        //add info line for CSQ
        InfoFormatLine csq_line;
        csq_line.id = "CSQ";
        csq_line.number = ".";
        csq_line.type = "String";
        csq_line.description = "Consequence annotations from VcfAnnotateConsequence. Format: Allele|Consequence|IMPACT|SYMBOL|"
                               "HGNC_ID|Feature|Feature_type|BIOTYPE|EXON|INTRON|HGVSc|HGVSp";
        vcf.vcfHeader().addInfoLine(csq_line);

        //annotate all variants
        foreach(VcfLinePtr variant, vcf.vcfLines())
        {
            //skip structural variants, e.g. <DEL>, <INS>
            if(variant->alt(0).startsWith("<"))
            {
                continue;
            }

            //get all transcripts where the variant is completely contained in the region
            int region_start = std::max(variant->end() - max_dist_to_trans, 0);
            int region_end = variant->start() + max_dist_to_trans;

            QVector<int> indices = transcript_index.matchingIndices(variant->chr(), region_start, region_end);

            QByteArray csq;

            //no transcripts in proximity: intergenic variant
            if(indices.isEmpty())
            {
                HgvsNomenclature hgvs;
                if(variant->isSNV()) hgvs.allele = variant->alt(0);
                else if(variant->isDel()) hgvs.allele = "-";
                else
                {
                    variant->normalize(VcfLine::ShiftDirection::RIGHT, ref_file);
                    hgvs.allele = variant->alt(0).mid(1);
                }
                hgvs.variant_consequence_type.insert(VariantConsequenceType::INTERGENIC_VARIANT);
                Transcript t;
                csq.append(hgvsNomenclatureToString(hgvs, t));
            }

            //hgvs annotation for each transcript in proximity to variant
            foreach(int idx, indices)
            {
                Transcript t = transcripts.at(idx);

                //create new VcfLine for annotation (don't change original variant coordinates by normalization!)
                VcfLine var_for_anno = VcfLine(variant->chr(), variant->start(), variant->ref(), variant->alt());

                HgvsNomenclature hgvs;
                try
                {
                    hgvs = hgvs_anno.variantToHgvs(t, var_for_anno, ref_file);
                }
                catch(Exception& e)
                {
                    QTextStream out(stdout);
                    out << "Variant out of region for transcript " << t.name() <<", chromsome " << t.chr().str()
                        << "(" << t.start() << " - " << t.end();
                    out << "Variant start: " << variant->start() << "; end: " << variant->end() << endl;
                    out << "Considered region:" << region_start << " - " << region_end << endl;
                }
                csq.append(hgvsNomenclatureToString(hgvs, t));
                if(idx != indices[indices.size()-1]) csq.append(",");
            }

            //add CSQ to INFO field; keep all other infos
            QByteArrayList info;
            InfoIDToIdxPtr info_ptr = InfoIDToIdxPtr(new OrderedHash<QByteArray, int>);

            bool csq_contained = false;

            for(int i = 0; i < variant->infoKeys().size(); i++)
            {
                QByteArray key = variant->infoKeys().at(i);
                if(key == "CSQ") csq_contained=true;
                info_ptr->push_back(key, i);
                info.append(variant->info(key));
            }

            if(csq_contained)
            {
                int csq_idx = info_ptr->operator []("CSQ");
                info[csq_idx] = csq;
            }
            else
            {
                info_ptr->push_back("CSQ", info_ptr->size());
                info.append(csq);
            }

            variant->setInfo(info);
            variant->setInfoIdToIdxPtr(info_ptr);
        }

        vcf.store(out_file);
    }

    void annotateVcfStream(const QString& in_file, const QString& out_file, const ChromosomalIndex<TranscriptList>& transcript_index,
                           const FastaFileIndex& ref_file, const int& max_dist_to_trans, const int& splice_region_ex, const int& splice_region_in)
    {
        TranscriptList transcripts = transcript_index.container();
    }

    QString hgvsNomenclatureToString(const HgvsNomenclature& hgvs, const Transcript& t)
    {
        QString csq = hgvs.allele;

        //find variant consequence type with highest priority
        VariantConsequenceType max_csq_type = VariantConsequenceType::INTERGENIC_VARIANT;
        foreach(VariantConsequenceType csq_type, hgvs.variant_consequence_type)
        {
            if(csq_type > max_csq_type)
            {
                max_csq_type = csq_type;
            }
        }
        csq.append("|" + hgvs.consequenceTypeToString(max_csq_type) + "|");

        //add variant impact
        VariantImpact impact = hgvs.consequenceTypeToImpact(max_csq_type);
        if(impact == VariantImpact::HIGH) csq.append("HIGH|");
        else if(impact == VariantImpact::MODERATE) csq.append("MODERATE|");
        else if(impact == VariantImpact::LOW) csq.append("LOW|");
        else csq.append("MODIFIER|");

        //gene symbol, HGNC ID, transcript ID
        csq.append(t.gene() + "|" + t.hgncId() + "|" + t.name());
        if(hgvs.transcript_id == "") csq.append("||");
        else csq.append("|Transcript|");

        //biotype
        if(t.isCoding()) csq.append("protein_coding|");
        else csq.append("processed_transcript|");

        //exon number
        if(hgvs.exon_number != -1) csq.append(QString::number(hgvs.exon_number) + "/" + QString::number(t.regions().count()) + "|");
        else csq.append("|");

        //intron number
        if(hgvs.intron_number != -1) csq.append(QString::number(hgvs.intron_number) + "/" + QString::number(t.regions().count() - 1) + "|");
        else csq.append("|");

        csq.append(hgvs.hgvs_c + "|" + hgvs.hgvs_p);

        return csq;
    }

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
    }

    virtual void setup()
    {
        setDescription("Adds variant effect predictions to a VCF file.");
        addInfile("in", "Input VCF file.", false);
        addInfile("gff", "GFF file with all interesting transcripts.", false);
        addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
        addOutfile("out", "Output VCF file containing predicted consequences for each variant.", false);

        //optional
        addFlag("all", "If set, all transcripts are imported (the default is to skip transcripts not labeled as with the 'GENCODE basic' tag).");
        addFlag("stream", "Allows to stream the input and output VCF without loading the whole file into memory. Only supported with uncompressed VCF files.");
        addInt("dist-to-trans", "maximum distance between variant and transcript", true, 5000);
        addInt("splice-region-ex", "number of bases at exon boundaries that are considered to be in the splice region", true, 3);
        addInt("splice-region-in", "number of bases at intron boundaries that are considered to be in the splice region", true, 8);
    }

    virtual void main()
    {
        // init
        //open refererence genome file
        QString ref_file = getInfile("ref");
        if (ref_file=="") ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
        FastaFileIndex reference(ref_file);

        QString in_file = getInfile("in");
        QString gff_file = getInfile("gff");
        QString out_file = getOutfile("out");
        bool all = getFlag("all");
        bool stream = getFlag("stream");

        int max_dist_to_trans = getInt("dist-to-trans");
        int splice_region_ex = getInt("splice-region-ex");
        int splice_region_in = getInt("splice-region-in");

        //parse gff file
        QMap<QByteArray, QByteArray> transcript_gene_relation;
        QMap<QByteArray, QByteArray> gene_name_relation;
        QList<TranscriptData> t_data_list = NGSHelper::loadGffFile(gff_file, transcript_gene_relation, gene_name_relation, all);

        TranscriptList transcripts;
        foreach(TranscriptData t_data, t_data_list)
        {
            Transcript t;
            t.setName(t_data.name);
            t.setGene(t_data.gene_symbol);
            t.setHgncId(t_data.hgnc_id);
            t.setSource(Transcript::ENSEMBL);
            t.setStrand(t_data.strand == "+" ? Transcript::PLUS : Transcript::MINUS);
            int start_coding = 0;
            int end_coding = 0;
            if(t_data.start_coding != -1 && t_data.end_coding != -1)
            {
                start_coding = t_data.start_coding;
                end_coding = t_data.end_coding;

                if(t.strand() == Transcript::MINUS)
                {
                   int temp = start_coding;
                   start_coding = end_coding;
                   end_coding = temp;
                }
            }
            t.setRegions(t_data.exons, start_coding, end_coding);
            transcripts.append(t);
        }

        transcripts.sortByPosition();
        ChromosomalIndex<TranscriptList> transcript_index(transcripts);

        if(stream)
        {
            annotateVcfStream(in_file, out_file, transcript_index, ref_file, max_dist_to_trans, splice_region_ex, splice_region_in);
        }
        else
        {
            annotateVcfFile(in_file, out_file, transcript_index, ref_file, max_dist_to_trans, splice_region_ex, splice_region_in);
        }
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
