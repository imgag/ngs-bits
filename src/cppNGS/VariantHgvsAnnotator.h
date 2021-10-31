#ifndef VARIANTHGVSANNOTATOR_H
#define VARIANTHGVSANNOTATOR_H

#include "cppNGS_global.h"
#include "BedFile.h"
#include "VariantList.h"
#include "VcfFile.h"
#include "FastaFileIndex.h"
#include "Transcript.h"
#include "Sequence.h"
#include "NGSHelper.h"
#include <QString>
#include <QHash>
#include <QSet>

///Representation of the effect of a variant
enum class VariantConsequenceType
{
    SPLICE_ACCEPTOR_VARIANT,
    SPLICE_DONOR_VARIANT,
    STOP_GAINED,
    FRAMESHIFT_VARIANT,
    STOP_LOST,
    START_LOST,
    INFRAME_INSERTION,
    INFRAME_DELETION,
    MISSENSE_VARIANT,
    PROTEIN_ALTERING_VARIANT,
    SPLICE_REGION_VARIANT,
    INCOMPLETE_TERMINAL_CODON_VARIANT,
    START_RETAINED_VARIANT,
    STOP_RETAINED_VARIANT,
    SYNONYMOUS_VARIANT,
    CODING_SEQUENCE_VARIANT,
    FIVE_PRIME_UTR_VARIANT,
    THREE_PRIME_UTR_VARIANT,
    NON_CODING_TRANSCRIPT_EXON_VARIANT,
    INTRON_VARIANT,
    NON_CODING_TRANSCRIPT_VARIANT,
    UPSTREAM_GENE_VARIANT,
    DOWNSTREAM_GENE_VARIANT,
    INTERGENIC_VARIANT
};

inline uint qHash(VariantConsequenceType key, uint seed)
{
    return ::qHash(static_cast<uint>(key), seed);
}

///Representation of the level of impact of a variant
enum class VariantImpact
{
    HIGH,
    MODERATE,
    LOW,
    MODIFIER
};

///Representation of HGVS nomenclature
struct CPPNGSSHARED_EXPORT HgvsNomenclature
{
    QString transcript_id;
    //QString hgvs_g;
    QString hgvs_c;
    QString hgvs_p;
    QSet<VariantConsequenceType> variant_consequence_type;
    QString exon_intron_number;

    VariantImpact consequenceTypeToImpact(VariantConsequenceType type)
    {
        switch(type)
        {
            case VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT:
            case VariantConsequenceType::SPLICE_DONOR_VARIANT:
            case VariantConsequenceType::STOP_GAINED:
            case VariantConsequenceType::FRAMESHIFT_VARIANT:
            case VariantConsequenceType::STOP_LOST:
            case VariantConsequenceType::START_LOST:
                return VariantImpact::HIGH;
                break;
            case VariantConsequenceType::INFRAME_INSERTION:
            case VariantConsequenceType::INFRAME_DELETION:
            case VariantConsequenceType::MISSENSE_VARIANT:
            case VariantConsequenceType::PROTEIN_ALTERING_VARIANT:
                return VariantImpact::MODERATE;
                break;
            case VariantConsequenceType::SPLICE_REGION_VARIANT:
            case VariantConsequenceType::INCOMPLETE_TERMINAL_CODON_VARIANT:
            case VariantConsequenceType::START_RETAINED_VARIANT:
            case VariantConsequenceType::STOP_RETAINED_VARIANT:
            case VariantConsequenceType::SYNONYMOUS_VARIANT:
                return VariantImpact::LOW;
                break;
            case VariantConsequenceType::CODING_SEQUENCE_VARIANT:
            case VariantConsequenceType::FIVE_PRIME_UTR_VARIANT:
            case VariantConsequenceType::THREE_PRIME_UTR_VARIANT:
            case VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT:
            case VariantConsequenceType::INTRON_VARIANT:
            case VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT:
            case VariantConsequenceType::UPSTREAM_GENE_VARIANT:
            case VariantConsequenceType::DOWNSTREAM_GENE_VARIANT:
            case VariantConsequenceType::INTERGENIC_VARIANT:
                return VariantImpact::MODIFIER;
                break;
        }
    }
};

///Class for generating HGVS nomenclature and variant effect from VCF/GSVar
class CPPNGSSHARED_EXPORT VariantHgvsAnnotator
{
private:
    //How far upstream or downstream can the variant be from the transcript
    int max_dist_to_transcript_;

    //How far the splice region extends into exon/intron
    int splice_region_ex_;
    int splice_region_in_;

    QString annotateRegionsCoding(const Transcript& transcript, HgvsNomenclature& hgvs, int gen_pos, bool plus_strand);
    QString annotateRegionsNonCoding(const Transcript& transcript, HgvsNomenclature& hgvs, int gen_pos, bool plus_strand);
    QString getHgvsPosition(const BedFile& regions, int gen_pos, bool plus_strand, bool utr_5 = false);
    QString getPositionInIntron(const BedFile& regions, int genomic_position, bool plus_strand, bool utr_5 = false);
    QString getHgvsProteinAnnotation(const VcfLine& variant, const FastaFileIndex& genome_idx, const QString& pos_hgvs_c, bool plus_strand);

    QByteArray toThreeLetterCode(QChar aa_one_letter_code);

    void annotateSpliceRegion(HgvsNomenclature& hgvs, const Transcript& transcript, int start, int end, bool plus_strand);

    void annotateProtSeqCsqSnv(HgvsNomenclature& hgvs);

public:
    ///Default constructor
    VariantHgvsAnnotator();

    ///Constructor to change parameters for detecting up/downstream and splice region variants
    VariantHgvsAnnotator(int max_dist_to_transcript, int splice_region_ex, int splice_region_in);

    ///Converts a variant in VCF format to HGVS nomenclature
    HgvsNomenclature variantToHgvs(const Transcript& transcript, const VcfLine& variant, const FastaFileIndex& genome_idx);
    HgvsNomenclature variantToHgvs(const Transcript& transcript, const Variant& variant, const FastaFileIndex& genome_idx);

    Sequence getCodingSequence(const Transcript& trans, const FastaFileIndex& genome_idx);
    QByteArray translate(const Sequence& seq, bool is_mito = false);
};

#endif // VARIANTHGVSANNOTATOR_H
