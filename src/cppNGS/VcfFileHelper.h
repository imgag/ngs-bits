#pragma once

#include "cppNGS_global.h"
#include "KeyValuePair.h"
#include "Exceptions.h"
#include "Log.h"

#include "ChromosomalIndex.h"
#include "Sequence.h"
#include "BedFile.h"

#include <QIODevice>
#include <QString>
#include <QMap>
#include <QTextStream>
#include <QByteArray>
#include <QList>

#include <memory>

namespace VcfFormat
{

template<typename K, typename V>
class CPPNGSSHARED_EXPORT OrderedHash {

public:

  //add key=value pair to the end of OrderedHash
  void push_back(K key, V value)
  {
	auto iter = hash_.find(key);
	if (iter != hash_.end()) {
	  return;
	}

	ordered_keys_.push_back(key);
	hash_.insert(key, value);
  }

  //access value by key
  const V& operator[](K key) const
  {
	  auto iter = hash_.find(key);
	  if (iter == hash_.end())
	  {
		THROW(ArgumentException, "Key " + key + " not found");
	  }
	  return iter.value();
  }

  //check if key exists
  bool hasKey(K key, V& value) const
  {
	  auto iter = hash_.find(key);
	  if (iter == hash_.end()) return false;
	  value = iter.value();
	  return true;
  }

  //access value by order
  typename QHash<K, V>::const_iterator at(int i) const
  {
	  if(i >= size())
	  {
		  THROW(ArgumentException, "Index " + QString::number(i) + " is out of range for OrderedHash of size " + size());
	  }
	  K key = ordered_keys_.at(i);
	  return hash_.find(key);
  }

  //get the number of inserted key=value pairs
  int size() const
  {
	  return ordered_keys_.size();
  }

  //check if the OrderedHash is empty
  bool empty() const
  {
	  return (ordered_keys_.empty());
  }

private:

  QVector<K> ordered_keys_;
  QHash<K, V> hash_;

};

const QByteArray& strToPointer(const QByteArray& str);
const QChar* strToPointer(const QString& str);

enum InfoFormatType {INFO, FORMAT};
using FormatIDToValueHash = OrderedHash<QByteArray, QByteArray>;
///Supported analysis types
enum AnalysisType
{
	GERMLINE_SINGLESAMPLE,
	GERMLINE_TRIO,
	GERMLINE_MULTISAMPLE,
	SOMATIC_SINGLESAMPLE,
	SOMATIC_PAIR
};

struct CPPNGSSHARED_EXPORT VcfHeaderLine
{

	QByteArray value;
	QByteArray key;

	void storeLine(QTextStream& stream)
	{
		stream << "##" << key << "=" << value << "\n";
	}
};
struct CPPNGSSHARED_EXPORT InfoFormatLine
{
	QByteArray id;
	QByteArray number;
	QByteArray type;
	QString description;

	void storeLine(QTextStream& stream, InfoFormatType line_type)
	{
		line_type==InfoFormatType::INFO ? stream << "##INFO" : stream << "##FORMAT";
		stream << "=<ID=" << id << ",Number=" << number << ",Type=" << type << ",Description=\"" << description << "\">" << "\n";
	}
};
struct CPPNGSSHARED_EXPORT FilterLine
{
	QByteArray id;
	QString description;

	void storeLine(QTextStream& stream)
	{
		stream << "##FILTER=<ID=" << id << ",Description=\"" << description  << "\">" << "\n";
	}
};

///struct representing a vcf header.
/// most important information is stored in seperate variables, additional information
/// is in in 'unspecific_header_lines'
class CPPNGSSHARED_EXPORT VCFHeader
{
public:

	const QByteArray& fileFormat() const
	{
		return fileformat_;
	}
	const QVector<VcfHeaderLine>& comments() const
	{
		return 	file_comments_;
	}
	const QVector<InfoFormatLine>& infoLines() const
	{
		return info_lines_;
	}
	const QVector<FilterLine>& filterLines() const
	{
		return filter_lines_;
	}
	const QVector<InfoFormatLine>& formatLines() const
	{
		return format_lines_;
	}

	void addInfoLine(const InfoFormatLine& info_line)
	{
		info_lines_.push_back(info_line);
	}
	void addFormatLine(const InfoFormatLine& format_line)
	{
		format_lines_.push_back(format_line);
	}
	void moveFormatLine(int from, int to)
	{
		format_lines_.move(from, to);
	}

	static const int MIN_COLS = 8;

	void storeHeaderInformation(QTextStream& stream) const;

	InfoFormatLine infoLineByID(const QByteArray& id, bool error_not_found = true) const;
	InfoFormatLine formatLineByID(const QByteArray& id, bool error_not_found = true) const;
	FilterLine filterLineByID(const QByteArray& id, bool error_not_found = true) const;
	int vepIndexByName(const QString& name, bool error_if_not_found) const;

	void setFormat(QByteArray& line);
	void setInfoFormatLine(QByteArray& line, InfoFormatType type, const int line_number);
	void setFilterLine(QByteArray& line, const int line_number);
	void setCommentLine(QByteArray& line, const int line_number);
	AnalysisType type(bool allow_fallback_germline_single_sample) const;
	void clear();

private:

	static const QByteArrayList InfoTypes;
	static const QByteArrayList FormatTypes;

	QByteArray fileformat_;
	QVector<VcfHeaderLine> file_comments_;

	QVector<InfoFormatLine> info_lines_;
	QVector<FilterLine> filter_lines_;
	QVector<InfoFormatLine> format_lines_;

	bool parseInfoFormatLine(QByteArray& line,InfoFormatLine& info_format_line, QByteArray type, const int line_number);
	InfoFormatLine lineByID(const QByteArray& id, const QVector<InfoFormatLine>& lines, bool error_not_found = true) const;
};

///representation of a line of a vcf file
class CPPNGSSHARED_EXPORT  VCFLine
{

public:

	const Chromosome& chr() const
	{
		return chr_;
	}
	const int& pos() const
	{
		return pos_;
	}
	const int& start() const
	{
		return pos_;
	}
	const Sequence& ref() const
	{
		return ref_;
	}
	int end() const
	{
		return (pos() + ref().length() - 1);
	}
	const QVector<Sequence>& alt() const
	{
		return alt_;
	}
	const QByteArray& altString() const
	{
		return alt_string_;
	}
	const Sequence& alt(int pos) const
	{
		return alt_.at(pos);
	}
	const QByteArrayList& id() const
	{
		return id_;
	}
	const double& qual() const
	{
		return qual_;
	}
	const QByteArrayList& filter() const
	{
		return filter_;
	}
	const QByteArrayList format() const
	{
		return format_;
	}
	const OrderedHash<QByteArray , QByteArray>& infos() const
	{
		return info_;
	}
	QByteArray info(const QByteArray& key, bool error_if_key_absent = false) const
	{
		if(error_if_key_absent)
		{
			return info_[key];
		}
		else
		{
			QByteArray value;
			if(info_.hasKey(key, value))
			{
				return value;
			}
			else
			{
				return "";
			}
		}
	}

	///returns samples as a hash of SAMPLE ID to a hash of FORMAT ID to value
	const OrderedHash<QByteArray, FormatIDToValueHash>& samples() const
	{
		return sample_;
	}
	///access the hash of FORMAT ID to value for a sample by SAMPLE ID
	const FormatIDToValueHash& sample(const QByteArray& sample_name) const
	{
		return sample_[sample_name];
	}
	///access the hash of FORMAT ID to value for a sample by position
	const FormatIDToValueHash& sample(int pos) const
	{
		if(pos >= samples().size()) THROW(ArgumentException, QString::number(pos) + " is out of range for SAMPLES. The VCF file provides " + QString::number(samples().size()) + " SAMPLES");
		return sample_.at(pos).value();
	}
	///access the value for a SAMPLE and FORMAT ID
	// by purpose does not return a reference because an empty QByteArray can be returned for non-existing keys
	//(use case: a specific FORMAT key is absent in some vcf lines)
	QByteArray sample(const QByteArray& sample_name, const QByteArray& format_key, bool error_if_format_key_absent = false) const
	{
		FormatIDToValueHash hash = sample_[sample_name];
		if(error_if_format_key_absent)
		{
			return hash[format_key];
		}
		else
		{
			QByteArray value;
			if(hash.hasKey(format_key, value))
			{
				return value;
			}
			else
			{
				return "";
			}
		}
	}
	///access the value for a SAMPLE position and FORMAT ID
	// by purpose does not return a reference because an empty QByteArray can be returned for non-existing keys
	//(use case: a specific FORMAT key is absent in some vcf lines)
	QByteArray sample(int sample_pos, const QByteArray& format_key, bool error_if_format_key_absent = false) const
	{
		if(sample_pos >= samples().size()) THROW(ArgumentException, QString::number(sample_pos) + " is out of range for SAMPLES. The VCF file provides " + QString::number(samples().size()) + " SAMPLES");
		FormatIDToValueHash hash = sample_.at(sample_pos).value();

		if(error_if_format_key_absent)
		{
			return hash[format_key];
		}
		else
		{
			QByteArray value;
			if(hash.hasKey(format_key, value))
			{
				return value;
			}
			else
			{
				return "";
			}
		}
	}

	void setChromosome(const Chromosome& chr)
	{
		chr_ =  chr;
	}
	void setPos(const int& pos)
	{
		pos_ = pos;
	}
	void setRef(const Sequence& ref)
	{
		ref_ = ref;
	}
	void setAlt(const QByteArrayList& alt)
	{
		alt_string_ = strToPointer(alt.join(","));
		for(const Sequence& seq : alt)
		{
			alt_.push_back(strToPointer(seq.toUpper()));
		}
	}
	void setId(const QByteArrayList& id)
	{
		id_ = id;
	}
	void setQual(const double& qual)
	{
		qual_ = qual;
	}
	void setFilter(const QByteArrayList& filter_list)
	{
		if(filter_list.size() == 1 && filter_list.at(0) == ".")
		{
			return;
		}
		for(const QByteArray& filter : filter_list)
		{
			filter_.push_back(strToPointer(filter));
		}
	}
	void setInfo(const OrderedHash<QByteArray , QByteArray>& info)
	{
		info_ = info;
	}
	void setFormat(const QByteArrayList& format)
	{
		format_ = format;
	}
	void setSample(const OrderedHash<QByteArray, FormatIDToValueHash>& sample)
	{
		sample_ = sample;
	}

	///Overlap check for chromosome and position range.
	bool overlapsWith(const Chromosome& input_chr, int input_start, int input_end) const
	{
		return (chr_==input_chr && BasicStatistics::rangeOverlaps(pos(), end(), input_start, input_end));
	}
	///Overlap check for position range only.
	bool overlapsWith(int input_start, int input_end) const
	{
		return BasicStatistics::rangeOverlaps(pos(), end(), input_start, input_end);
	}
	///Overlap check BED file line.
	bool overlapsWith(const BedLine& line) const
	{
		return overlapsWith(line.chr(), line.start(), line.end());
	}

	//returns if the chromosome is valid
	bool isValidGenomicPosition() const;
	//returns all not passed filters
	QByteArrayList failedFilters() const;
	void checkValid() const;
	void storeLineInformation(QTextStream& stream) const;
	QString toString() const;
	///Returns if the variant is a SNV
	bool isSNV() const
	{
		return alt(0).length()==1 && ref_.length()==1 && alt(0)!="-" && ref_!="-";
	}
	QByteArrayList vepAnnotations(int field_index) const;

private:
	Chromosome chr_;
	int pos_;
	Sequence ref_;
	QVector<Sequence> alt_; //comma seperated list of alternative sequences
	QByteArray alt_string_;

	QByteArrayList id_; //; seperated list of id-strings
	double qual_;

	QByteArrayList filter_; //; seperated list of failed filters or "PASS"
	OrderedHash<QByteArray , QByteArray> info_; //; seperated list of info key=value pairs

	//obligatory columns
	QByteArrayList format_; //: seperated list of formats for each sample
	OrderedHash<QByteArray, FormatIDToValueHash> sample_; // hash of a sample name to a hash of format entries to values
};

///Comparator helper class that used by sort().
class LessComparator
{
	public:
		///Constructor.
		LessComparator(bool use_quality);
		bool operator()(const VCFLine& a, const VCFLine& b) const;

	private:
		bool use_quality;
};
///Comparator helper class used by sortByFile.
class LessComparatorByFile
{
	public:
		///Constructor with FAI file, which determines the chromosome order.
		LessComparatorByFile(QString filename);
		bool operator()(const VCFLine& a, const VCFLine& b) const;

	private:
		QString filename_;
		QHash<int, int> chrom_rank_;
};

} //end namespace VcfFormat
