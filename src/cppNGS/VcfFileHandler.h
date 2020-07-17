#pragma once

#include "VcfFileHelper.h"
#include "BedFile.h"
#include <zlib.h>

//####################### CHANGES ######################
/*
 *  TO DOS:
 *
 *  make libs handle multiallelic =>  z.B. in ancestry() ???
 *		-> tests anpassen
 *  - order functions alphabetically
 *  - add filter into header when it first appears in a vcf line
 *		tests anpassen
 *  - tests for VCFLine and VCFHeader
 *
 *  - TEST Somatic for LIB and SomaticQC as TOOL (both have two new vcf, must be generated new with new fucntion)
 *  - VcfFile mit VcfHandler mergen
 *  - fucntion VariantList::storeAsVcf: genotype in sample/format leftAlign reverse with RefSequ
 *		test: 10 insert, del, Snp, complexSubst
 *
 * ENDE:
 * - streaming tools use VcfLine
 *
 * - use check at the end of store (one time for all tests)
 *
 *
 * INFO:
 * MULTI SAMPLE: VCFSORT SOMATICQC, UPDHUTNER, VCFFilter
 */

///class handling vcf and vcf.gz files
class CPPNGSSHARED_EXPORT VcfFileHandler
{

public:

	///loads a vcf or vcf.gz file
	void load(const QString& filename, bool allow_multi_sample=false, const BedFile* roi=nullptr, bool invert=false);
	///stores the data of VCFFileHandler in a vcf file
	void store(const QString& filename, bool stdout_if_file_empty = false, bool compress=false, int compression_level = 1) const;
	///stores a VCFFile as tsv file, INFO and FORMAT fields are differentiated by "_info" and "_format" attached to the name in ##Description lines,
	///in the header line each FORMAT column has additionally the sample name attached:
	/// ##VCF:
	/// #INFO	FORMAT	SAMPLE_1	SAMPLE_2
	/// uniqInfo=1;TWICE=2	GT:TWICE	1|1:z	0|1:z
	///
	/// ##TSV:
	/// #uniqInfo_info	TWICE_info	GT_format_SAMPLE_1	TWICE_format_SAMPLE_1	GT_format_SAMPLE_2	TWICE_format_SAMPLE_2
	///	1	2	1|1	z	0|1	z
	void storeAsTsv(const QString& filename) const;

	void leftNormalize(QString reference_genome);

	void sort(bool use_quality = false);
	void sortByFile(QString filename);
	///Costum sorting of variants.
	template <typename T>
	void sortCustom(const T& comparator)
	{
		std::sort(vcf_lines_.begin(), vcf_lines_.end(), comparator);
	}

	void removeDuplicates(bool sort_by_quality);
	int count() const
	{
		return vcf_lines_.count();
	}
	///Returns analysis type.
	AnalysisType type(bool allow_fallback_germline_single_sample = true) const;
	///creates a VCFLine for a Variant, adds it to the vcf_lines_ and returns it
	const VCFLine& addGSvarVariant(const Variant& var);

	///returns a QList of all sample names
	QByteArrayList sampleIDs() const;
	///returns a QList of all information IDs in the vcf file
	QByteArrayList informationIDs() const;
	///returns a QList of all filter IDs in the vcf file
	QByteArrayList filterIDs() const;
	///returns a QList of all format IDs in the vcf file
	QByteArrayList formatIDs() const;

	///returns a QVector of vcf_lines_
	const QVector<VCFLine>& vcfLines() const
	{
		return vcf_lines_;
	}
	///Read-Write access to a vcf_lines_
	QVector<VCFLine>& vcfLines()
	{
		return vcf_lines_;
	}
	///returns the VCFLine at pos
	const VCFLine& vcfLine(int pos) const
	{
		return vcf_lines_.at(pos);
	}
	///Read-Write access to a vcf_line
	VCFLine& vcfLine(int pos)
	{
		return vcf_lines_[pos];
	}
	///returns the VCFLine at pos
	const VCFLine& operator[](int pos) const
	{
		return vcf_lines_.at(pos);
	}
	///Read-Write access to a vcf_line by operator[]
	VCFLine& operator[](int pos)
	{
		return vcf_lines_[pos];
	}

	///returns a struct storing header information
	const VCFHeader& vcfHeader() const
	{
		return vcf_header_;
	}
	///Read-Write access to the vcf header
	VCFHeader& vcfHeader()
	{
		return vcf_header_;
	}
	///returns a QVector of all column headers for a vcfLine
	const QVector<QByteArray>& vcfColumnHeader() const
	{
		return column_headers_;
	}

	///Resize vector of vcf lines.
	void resize(int size)
	{
		vcf_lines_.resize(size);
	}

	void storeLineInformation(QTextStream& stream, VCFLine line) const;
	QString lineToString(int pos) const;

private:

	void parseVcfHeader(const int line_number, QByteArray& line);
	void parseHeaderFields(QByteArray& line, bool allow_multi_sample);
	void parseVcfEntry(const int line_number, QByteArray& line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert=false);
	void processVcfLine(int& line_number, QByteArray line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert=false);
	void loadFromVCF(const QString& filename, bool allow_multi_sample=false, ChromosomalIndex<BedFile>* roi_idx=nullptr, bool invert=false);
	void loadFromVCFGZ(const QString& filename, bool allow_multi_sample=false, ChromosomalIndex<BedFile>* roi_idx=nullptr, bool invert=false);

	void clear();

	QVector<VCFLine> vcf_lines_;
	VCFHeader vcf_header_;
	QVector<QByteArray> column_headers_;

};
