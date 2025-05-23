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
#include "WorkerLowOrHighCoverage.h"
#include "WorkerAverageCoverage.h"

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
	static QCCollection variantList(const VcfFile& variants, bool filter);
	////Calculates QC metrics for phasing on a VCF file (long read data) and returns the phasing blocks as BED file
	static QCCollection phasing(const VcfFile& variants, bool filter, BedFile& phasing_blocks);
	///Calculates mapping QC metrics for a target region from a BAM file. The input BED file must be merged!
	static QCCollection mapping(const BedFile& bed_file, const QString& bam_file, const QString& ref_file, int min_mapq=1, bool is_cfdna = false);
	///Calculates mapping QC metrics from a BAM file.
	static QCCollection mapping(const QString& bam_file, int min_mapq=1, const QString& ref_file = QString());
	///Calculates mapping QC metrics for WGS from a BAM file.
	static QCCollection mapping_wgs(const QString& bam_file, const QString& bedpath="", int min_mapq=1, const QString& ref_file = QString());
	///Calculates mapping QC metrics for a housekeeping genes exon region from a BAM file. The input BED file must be merged!
	static QCCollection mapping_housekeeping(const BedFile& bed_file, const QString& bam_file, const QString& ref_file, int min_mapq=1);
	///Calculates target region statistics (term-value pairs). @p merge determines if overlapping regions are merged before calculating the statistics.
	static QCCollection region(const BedFile& bed_file, bool merge);
	///Calculates somatic QC metrics from BAM and VCF file
	static QCCollection somatic(GenomeBuild build, QString& tumor_bam, QString& normal_bam, QString& somatic_vcf, QString ref_fasta, const BedFile& target_file, bool skip_plots = false);

	///Calculates QC depths for somatic custom panel
	static QCCollection somaticCustomDepth(const BedFile& bed_file, QString bam_file, QString ref_file, int min_mapq=1);

	///Calculates mutation burden metric from somatic VCF normalized to Exome Size
	static QCValue mutationBurdenNormalized(QString somatic_vcf, QString exons, QString target, QString tsg, QString blacklist);
	///Calculates unnormalized raw mutation burden
	static QCValue mutationBurden(QString somatic_vcf, QString target, QString blacklist);

	///Calculates the percentage of common SNPs that lie outside the expected allele frequency range for diploid organisms.
	static QCCollection contamination(GenomeBuild build, QString bam, const QString& ref_file = QString(), bool debug = false, int min_cov = 20, int min_snps = 50, bool include_not_properly_paired = false);
	///Returns ancestry estimates for a variant list in VCF format.
	static AncestryEstimates ancestry(GenomeBuild build, QString filename, int min_snp=1000, double abs_score_cutoff = 0.32, double max_mad_dist = 4.2);

	///Calculates the part of the target region that has a lower coverage than the given cutoff. The input BED file must be merged and sorted!
	static BedFile lowCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff, int min_mapq=1, int min_baseq=0, int threads=1, const QString& ref_file = QString(), bool random_access=true, bool debug=false);
	///Calculates and annotates the average coverage of the regions in the bed file. Debug flag enables debug output to stdout.
	static void avgCoverage(BedFile& bed_file, const QString& bam_file, int min_mapq=1, int threads=1, int decimals=2, const QString& ref_file = QString(), bool random_access=true, bool debug=false);
	///Calculates the part of the genome that has a higher coverage than the given cutoff.
	static BedFile highCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff, int min_mapq=1, int min_baseq=0, int threads=1, const QString& ref_file = QString(), bool random_access=true, bool debug=false);

	///Determines the gender based on the read ratio between X and Y chromosome.
	static GenderEstimate genderXY(QString bam_file, double max_female=0.06, double min_male=0.09, const QString& ref_file = QString());
	///Determines the gender based on the fraction of heterozygous SNPs on chromosome X.
	static GenderEstimate genderHetX(GenomeBuild build, QString bam_file, double max_male=0.15, double min_female=0.24, const QString& ref_file = QString(), bool include_not_properly_paired = false);
	///Determines the gender based on the coverge of the SRY gene on chrY.
	static GenderEstimate genderSRY(GenomeBuild build, QString bam_file, double min_cov=20.0, const QString& ref_file = QString());

protected:
	///No default constructor
	Statistics();

private:
	static BedFile lowOrHighCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff, int min_mapq, int min_baseq, int threads, const QString& ref_file, bool is_high, bool random_access, bool debug);
	//Returns the ratio of chrY and chrX reads (for gender check and determining XXY karyotype). If no reads are found on chrX, nan is returned.
	static double yxRatio(BamReader& reader, double* count_x=nullptr, double* count_y=nullptr);

	template <typename T>
	static void addQcValue(QCCollection& output, QByteArray accession, QByteArray name, const T& value);
	static void addQcPlot(QCCollection& output, QByteArray accession, QByteArray name, QString filename);
};

#endif // STATISTICS_H
