#pragma once

#include "BedFile.h"
#include "VcfFileHelper.h"

#include <zlib.h>

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

	void leftNormalize(QString reference_genome);
	///loads a vcf or vcf.gz file
    void load(const QString& filename, bool allow_multi_sample=true, const BedFile* roi=nullptr, bool invert=false);
	///removes duplicate variants
	void removeDuplicates(bool sort_by_quality);
	///stores the data of VCFFileHandler in a vcf file
	void store(const QString& filename, bool stdout_if_file_empty = false, bool compress=false, int compression_level = 1) const;
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

	///Returns analysis type.
	AnalysisType type(bool allow_fallback_germline_single_sample = true) const;

	///returns a QVector of vcf_lines_
    const QVector<VCFLinePtr>& vcfLines() const
	{
		return vcf_lines_;
	}
	///Read-Write access to a vcf_lines_
    QVector<VCFLinePtr>& vcfLines()
	{
		return vcf_lines_;
	}
	///returns the VCFLine at pos
	const VCFLine& vcfLine(int pos) const
	{
        return *(vcf_lines_.at(pos));
	}
	///Read-Write access to a vcf_line
	VCFLine& vcfLine(int pos)
	{
        return *(vcf_lines_[pos]);
	}
	///returns the VCFLine at pos
	const VCFLine& operator[](int pos) const
	{
        return *(vcf_lines_.at(pos));
	}
	///Read-Write access to a vcf_line by operator[]
	VCFLine& operator[](int pos)
	{
        return *(vcf_lines_[pos]);
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

    ///Converts a Variant list (e.g. from a GSvar file) to a VcfFile
	static VcfFile convertGSvarToVcf(const VariantList& variant_list, const QString& reference_genome);

    ///Validates VCF file from file path
    static bool isValid(QString vcf_file_path, QString ref_file, QTextStream& out_stream, bool print_general_information = false, int max_lines = std::numeric_limits<int>::max());

    ///Returns the content of a column by index (tab-separated line) from a QByteArray line
    static QByteArray getPartByColumn(const QByteArray& line, int index);

    ///Returns string where all forbidden char of an info column value are URL encoded
    static QString encodeInfoValue(QString info_value);

    ///Returns string where all URL encoded chars of an info column value are decoded
    static QString decodeInfoValue(QString encoded_info_value);

private:

	void clear();
    void loadFromVCF(const QString& filename, bool allow_multi_sample=false, ChromosomalIndex<BedFile>* roi_idx=nullptr, bool invert=false);
    void loadFromVCFGZ(const QString& filename, bool allow_multi_sample=false, ChromosomalIndex<BedFile>* roi_idx=nullptr, bool invert=false);
	void parseHeaderFields(QByteArray& line, bool allow_multi_sample);
    void parseVcfEntry(const int line_number, QByteArray& line, QSet<QByteArray>& info_ids, QSet<QByteArray>& format_ids, QSet<QByteArray>& filter_ids, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert=false);
	void parseVcfHeader(const int line_number, QByteArray& line);
    void processVcfLine(int& line_number, QByteArray line, QSet<QByteArray>& info_ids, QSet<QByteArray>& format_ids, QSet<QByteArray>& filter_ids, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert=false);
	void storeLineInformation(QTextStream& stream, VCFLine line) const;

    QVector<VCFLinePtr> vcf_lines_; //variant lines
    VCFHeader vcf_header_; //all informations from header
    QVector<QByteArray> column_headers_; //heading of variant lines

    SampleIDToIdxPtr sample_id_to_idx; //Hash of SampleID to its position
    QHash<ListOfFormatIds, FormatIDToIdxPtr> format_id_to_idx_list; //
    QHash<ListOfInfoIds, FormatIDToIdxPtr> info_id_to_idx_list;

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
