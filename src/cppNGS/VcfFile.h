#pragma once

#include "BedFile.h"
#include "VcfFileHelper.h"
#include "htslib/bgzf.h"

#include <zlib.h>

#define BGZF_NO_COMPRESSION         10
#define BGZF_GZIP_COMPRESSION		0
#define BGZF_BEST_SPEED             1
#define BGZF_BEST_COMPRESSION       9

///class handling vcf and vcf.gz files
class CPPNGSSHARED_EXPORT VcfFile
{

public:

	static const int MIN_COLS = 8;
	static const int CHROM = 0;
	static const int POS = 1;
	static const int ID = 2;
	static const int REF = 3;
	static const int ALT = 4;
	static const int QUAL = 5;
	static const int FILTER = 6;
	static const int INFO = 7;
	static const int FORMAT = 8;

	// definition of all characters which have to be escaped in the INFO values
	// (using URL-Encoding (https://en.wikipedia.org/wiki/Percent-encoding, RFC 3986))
	// values defined in VcfFile.cpp
	static const QList<KeyValuePair> INFO_URL_MAPPING;

	///Default constructor
	VcfFile();

	int count() const
	{
		return vcf_lines_.count();
	}
	///returns a QList of all filter IDs in the vcf file
	QByteArrayList filterIDs() const;
	///returns a QList of all format IDs in the vcf file
	QByteArrayList formatIDs() const;
	///returns a QList of all information IDs in the vcf file
	QByteArrayList informationIDs() const;
	///returns a QList of all sample names
	QByteArrayList sampleIDs() const;
	///save a variant line as string
	QString lineToString(int pos) const;

	///Leftnormalize every vcf line in the vcf file according to a reference genome
	void leftNormalize(QString reference_genome);
	///loads a vcf or vcf.gz file
	void load(const QString& filename, bool allow_multi_sample = true);
	void load(const QString& filename, const BedFile& roi,  bool allow_multi_sample = true, bool invert = false);
	///removes duplicate variants
	void removeDuplicates(bool sort_by_quality);
	///stores the data of VCFFileHandler in a vcf file
	void store(const QString& filename, bool stdout_if_file_empty = false, int compression_level = BGZF_NO_COMPRESSION) const;
	///stores a VCFFile as tsv file, INFO and FORMAT fields are differentiated by "_info" and "_format" attached to the name in ##Description lines,
	///in the header line each FORMAT column is additionally prefixed with the sample name:
	/// ##VCF:
	/// #INFO	FORMAT	SAMPLE_1	SAMPLE_2
	/// uniqInfo=1;TWICE=2	GT:TWICE	1|1:z	0|1:z
	///
	/// ##TSV:
	/// #uniqInfo_info	TWICE_info	SAMPLE_1_GT_format	SAMPLE_1_TWICE_format	SAMPLE_2_GT_format	SAMPLE_2_TWICE_format
	///	1	2	1|1	z	0|1	z
	void storeAsTsv(const QString& filename);
	void sort(bool use_quality = false);
	void sortByFile(QString filename);
	///Costum sorting of variants.
	template <typename T>
	void sortCustom(const T& comparator)
	{
		std::sort(vcf_lines_.begin(), vcf_lines_.end(), comparator);
	}

	///Returns a QVector of all vcf lines
	const QVector<VcfLinePtr>& vcfLines() const
	{
		return vcf_lines_;
	}
	///Read-Write access to all vcf lines
	QVector<VcfLinePtr>& vcfLines()
	{
		return vcf_lines_;
	}
	///Returns the Vcf line at pos
	const VcfLine& vcfLine(int pos) const
	{
		return *(vcf_lines_.at(pos));
	}
	///Read-Write access to a vcf line at pos
	VcfLine& vcfLine(int pos)
	{
		return *(vcf_lines_[pos]);
	}
	///Returns the vcf line at pos
	const VcfLine& operator[](int index) const
	{
		return *(vcf_lines_[index]);
	}
	///Read-Write access to a vcf line at pos
	VcfLine& operator[](int index)
	{
		return *(vcf_lines_[index]);
	}
	///Returns the vcf header
	const VcfHeader& vcfHeader() const
	{
		return vcf_header_;
	}
	///Read-Write access to the vcf header
	VcfHeader& vcfHeader()
	{
		return vcf_header_;
	}
	///Returns a QVector of all column headers for a vcf line
	const QVector<QByteArray>& vcfColumnHeader() const
	{
		return column_headers_;
	}
	///Read-Write access to column headers for a vcf line
	QVector<QByteArray>& vcfColumnHeader()
	{
		return column_headers_;
	}
	///Returns the pointer hashing each sample ID to its position in the sample list
	const SampleIDToIdxPtr& sampleIDToIdx() const
	{
		return sample_id_to_idx_;
	}
	///Returns the hash storing all possible format ID orders in the vcf file
	const QHash<ListOfFormatIds, FormatIDToIdxPtr>& formatIDToIdxList() const
	{
		return format_id_to_idx_list_;
	}
	///Returns the hash storing all possible info ID orders in the vcf file
	const QHash<ListOfInfoIds, FormatIDToIdxPtr>& infoIDToIdxList() const
	{
		return info_id_to_idx_list_;
	}

	///Resize vector of vcf lines.
	void resize(int size)
	{
		vcf_lines_.resize(size);
	}

	///Write all column headers to a QTextStream
	void storeHeaderColumns(QTextStream& stream) const;

	///Copies meta data from a VcfFile (comment, header, column headers), but not the variants.
	///(copies pointers for sampleIDs, format and info line order as well. Therefore should only
	/// be used to subset a VcfFile and copy the meta data, not to add entirely new variants)
	void copyMetaDataForSubsetting(const VcfFile& rhs);

	///Returns VCF content as string
    QByteArray toText() const;
	///Reads a VCF from a string
    void fromText(const QByteArray& text);

	///Converts a Variant list (e.g. from a GSvar file) to a VcfFile
	static VcfFile convertGSvarToVcf(const VariantList& variant_list, const QString& reference_genome);

	///Validates VCF file from file path
	static bool isValid(QString filename, QString ref_file, QTextStream& out_stream, bool print_general_information = false, int max_lines = std::numeric_limits<int>::max());

	///Returns the content of a column by index (tab-separated line) from a QByteArray line
	static QByteArray getPartByColumn(const QByteArray& line, int index);

	///Returns string where all forbidden char of an info column value are URL encoded
	static QString encodeInfoValue(QString info_value);

	///Returns string where all URL encoded chars of an info column value are decoded
	static QString decodeInfoValue(QString encoded_info_value);

private:

	void clear();
	void loadFromVCFGZ(const QString& filename, bool allow_multi_sample = true, ChromosomalIndex<BedFile>* roi_idx = nullptr, bool invert = false);
	void parseHeaderFields(const QByteArray& line, bool allow_multi_sample);
	void parseVcfEntry(int line_number, const QByteArray& line, QSet<QByteArray>& info_ids, QSet<QByteArray>& format_ids, QSet<QByteArray>& filter_ids, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert=false);
	void parseVcfHeader(int line_number, const QByteArray& line);
	void processVcfLine(int& line_number, const QByteArray& line, QSet<QByteArray>& info_ids, QSet<QByteArray>& format_ids, QSet<QByteArray>& filter_ids, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert=false);
	void storeLineInformation(QTextStream& stream, VcfLine line) const;

	QVector<VcfLinePtr> vcf_lines_; //variant lines
	VcfHeader vcf_header_; //all informations from header
	QVector<QByteArray> column_headers_; //heading of variant lines

	SampleIDToIdxPtr sample_id_to_idx_; //Hash of SampleID to its position
	QHash<ListOfFormatIds, FormatIDToIdxPtr> format_id_to_idx_list_; //Hash storing all possible format orders
	QHash<ListOfInfoIds, FormatIDToIdxPtr> info_id_to_idx_list_; //Hash storing all possible info orders

	//INFO/FORMAT/FILTER definition line for VCFCHECK only
	struct DefinitionLine
	{
		QByteArray id;
		QByteArray description;
		QByteArray type;
		QByteArray number;
		int used = 0;

		QByteArray toString() const
		{
			QByteArray output;
			output += "ID="+id + " ("+QByteArray::number(used)+"x used)";
			if (!type.isEmpty()) output += " Type="+type;
			if (!number.isEmpty()) output += " Number="+number;
			output += " Description="+description;

			return output;
		}
	};

	//print information line
	static void printInfo(QTextStream& out, QByteArray message)
	{
		out << message.trimmed() << "\n";
	}

	//print warning line
	static void printWarning(QTextStream& out, QByteArray message, int l, const QByteArray& line)
	{
		out << "WARNING: " << message.trimmed() <<  " - in line " << QByteArray::number(l) << ":\n" << line << "\n";
	}

	//print error line
	static void printError(QTextStream& out, QByteArray message, int l, const QByteArray& line)
	{
		out << "ERROR: " << message.trimmed() << " - in line " << QByteArray::number(l) << ":\n" << line << "\n";
	}

	//parse definition line
	static DefinitionLine parseDefinitionLine(QTextStream& out, int l, QByteArray line);

	//check number of values of a FORMAT/INFO entry
	static void checkValues(const DefinitionLine& def, const QByteArrayList& values, int alt_count, const QByteArray& sample, QTextStream& out, int l, const QByteArray& line);

	//for using the parse functions in testing
	friend class VcfLine_Test;
};
