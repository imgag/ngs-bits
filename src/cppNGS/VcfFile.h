#pragma once

#include "BedFile.h"
#include "VcfLine.h"
#include "KeyValuePair.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "htslib/bgzf.h"

#include <zlib.h>

#define BGZF_NO_COMPRESSION         10
#define BGZF_GZIP_COMPRESSION		0
#define BGZF_BEST_SPEED             1
#define BGZF_BEST_COMPRESSION       9

///Handling of VCF and VCF.GZ files
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
	///Returns a QList of all sample names
	const QByteArrayList& sampleIDs() const
	{
		return sample_names_;
	}
	///Sets the sample names (also for all lines) - this is only needed if you construct VCF files manually
	void setSampleNames(const QByteArrayList& names)
	{
		sample_names_ = names;

		for (int i=0; i<vcf_lines_.count(); ++i)
		{
			vcf_lines_[i].setSamplNames(names);
		}
	}

	///Left-normalize every VCF line in the vcf file according to a reference genome
	void leftNormalize(QString reference_genome);
	///Right-normalize every VCF line in the vcf file according to a reference genome
	void rightNormalize(QString reference_genome);
	///Load a VCF file
	void load(const QString& filename, bool allow_multi_sample = true);
	///Load part of a VCF file defied by a region (inside the region of invert=false and outside the region otherwise)
	void load(const QString& filename, const BedFile& roi,  bool allow_multi_sample = true, bool invert = false);
	///removes duplicate variants
	void removeDuplicates(bool sort_by_quality);
	///Stores the data in a file
	void store(const QString& filename, bool stdout_if_file_empty = false, int compression_level = BGZF_NO_COMPRESSION) const;
	///Stores a VCF file as a TSV representaton
	void storeAsTsv(const QString& filename);
	///Sort according to chr/postion.
	void sort(bool use_quality = false);
	///Sort according to chr/postion - chromosome order is taken from the given FAI file.
	void sortByFile(QString fai_file);

	///Returns the VCF line at the given position
	const VcfLine& operator[](int index) const
	{
		return vcf_lines_[index];
	}
	///Returns a VCF file that can be modified
	VcfLine& operator[](int index)
	{
		return vcf_lines_[index];
	}

	///Append a VCF line
	void append(const VcfLine& line)
	{
		vcf_lines_ << line;
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

	///Restrict to a certein number of lines.
	void restrictTo(int size)
	{
		while (vcf_lines_.count() > size) vcf_lines_.pop_back();
	}

	///Copies meta data from a VcfFile (comment, header, column headers), but not the variants. Should be used to subset a VcfFile, not to add entirely new variants.
	void copyMetaData(const VcfFile& rhs);

	///Returns VCF content as string
    QByteArray toText() const;
	///Reads a VCF from a string
    void fromText(const QByteArray& text);

	///Converts a variant list in GSvar format to a VCF file
	static VcfFile fromGSvar(const VariantList& variant_list, const QString& reference_genome);

	///Validates a VCF file
	static bool isValid(QString filename, QString ref_file, QTextStream& out_stream, bool print_general_information = false, int max_lines = std::numeric_limits<int>::max());

	///Returns string where all forbidden char of an INFO column value are URL encoded
	static QString encodeInfoValue(QString info_value);

	///Returns string where all URL encoded chars of an INFO column value are decoded
	static QString decodeInfoValue(QString encoded_info_value);

	///Remove contig headers that are not unsed.
	void removeUnusedContigHeaders();

private:
	void storeHeaderColumns(QTextStream& stream) const;
	void clear();
	void loadFromVCFGZ(const QString& filename, bool allow_multi_sample = true, ChromosomalIndex<BedFile>* roi_idx = nullptr, bool invert = false);
	void parseHeaderFields(const QByteArray& line, bool allow_multi_sample);
	void parseVcfEntry(int line_number, const QByteArray& line, QSet<QByteArray>& info_ids, QSet<QByteArray>& format_ids, QSet<QByteArray>& filter_ids, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert=false);
	void parseVcfHeader(int line_number, const QByteArray& line);
	void processVcfLine(int& line_number, const QByteArray& line, QSet<QByteArray>& info_ids, QSet<QByteArray>& format_ids, QSet<QByteArray>& filter_ids, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert=false);
	void storeLineInformation(QTextStream& stream, const VcfLine& line) const;

	QList<VcfLine> vcf_lines_; //variant lines
	VcfHeader vcf_header_; //all informations from header
	QByteArrayList sample_names_;

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

	//costum sorting of variants
	template <typename T>
	void sortCustom(const T& comparator)
	{
		std::sort(vcf_lines_.begin(), vcf_lines_.end(), comparator);
	}

	//Comparator helper class that used by sort().
	class LessComparator
	{
	public:
		///Constructor.
		LessComparator(bool use_quality);
		bool operator()(const VcfLine& a, const VcfLine& b) const;

	private:
		bool use_quality;
	};
	//Comparator helper class used by sortByFile.
	class LessComparatorByFile
	{
	public:
		//Constructor with FAI file, which determines the chromosome order.
		LessComparatorByFile(QString fai_file);
		bool operator()(const VcfLine& a, const VcfLine& b) const;

	private:
		QString filename_;
		QHash<int, int> chrom_rank_;
	};
	//for using the parse functions in testing
	friend class VcfLine_Test;

	//storing all QByteArrays in a list of unique QByteArrays
	static const QByteArray& strCache(const QByteArray& str);
	static const QByteArrayList& strArrayCache(const QByteArrayList& str);
};
