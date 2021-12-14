#ifndef STATISTICS_H
#define STATISTICS_H

#include "cppNGS_global.h"
#include "VcfFile.h"
#include "BedFile.h"
#include "QCCollection.h"
#include "KeyValuePair.h"
#include "NGSHelper.h"
#include "GenomeBuild.h"
#include <QMap>

///Helper class for gender estimates
struct CPPNGSSHARED_EXPORT GenderEstimate
{
	QString gender;
	QList<KeyValuePair> add_info; //key-value pairs of additional information
};


///Helper class for ancestry estimates
struct CPPNGSSHARED_EXPORT AncestryEstimates
{
	int snps;
	double afr;
	double eur;
	double sas;
	double eas;
	QString population;
};

///NGS statistics and some BAM file operations.
class CPPNGSSHARED_EXPORT Statistics
{
public:
	///Calculates QC metrics on a variant list (only for VCF).
	static QCCollection variantList(VcfFile variants, bool filter);
	///Calculates mapping QC metrics for a target region from a BAM file. The input BED file must be merged!
	static QCCollection mapping(const BedFile& bed_file, const QString& bam_file, const QString& ref_file, int min_mapq=1, bool is_cfdna = false);
    ///Calculates mapping QC metrics for RNA from a BAM file.
	static QCCollection mapping_rna(const QString& bam_file, int min_mapq=1, const QString& ref_file = QString());
	///Calculates mapping QC metrics for WGS from a BAM file.
	static QCCollection mapping(const QString& bam_file, int min_mapq=1, const QString& ref_file = QString());
	///Calculates target region statistics (term-value pairs). @p merge determines if overlapping regions are merged before calculating the statistics.
	static QCCollection region(const BedFile& bed_file, bool merge);
	///Calculates somatic QC metrics from BAM and VCF file
	static QCCollection somatic(GenomeBuild build, QString& tumor_bam, QString& normal_bam, QString& somatic_vcf, QString ref_fasta, const BedFile& target_file, bool skip_plots = false, const QString& ref_file_cram = QString());
	///Calculates mutation burden metric from somatic VCF
	static QCValue mutationBurden(QString somatic_vcf, QString exons, QString target, QString tsg, QString blacklist);
	///Calculates the percentage of common SNPs that lie outside the expected allele frequency range for diploid organisms.
	static QCCollection contamination(GenomeBuild build, QString bam, const QString& ref_file = QString(), bool debug = false, int min_cov = 20, int min_snps = 50);
	///Returns ancestry estimates for a variant list in VCF format.
	static AncestryEstimates ancestry(GenomeBuild build, QString filename, int min_snp=1000, double abs_score_cutoff = 0.32, double max_mad_dist = 4.2);

	///Calculates the part of the target region that has a lower coverage than the given cutoff. The input BED file must be merged and sorted!
	static BedFile lowCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff, int min_mapq=1, int min_baseq=0, const QString& ref_file = QString());
    ///Calculates the part of the genome that has a lower coverage than the given cutoff.
	static BedFile lowCoverage(const QString& bam_file, int cutoff, int min_mapq=1, int min_baseq=0, const QString& ref_file = QString());
	///Calculates and annotates the average coverage of the regions in the bed file. The input BED file must be merged and sorted! Panel mode should be used if only a small part of the BAM data is needed.
	static void avgCoverage(BedFile& bed_file, const QString& bam_file, int min_mapq=1, bool include_duplicates=false, bool panel_mode=false, int decimals=2, const QString& ref_file = QString());
	///Calculates the part of the target region that has a lower coverage than the given cutoff. The input BED file must be merged and sorted!
	static BedFile highCoverage(const QString& bam_file, int cutoff, int min_mapq=1, int min_baseq=0, const QString& ref_file = QString());
	///Calculates the part of the genome that has a higher coverage than the given cutoff.
	static BedFile highCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff, int min_mapq=1, int min_baseq=0, const QString& ref_file = QString());

	///Determines the gender based on the read ratio between X and Y chromosome.
	static GenderEstimate genderXY(QString bam_file, double max_female=0.06, double min_male=0.09, const QString& ref_file = QString());
	///Determines the gender based on the fraction of heterozygous SNPs on chromosome X.
	static GenderEstimate genderHetX(GenomeBuild build, QString bam_file, double max_male=0.15, double min_female=0.24, const QString& ref_file = QString());
	///Determines the gender based on the coverge of the SRY gene on chrY.
	static GenderEstimate genderSRY(GenomeBuild build, QString bam_file, double min_cov=20.0, const QString& ref_file = QString());

	static QCCollection hrdScore(const CnvList& cnvs, GenomeBuild build);

protected:
	///No default constructor
	Statistics();

private:
	static void countCoverageWithoutBaseQuality(QVector<int>& roi_cov, int ol_start, int ol_end);
	static void countCoverageWithBaseQuality(int min_baseq, QVector<int>& roi_cov, int start, int ol_start, int ol_end, QBitArray& baseQualities, const BamAlignment& al);
	static void countCoverageWGSWithoutBaseQuality(int start, int end, QVector<unsigned char>& cov);
	static void countCoverageWGSWithBaseQuality(int min_baseq, QVector<unsigned char>& cov, int start, int end, QBitArray& baseQualities, const BamAlignment& al);

	template <typename T>
	static void addQcValue(QCCollection& output, QByteArray accession, QByteArray name, const T& value);
	static void addQcPlot(QCCollection& output, QByteArray accession, QByteArray name, QString filename);
};

#endif // STATISTICS_H
