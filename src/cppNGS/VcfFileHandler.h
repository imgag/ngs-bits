#pragma once

#include "VcfFileHelper.h"
#include "BedFile.h"
#include "VariantList.h"


//####################### CHANGES ######################
/*
 *
 *  TO DOS:
 *
 * - when accessing alternative base sequences we always have to index the first one (before only one was stored)
 *  =>  z.B. in ancestry() ???
 * - when accessing SAMPLES we have to index the first one (before only one SAMPLE was stored)
 *
 *
 *  - modified storeVcfToTsv to also handle multiple sample entries: before every formatID was combined with "_ss" ???
 *  - all bases are processed to be UPPER CASE !!!
 *  - toUTF8() for some variables (internally QByteArray in VCFFileHandler)
 *  - normalize function does pos += 1 only for bool=to_gsvar
 *
 *  - output of storeVcf is in order of info/filter/format tags as they were in the origional line (before according to header order)
 *  - store function in VariantList supports VCF: however might not work ideally... (accessing of ID QUAL and FILTER)
 *
 *  - tools taking both: SampleSimilarity / VariantFilterRegions / VcfStore
 *
 * #KLEINE TODOS:
 *
 *  - remove namespace/ order functions alphabetically
 *  - add filter into header when it first appears in a vcf line
 *  - tests for VCFLine and VCFHeader
 *  - TEST Somatic angucken: vcf file has to be checked again (it was 'falsly' genereated with storeToVcf and filters were copied)
 *
 */
namespace VcfFormat
{

///class handling vcf and vcf.gz files
class CPPNGSSHARED_EXPORT VcfFileHandler
{

public:

	///loads a vcf or vcf.gz file
	void load(const QString& filename, const BedFile* roi=nullptr, bool invert=false);
	///stores the data of VCFFileHandler in a vcf file
	void store(const QString& filename, VariantListFormat format=VCF) const;

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
	void parseHeaderFields(QByteArray& line);
	void parseVcfEntry(const int line_number, QByteArray& line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids, ChromosomalIndex<BedFile>* roi_idx, bool invert=false);
	void processVcfLine(int& line_number, QByteArray line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids, ChromosomalIndex<BedFile>* roi_idx, bool invert=false);
	void loadFromVCF(const QString& filename, ChromosomalIndex<BedFile>* roi_idx=nullptr, bool invert=false);
	void loadFromVCFGZ(const QString& filename, ChromosomalIndex<BedFile>* roi_idx=nullptr, bool invert=false);
	void storeToVcf(const QString& filename) const;
	void storeToTsv(const QString& filename) const;

	void clear();

	QVector<VCFLine> vcf_lines_;
	VCFHeader vcf_header_;
	QVector<QByteArray> column_headers_;

};

} //end namespace VcfFormat
