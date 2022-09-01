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
#include "Exceptions.h"
#include <QString>
#include <QHash>
#include <QSet>

///Representation of the effect of a variant
enum class VariantConsequenceType : int
{
    INTERGENIC_VARIANT,
    DOWNSTREAM_GENE_VARIANT,
    UPSTREAM_GENE_VARIANT,
    NON_CODING_TRANSCRIPT_VARIANT,
    INTRON_VARIANT,
    NON_CODING_TRANSCRIPT_EXON_VARIANT,
    THREE_PRIME_UTR_VARIANT,
    FIVE_PRIME_UTR_VARIANT,
    CODING_SEQUENCE_VARIANT,
    SYNONYMOUS_VARIANT,
    STOP_RETAINED_VARIANT,
    START_RETAINED_VARIANT,
    INCOMPLETE_TERMINAL_CODON_VARIANT,
    SPLICE_REGION_VARIANT,
    PROTEIN_ALTERING_VARIANT,
    MISSENSE_VARIANT,
    INFRAME_DELETION,
    INFRAME_INSERTION,
    START_LOST,
    STOP_LOST,
    FRAMESHIFT_VARIANT,
    STOP_GAINED,
    SPLICE_DONOR_VARIANT,
    SPLICE_ACCEPTOR_VARIANT
};

inline uint qHash(VariantConsequenceType key)
{
	return qHash(static_cast<int>(key));
}

///Representation of the level of impact of a variant
enum class VariantImpact
{
    MODIFIER,
    LOW,
    MODERATE,
    HIGH
};

///Representation of HGVS nomenclature
struct CPPNGSSHARED_EXPORT HgvsNomenclature
{
    QString transcript_id;
	QString allele;
    QString hgvs_c;
    QString hgvs_p;
    QSet<VariantConsequenceType> variant_consequence_type;
    int exon_number{-1};
    int intron_number{-1};

	QString variantConsequenceTypesAsString(QString sep="&");

	static VariantImpact consequenceTypeToImpact(VariantConsequenceType type)
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
		THROW(ProgrammingException, "Unhandled variant consequence type " + QString::number(static_cast<int>(type)) + "!");
	}

	static QString consequenceTypeToString(VariantConsequenceType type)
    {
        switch(type)
        {
            case VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT: return "splice_acceptor_variant";
            case VariantConsequenceType::SPLICE_DONOR_VARIANT: return "splice_donor_variant";
            case VariantConsequenceType::STOP_GAINED: return "stop_gained";
            case VariantConsequenceType::FRAMESHIFT_VARIANT: return "frameshift_variant";
            case VariantConsequenceType::STOP_LOST: return "stop_lost";
            case VariantConsequenceType::START_LOST: return "start_lost";
            case VariantConsequenceType::INFRAME_INSERTION: return "inframe_insertion";
            case VariantConsequenceType::INFRAME_DELETION: return "inframe_deletion";
            case VariantConsequenceType::MISSENSE_VARIANT: return "missense_variant";
            case VariantConsequenceType::PROTEIN_ALTERING_VARIANT: return "protein_altering_variant";
            case VariantConsequenceType::SPLICE_REGION_VARIANT: return "splice_region_variant";
            case VariantConsequenceType::INCOMPLETE_TERMINAL_CODON_VARIANT: return "incomplete_terminal_codon_variant";
            case VariantConsequenceType::START_RETAINED_VARIANT: return "start_retained_variant";
            case VariantConsequenceType::STOP_RETAINED_VARIANT: return "stop_retained_variant";
            case VariantConsequenceType::SYNONYMOUS_VARIANT: return "synonymous_variant";
            case VariantConsequenceType::CODING_SEQUENCE_VARIANT: return "coding_sequence_variant";
            case VariantConsequenceType::FIVE_PRIME_UTR_VARIANT: return "5_prime_UTR_variant";
            case VariantConsequenceType::THREE_PRIME_UTR_VARIANT: return "3_prime_UTR_variant";
            case VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT: return "non_coding_transcript_exon_variant";
            case VariantConsequenceType::INTRON_VARIANT: return "intron_variant";
            case VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT: return "non_coding_transcript_variant";
            case VariantConsequenceType::UPSTREAM_GENE_VARIANT: return "upstream_gene_variant";
            case VariantConsequenceType::DOWNSTREAM_GENE_VARIANT: return "downstream_gene_variant";
            case VariantConsequenceType::INTERGENIC_VARIANT: return "intergenic_variant";
		}

		THROW(ProgrammingException, "Unhandled variant consequence type " + QString::number(static_cast<int>(type)) + "!");
    }
};

///Class for generating HGVS nomenclature and variant effect from VCF/GSVar
class CPPNGSSHARED_EXPORT VariantHgvsAnnotator
{
public:
    ///Constructor to change parameters for detecting up/downstream and splice region variants: different for 5 and 3 prime site intron
    VariantHgvsAnnotator(int max_dist_to_transcript, int splice_region_ex, int splice_region_in_5, int splice_region_in_3);

    ///Converts a variant in VCF format to HGVS nomenclature
	HgvsNomenclature variantToHgvs(const Transcript& transcript, VcfLine& variant, const FastaFileIndex& genome_idx);
	HgvsNomenclature variantToHgvs(const Transcript& transcript, const Variant& variant, const FastaFileIndex& genome_idx);

    QByteArray translate(const Sequence& seq, bool is_mito = false, bool end_at_stop = true);
    Sequence getCodingSequence(const Transcript& trans, const FastaFileIndex& genome_idx, bool add_utr_3 = false);

private:
	//How far upstream or downstream can the variant be from the transcript
	int max_dist_to_transcript_;

	//How far the splice region extends into exon/intron
	int splice_region_ex_;
	int splice_region_in_5_;
	int splice_region_in_3_;

	QString annotateRegionsCoding(const Transcript& transcript, HgvsNomenclature& hgvs, int gen_pos, bool plus_strand, bool is_dup = false);
	QString annotateRegionsNonCoding(const Transcript& transcript, HgvsNomenclature& hgvs, int gen_pos, bool plus_strand, bool is_dup = false);
	QString getHgvsPosition(const BedFile& regions, HgvsNomenclature& hgvs, int gen_pos, bool plus_strand, const BedFile& coding_regions, bool utr_5 = false, int first_region = 0);
	QString getPositionInIntron(const BedFile& regions, HgvsNomenclature& hgvs, int genomic_position, bool plus_strand, const BedFile &coding_regions, bool utr_5 = false, int first_region = 0);
	QString getHgvsProteinAnnotation(const VcfLine& variant, const FastaFileIndex& genome_idx, const QString& pos_hgvs_c, const Transcript& transcript, bool plus_strand);

	QByteArray toThreeLetterCode(char aa_one_letter_code);

	void annotateSpliceRegion(HgvsNomenclature& hgvs, const Transcript& transcript, int start, int end, bool plus_strand, bool insertion);

	void annotateProtSeqCsqSnv(HgvsNomenclature& hgvs);
};

#endif // VARIANTHGVSANNOTATOR_H
