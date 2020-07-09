#pragma once

#include "VcfFileHelper.h"
#include "BedFile.h"
#include "VariantList.h"


//####################### CHANGE LOG ######################
/*
 * - does not convert from VCf to TSV
 * - VariantList creates a seperate column for every INFO and FORMAT key ever observed in ANY VCF line
 *		=> now we only store key=value pairs for existing keys for every line seperately
 * - when accessing alternative base sequences we always have to index the first one (before only one was stored)
 * - when accessing SAMPLES we have to index the first one (before only one SAMPLE was stored)
 */
namespace VcfFormat
{

///class handling vcf and vcf.gz files
class CPPNGSSHARED_EXPORT VcfFileHandler
{

public:

	///loads a vcf or vcf.gz file
	void load(const QString& filename, const BedFile* roi=nullptr);
	///stores the data of VCFFileHandler in a vcf file
	void store(const QString& filename) const;

	void checkValid() const;
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

	///returns a QVector of VCFLines
	const QVector<VCFLine>& vcfLines() const
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

private:

	void parseVcfHeader(const int line_number, QByteArray& line);
	void parseHeaderFields(QByteArray& line);
	void parseVcfEntry(const int line_number, QByteArray& line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids, ChromosomalIndex<BedFile>* roi_idx);
	void processVcfLine(int& line_number, QByteArray line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids, ChromosomalIndex<BedFile>* roi_idx);
	void loadFromVCF(const QString& filename, ChromosomalIndex<BedFile>* roi_idx=nullptr);
	void loadFromVCFGZ(const QString& filename, ChromosomalIndex<BedFile>* roi_idx=nullptr);
	void clear();

	QVector<VCFLine> vcf_lines_;
	VCFHeader vcf_header_;
	QVector<QByteArray> column_headers_;

};

} //end namespace VcfFormat
