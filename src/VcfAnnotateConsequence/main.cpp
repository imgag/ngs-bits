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
    void annotateVcfStream(const QString& in_file, const QString& out_file, const ChromosomalIndex<TranscriptList>& transcript_index,
                           const FastaFileIndex& reference, const int& max_dist_to_trans, const int& splice_region_ex,
                           const int& splice_region_in_5, const int& splice_region_in_3, const QByteArray& tag)
    {
        if(in_file != "" && in_file == out_file)
        {
            THROW(ArgumentException, "Input and output files must be different when streaming!");
        }
        QSharedPointer<QFile> reader = Helper::openFileForReading(in_file, true);
        QSharedPointer<QFile> writer = Helper::openFileForWriting(out_file, true);

        bool csq_line_inserted = false;

        TranscriptList transcripts = transcript_index.container();
        VariantHgvsAnnotator hgvs_anno(max_dist_to_trans, splice_region_ex, splice_region_in_5, splice_region_in_3);

        while(!reader->atEnd())
        {
            QByteArray line = reader->readLine();

            //skip empty lines
            if(line.trimmed().isEmpty()) continue;

            //write out headers unchanged (unless they are the CSQ description)
            if(line.startsWith("##"))
            {
                if(!line.contains(tag))
                {
                    writer->write(line);
                    continue;
                }
            }

            //add the CSQ info line after all other header lines
            if(!csq_line_inserted)
            {
                writer->write("##INFO=<ID=" + tag + ",Number=.,Type=String,Description=\"Consequence annotations from VcfAnnotateConsequence. "
                              "Format: Allele|Consequence|IMPACT|SYMBOL|HGNC_ID|Feature|Feature_type|BIOTYPE|EXON|INTRON|HGVSc|HGVSp\">\n");
                csq_line_inserted = true;
            }

            //column header line
            if(line.startsWith("#"))
            {
                writer->write(line);
                continue;
            }

            //split line and extract variant infos
            line = line.trimmed();
            QList<QByteArray> parts = line.split('\t');
            if(parts.count() < 8) THROW(FileParseException, "VCF with too few columns: " + line);

            Chromosome chr = parts[0];
            int pos = atoi(parts[1]);
            Sequence ref = parts[3].toUpper();
            Sequence alt = parts[4].toUpper();

            //write out multi-allelic and structural (e.g. <DEL>) variants without CSQ annotation
            if(alt.contains(',') || alt.startsWith("<"))
            {
                writeLine(writer, parts, pos, ref, alt, parts[7]);
                continue;
            }

            VcfLine variant = VcfLine(chr, pos, ref, QVector<Sequence>() << alt);

            //get all transcripts where the variant is completely contained in the region
            int region_start = std::max(variant.end() - max_dist_to_trans, 0);
            int region_end = variant.start() + max_dist_to_trans;

            QVector<int> indices = transcript_index.matchingIndices(variant.chr(), region_start, region_end);

            QByteArray csq;

            //no transcripts in proximity: intergenic variant
            if(indices.isEmpty())
            {
                //treat each alternative allele of multi-allelic variants as a new variant
                foreach(Sequence alt, variant.alt())
                {
                    VcfLine var_for_anno = VcfLine(variant.chr(), variant.start(), variant.ref(), QVector<Sequence>() << alt);
                    HgvsNomenclature hgvs;
                    if(var_for_anno.isSNV()) hgvs.allele = var_for_anno.alt(0);
                    else if(var_for_anno.isDel()) hgvs.allele = "-";
                    else
                    {
                        if(var_for_anno.alt(0).at(0) == var_for_anno.ref().at(0))
                        {
                            hgvs.allele = var_for_anno.alt(0).mid(1);
                        }
                        else
                        {
                            hgvs.allele = var_for_anno.alt(0);
                        }
                    }
                    hgvs.variant_consequence_type.insert(VariantConsequenceType::INTERGENIC_VARIANT);
                    Transcript t;
                    csq.append(hgvsNomenclatureToString(hgvs, t));
                }
            }

            //hgvs annotation for each transcript in proximity to variant
            foreach(int idx, indices)
            {
                Transcript t = transcripts.at(idx);

                //treat each alternative allele of multi-allelic variants as a new variant
                foreach(Sequence alt, variant.alt())
                {
                    //create new VcfLine for annotation (don't change original variant coordinates by normalization!)
                    VcfLine var_for_anno = VcfLine(variant.chr(), variant.start(), variant.ref(), QVector<Sequence>() << alt);

                    HgvsNomenclature hgvs;
                    try
                    {
                        hgvs = hgvs_anno.variantToHgvs(t, var_for_anno, reference);
                        if(!csq.isEmpty()) csq.append(",");
                        csq.append(hgvsNomenclatureToString(hgvs, t));
                    }
                    catch(ArgumentException& e)
                    {
                        QTextStream out(stdout);
                        out << e.message() << endl;
                        out << "Variant out of region for transcript " << t.name() <<", chromosome " << t.chr().str()
                            << " (" << t.start() << " - " << t.end() << "); ";
                        out << "Variant start: " << variant.start() << "; end: " << variant.end()
                            << "; start of shifted variant: " << var_for_anno.start() << "; end: " << var_for_anno.end() << endl;
                        out << "Considered region: " << region_start << " - " << region_end << endl;
                    }
                }
            }

            //add CSQ to INFO; keep all other infos
            QByteArrayList info_list;
            if(parts[7] != "" && parts[7] != ".")
            {
                info_list = parts[7].split(';');
            }
            int csq_idx = info_list.indexOf(tag);
            if(csq_idx != -1)
            {
                info_list[csq_idx] = tag + "=" + csq;
            }
            else
            {
                info_list.append(tag + "=" + csq);
            }
            writeLine(writer, parts, pos, ref, alt, info_list.join(';'));
        }
    }

    void writeLine(QSharedPointer<QFile>& writer, const QList<QByteArray>& parts, int pos, const QByteArray& ref, const QByteArray& alt, const QByteArray& info)
    {
        char tab = '\t';
        writer->write(parts[0]);
        writer->write(&tab, 1);
        writer->write(QByteArray::number(pos));
        writer->write(&tab, 1);
        writer->write(parts[2]);
        writer->write(&tab, 1);
        writer->write(ref);
        writer->write(&tab, 1);
        writer->write(alt);
        writer->write(&tab, 1);
        writer->write(parts[5]);
        writer->write(&tab, 1);
        writer->write(parts[6]);
        writer->write(&tab, 1);
        writer->write(info);
        if(parts.count() > 8)
        {
            for(int i=8; i<parts.count(); ++i)
            {
                writer->write(&tab, 1);
                writer->write(parts[i]);
            }
        }
        writer->write("\n");
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
        else if(t.isValid()) csq.append("processed_transcript|");
        else csq.append("|");

        //exon number
        if(hgvs.exon_number != -1) csq.append(QString::number(hgvs.exon_number) + "/" + QString::number(t.regions().count()) + "|");
        else csq.append("|");

        //intron number
        if(hgvs.intron_number != -1) csq.append(QString::number(hgvs.intron_number) + "/" + QString::number(t.regions().count() - 1) + "|");
        else csq.append("|");

        csq.append(hgvs.hgvs_c + "|");
        QString hgvs_p = hgvs.hgvs_p;
        csq.append(hgvs_p.replace("=", "%3D"));

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
        addOutfile("out", "Output VCF file annotated with predicted consequences for each variant.", false);

        //optional
        addFlag("all", "If set, all transcripts are imported (the default is to skip transcripts not labeled with the 'GENCODE basic' tag).");
        addString("tag", "tag that is used for the consequence annotation", true, "CSQ");
        addInt("dist-to-trans", "maximum distance between variant and transcript", true, 5000);
        addInt("splice-region-ex", "number of bases at exon boundaries that are considered to be in the splice region", true, 3);
        addInt("splice-region-in-5", "number of bases at intron boundaries (5') that are considered to be in the splice region", true, 8);
        addInt("splice-region-in-3", "number of bases at intron boundaries (5') that are considered to be in the splice region", true, 8);
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
        QByteArray tag = getString("tag").toUtf8();

        int max_dist_to_trans = getInt("dist-to-trans");
        int splice_region_ex = getInt("splice-region-ex");
        int splice_region_in_5 = getInt("splice-region-in-5");
        int splice_region_in_3 = getInt("splice-region-in-3");

        if(max_dist_to_trans <= 0 || splice_region_ex <= 0 || splice_region_in_5 <= 0 || splice_region_in_3 <= 0)
        {
            THROW(CommandLineParsingException, "Distance to transcript and splice region parameters must be >= 1!");
        }

        //parse gff file
        QMap<QByteArray, QByteArray> transcript_gene_relation;
        QMap<QByteArray, QByteArray> gene_name_relation;
        TranscriptList transcripts = NGSHelper::loadGffFile(gff_file, transcript_gene_relation, gene_name_relation, all);

        transcripts.sortByPosition();
        ChromosomalIndex<TranscriptList> transcript_index(transcripts);

        annotateVcfStream(in_file, out_file, transcript_index, reference, max_dist_to_trans,
                          splice_region_ex, splice_region_in_5, splice_region_in_3, tag);
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
