#ifndef STATISTICS_H
#define STATISTICS_H

#include "cppNGS_global.h"
#include "VariantList.h"
#include "BedFile.h"
#include "QCCollection.h"
#include <QMap>


///NGS statistics and some BAM file operations.
class CPPNGSSHARED_EXPORT Statistics
{
public:
	///Calculates QC metrics on a variant list (only for VCF).
	static QCCollection variantList(const VariantList& variants);
    ///Calculates mapping QC metrics for a target regionfrom a BAM file. The input BED file must be merged!
	static QCCollection mapping(const BedFile& bed_file, const QString& bam_file, int min_mapq=1);
    ///Calculates mapping QC metrics for WGS from a BAM file. The input BED file must be merged!
    static QCCollection mapping(const QString& genome, const QString& bam_file, int min_mapq=1);
    ///Calculates special mapping QC metrics on three defined exons.
	static QCCollection mapping3Exons(const QString& bam_file);
	///Calculates target region statistics (term-value pairs). @p merge determines if overlapping regions are merged before calculating the statistics.
	static QCCollection region(const BedFile& bed_file, bool merge);

	///Calculates the part of the target region that has a lower coverage than the given cutoff. The input BED file must be merged and sorted!
	static BedFile lowCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff, int min_mapq=1);
	///Calculates and annotates the average coverage of the regions in the bed file. The input BED file must be merged and sorted!
	static void avgCoverage(BedFile& bed_file, const QString& bam_file, int min_mapq=1);

	///Determines the gender based on the read ratio between X and Y chromosome.
	static QString genderXY(const QString& bam_file, QStringList& debug_output, double max_female=0.06, double min_male=0.09);
	///Determines the gender based on the fraction of heterocygous SNPs on chromosome X.
	static QString genderHetX(const QString& bam_file, QStringList& debug_output, double max_male=0.15, double min_female=0.24);

protected:
	///No default constructor
	Statistics();
};

#endif // STATISTICS_H
