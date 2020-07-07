#pragma once

#include "VcfFileHelper.h"
#include "BedFile.h"

namespace VcfFormat
{

///class handling vcf and vcf.gz files
class CPPNGSSHARED_EXPORT VcfFileHandler
{

public:

	///loads a vcf or vcf.gz file
	void load(const QString& filename, const BedFile* roi=nullptr);
	///stores the data of VCFFileHandler in vcf file
	void store(const QString& filename) const;

	void checkValid() const;
	void sort(bool use_quality = false);
	void removeDuplicates(bool sort_by_quality);
	int count() const
	{
		return VcfLines_.count();
	}
	///Returns analysis type.
	AnalysisType type(bool allow_fallback_germline_single_sample = true) const;



	///returns a QList of all sample names
	QByteArrayList sampleIDs() const;
	///returns a QList of all information IDs in the vcf file
	QByteArrayList informationIDs() const;
	///returns a QList of all filter IDs in the vcf file
	QByteArrayList filterIDs() const;
	///returns a QList of all format IDs in the vcf file
	QByteArrayList formatIDs() const;

	///returns a QVector of VCFLines
	QVector<VCFLine> vcfLines() const
	{
		return VcfLines_;
	}
	///returns the VCFLine at pos
	VCFLine vcfLine(int pos) const
	{
		return VcfLines_.at(pos);
	}
	///returns a struct storing header information
	VCFHeader vcfHeader() const
	{
		return VcfHeader_;
	}
	///returns a QVector of all column headers for a vcfLine
	QVector<QByteArray> vcfColumnHeader() const
	{
		return column_headers_;
	}


private:

	void parseVcfHeader(const int line_number, QByteArray& line);
	void parseHeaderFields(QByteArray& line);
	void parseVcfEntry(const int line_number, QByteArray& line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids, ChromosomalIndex<BedFile>* roi_idx);
	void processVcfLine(int& line_number, QByteArray line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids, ChromosomalIndex<BedFile>* roi_idx);
	void loadFromVCF(const QString& filename, ChromosomalIndex<BedFile>* roi_idx=nullptr);
	void loadFromVCFGZ(const QString& filename, ChromosomalIndex<BedFile>* roi_idx=nullptr);
	void clear();

	QVector<VCFLine> VcfLines_;
	VCFHeader VcfHeader_;
	QVector<QByteArray> column_headers_;

};

} //end namespace VcfFormat
