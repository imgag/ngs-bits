#ifndef NGSHELPER_H
#define NGSHELPER_H

#include "api/BamReader.h"
#include "cppNGS_global.h"
#include "Pileup.h"
#include "VariantList.h"
#include "ChromosomalIndex.h"
#include <QPair>
#include <QStringList>
#include "limits"

using namespace BamTools;

///Variant details struct
struct VariantDetails
{
	VariantDetails()
		: depth(std::numeric_limits<int>::quiet_NaN())
		, frequency(std::numeric_limits<double>::quiet_NaN())
		, mapq0_frac(std::numeric_limits<double>::quiet_NaN())
	{
	}

	int depth;
	double frequency;
	double mapq0_frac;
};

///Helper class for NGS-specific stuff.
class CPPNGSSHARED_EXPORT NGSHelper
{
public:
	///Opens a BAM file for reading.
	static void openBAM(BamTools::BamReader& reader, QString bam_file);

	/**
	  @brief Returns the pileup at the given chromosomal position (1-based).
	  @param indel_window The value controls how far up- and down-stream of the given postion, indels are considered to compensate for alignment differences. Indels are not reported when this paramter is set to -1.
	*/
	static Pileup getPileup(BamTools::BamReader& reader, const Chromosome& chr, int pos, int indel_window = -1, int min_mapq = 1, bool anom = false, int min_baseq = 13);
	///Returns the pileups for a the given chromosomal range (1-based).
	static void getPileups(QList<Pileup>& pileups, BamTools::BamReader& reader, const Chromosome& chr, int start, int end, int min_mapq = 1);

	///Returns the depth/frequency for a variant (start, ref, obs in TSV style). If the depth is 0, quiet_NaN is returned as frequency.
	static VariantDetails getVariantDetails(BamTools::BamReader& reader, const FastaFileIndex& reference, const Variant& variant);

	/**
	  @brief Returns indels for a chromosomal range (1-based) and the depth of the region.
	  @note Insertions are prefixed with '+', deletions with '-'.
	*/
	static void getIndels(const FastaFileIndex& reference, BamTools::BamReader& reader, const Chromosome& chr, int start, int end, QVector<Sequence>& indels, int& depth, double& mapq0_frac);

	///Returns selected SNPs of the hg19 genome (only single base exchange, MAF 20%-80%).
	static VariantList getSNPs();

	/**
	  @brief Returns the correct base of an alignment at a chromosomal position (1-based) using the CIGAR data (compensates for indels) along with its quality value (calculated form the ASCII character by using an offset of 33).
	  @note If the base is deleted, '-' with quality 255 is returned. If the base is skipped/soft-clipped, '~' with quality -1 is returned.
	  @note The alignment has to contain string data, i.e. not only the core data can be loaded.
	*/
	static QPair<char, int> extractBaseByCIGAR(const BamTools::BamAlignment& al, int pos);
	/**
	  @brief Returns the indels at a chromosomal position (1-based) or a range when using the @p indel_window parameter.
	  @note Insertions: The string starts with '+' and then contains the bases (e.g. '+TT'). The position is the base @em before which insersion is located.
	  @note Deletions: the string starts with '-' and then contains the number of bases (e.g. '-2'). The position is the first deleted base.
	*/
	static void extractIndelsByCIGAR(QVector<Sequence>& indels, BamTools::BamAlignment& al, int pos, int indel_window=0);

	///Returns the complement of a base
	static char complement(char base);
	///Changes a DNA sequence to reverse/complementary order.
	static QByteArray changeSeq(const QByteArray& seq, bool rev, bool comp);

	///Soft-clip alignment from the beginning or end (positions are 1-based)
	static void softClipAlignment(BamTools::BamAlignment& al, int start_ref_pos, int end_ref_pos);

	///Convert Cigar data to QString
	static QString Cigar2QString(std::vector<CigarOp> Cigar, bool expand = false);

	///Converts text to gene list (one gene per tab-separated line)
	static QStringList textToGenes(QString text, int col_index = 0);

	///Create sample overview file
	static void createSampleOverview(QStringList in, QString out, int indel_window=100, bool cols_auto=true, QStringList cols = QStringList());

private:
	///Constructor declared away
	NGSHelper();
};

#endif // NGSHELPER_H
