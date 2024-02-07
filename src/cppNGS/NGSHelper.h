#ifndef NGSHELPER_H
#define NGSHELPER_H

#include "cppNGS_global.h"
#include "GenomeBuild.h"
#include "Transcript.h"
#include "GeneSet.h"
#include "VcfFile.h"
#include "BamReader.h"

//Helper datastructure for gene impringing info.
struct ImprintingInfo
{
	QByteArray expressed_allele;
	QByteArray status;
};

//Target region information.
struct TargetRegionInfo
{
	QString name;
	BedFile regions;
	GeneSet genes;

	//Returns if a target region is set
	bool isValid() const
	{
		return !regions.isEmpty();
	}

	//Clears all data
	void clear()
	{
		name.clear();
		regions.clear();
		genes.clear();
	}

};

//Settings for Gff parser
struct GffSettings
{
	bool skip_not_gencode_basic = true; //skip transcripts that are not flagged as "GENCODE basic"
	bool print_to_stdout = true; //print summary to stdout
};

//Output of Ensembl GFF file parser
struct GffData
{
	//Transcripts
	TranscriptList transcripts;

	//Map from ENST to ENSG.
	QHash<QByteArray, QByteArray> enst2ensg;

	//Map from ENSG to gene symbol
	QHash<QByteArray, QByteArray> ensg2symbol;
};


//Impact of MaxEntScan prediction
enum MaxEntScanImpact
{
	LOW,
	MODERATE,
	HIGH
};

///Helper class for NGS-specific stuff.
class CPPNGSSHARED_EXPORT NGSHelper
{
public:
	///Returns known SNPs and indels from gnomAD (AF>=1%, AN>=5000).
	static VcfFile getKnownVariants(GenomeBuild build, bool only_snvs, const BedFile& roi, double min_af=0.0, double max_af=1.0);
	static VcfFile getKnownVariants(GenomeBuild build, bool only_snvs, double min_af=0.0, double max_af=1.0);

	///Soft-clip alignment from the beginning or end (positions are 1-based)
	static void softClipAlignment(BamAlignment& al, int start_ref_pos, int end_ref_pos);

	///Create sample overview file
	static void createSampleOverview(QStringList in, QString out, int indel_window=100, bool cols_auto=true, QStringList cols = QStringList());

	///Translates a codon to the 1-letter amino acid code
	static char translateCodon(const QByteArray& codon, bool use_mito_table=false);
	static QByteArray translateCodonThreeLetterCode(const QByteArray& codon, bool use_mito_table=false);

	///Converts a 1-letter amino acid code to a 3-letter amino acid code
	static QByteArray threeLetterCode(char aa_one_letter_code);

	///Converts a 3-letter amino acid code to a 1-letter amino acid code
	static char oneLetterCode(const QByteArray& aa_tree_letter_code);

	///Returns the pseudoautomal regions on gnosomes.
	static const BedFile& pseudoAutosomalRegion(GenomeBuild build);

	///Returns the cytogenetic band for to chromosomal position
	static QByteArray cytoBand(GenomeBuild build, Chromosome chr, int pos);
	///Returns the chromosomal range of a cytoband or cytoband range.
	static BedLine cytoBandToRange(GenomeBuild build, QByteArray cytoband);

	///Returns a map if imprinted genes and inherited allele.
	static const QMap<QByteArray, ImprintingInfo>& imprintingGenes();

	///Parses a chromosomal region from the given text. Throws an error, if the region is not valid.
	static void parseRegion(const QString& text, Chromosome& chr, int& start, int& end, bool allow_chr_only = false);

	///Returns Bed File with coordinates of centromeres.
	static const BedFile& centromeres(GenomeBuild build);

	///Returns Bed file with coordinates of telomeres.
	static const BedFile& telomeres(GenomeBuild build);

	///Converts the 3letter ancestry code to a human-readable text, see http://m.ensembl.org/Help/Faq?id=532
	static QString populationCodeToHumanReadable(QString code);

	///Returns transcripts with features from a Ensembl GFF file, transcript_gene_relation (ENST>ENSG) and gene_name_relation (ENSG>gene symbol).
	static GffData loadGffFile(QString filename, GffSettings settings);

	///Returns a map with matching Ensembl, RefSeq and CCDS transcript identifiers (without version numbers).
	static const QMap<QByteArray, QByteArrayList>& transcriptMatches(GenomeBuild build);

	///Returns the MaxEntScan impact. 'score_pairs_with_impact' returns the score apirs with annotation of impact (if not low).
	static MaxEntScanImpact maxEntScanImpact(const QByteArrayList& score_pairs, QByteArray& score_pairs_with_impact, bool splice_site_only);
	///Returns the maximum SpliceAI score based on the annotation. Returns -1 if no score was calculated. If tooltip is set, detail for showing in a GUI (gene, score, position offset) are written into the variable.
	static double maxSpliceAiScore(QString annotation_string, QString* tooltip = nullptr);

private:
	///Constructor declared away
	NGSHelper() = delete;
};

#endif // NGSHELPER_H
