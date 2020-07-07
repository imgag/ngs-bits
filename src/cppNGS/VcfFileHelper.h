#pragma once

#include "cppNGS_global.h"
#include "KeyValuePair.h"
#include "Exceptions.h"
#include "Log.h"

#include "ChromosomalIndex.h"
#include "Sequence.h"

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
  V operator[](K key) const
  {
	  auto iter = hash_.find(key);
	  if (iter == hash_.end())
		THROW(ArgumentException, "Key" + key + "not found");
	  return iter.value();
  }

  //check if key exists
  bool hasKey(K key, V& value) const
  {
	  auto iter = hash_.find(key);
	  if (iter == hash_.end())
		return false;
	  value = iter.value();
	  return true;
  }

  //access value by order
  std::pair<K, V> at(int i) const
  {
	  K key = ordered_keys_.at(i);
	  return std::make_pair(key, hash_[key]);
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

enum InfoFormatType {INFO, FORMAT};

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

const QByteArray& strToPointer(const QByteArray& str);
const QChar* strToPointer(const QString& str);

///Supported analysis types
enum AnalysisType
{
	GERMLINE_SINGLESAMPLE,
	GERMLINE_TRIO,
	GERMLINE_MULTISAMPLE,
	SOMATIC_SINGLESAMPLE,
	SOMATIC_PAIR
};

using FormatToValueHash = OrderedHash<QByteArray, QByteArray>;

///struct representing a vcf header.
/// most important information is stored in seperate variables, additional information
/// is in in 'unspecific_header_lines'
class CPPNGSSHARED_EXPORT VCFHeader
{
public:
	QByteArray fileformat_;
	QVector<VcfHeaderLine> file_comments_;

	QVector<InfoFormatLine> info_lines_;
	QVector<FilterLine> filter_lines_;
	QVector<InfoFormatLine> format_lines_;

	static const int MIN_COLS = 8;

	void storeHeaderInformation(QTextStream& stream) const;

	void setFormat(QByteArray& line);
	void setInfoFormatLine(QByteArray& line, InfoFormatType type, const int line_number);
	void setFilterLine(QByteArray& line, const int line_number);
	void setCommentLine(QByteArray& line, const int line_number);
	AnalysisType type(bool allow_fallback_germline_single_sample) const;

private:

	static const QByteArrayList InfoTypes;
	static const QByteArrayList FormatTypes;

	bool parseInfoFormatLine(QByteArray& line,InfoFormatLine& info_format_line, QByteArray type, const int line_number);
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
	const Sequence& ref() const
	{
		return ref_;
	}
	const QVector<Sequence>& alt() const
	{
		return alt_;
	}
	const QByteArrayList& id() const
	{
		return id_;
	}
	const int& qual() const
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
	const OrderedHash<QByteArray , QByteArray>& info() const
	{
		return info_;
	}
	QByteArray info(const QByteArray& key) const
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
	//const OrderedHash<QByteArray, FormatToValueHash>& sample() const
	//{
	//	return sample_;
	//}
	//FormatToValueHash sample(const QByteArray& sample_name) const
	//{
	//	return sample_[sample_name];
	//}
	const QVector<FormatToValueHash>& sample() const
	{
		return sample_;
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
		for(const Sequence& seq : alt)
		{
			alt_.push_back(strToPointer(seq));
		}
	}
	void setId(const QByteArrayList& id)
	{
		id_ = id;
	}
	void setQual(const int& qual)
	{
		qual_ = qual;
	}
	void setFilter(const QByteArrayList& filter_list)
	{
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
	//void setSample(const OrderedHash<QByteArray, FormatToValueHash>& sample)
	//{
	//	sample_ = sample;
	//}
	void setSample(const QVector<FormatToValueHash>& sample)
	{
		sample_ = sample;
	}

	//returns if the chromosome is valid
	bool isValidGenomicPosition() const;
	//returns all not passed filters
	QByteArrayList failedFilters();
	void checkValid() const;

private:
	Chromosome chr_;
	int pos_;
	Sequence ref_;
	QVector<Sequence> alt_; //comma seperated list of alternative sequences

	QByteArrayList id_; //; seperated list of id-strings
	int qual_;

	QByteArrayList filter_; //; seperated list of failed filters or "PASS"
	OrderedHash<QByteArray , QByteArray> info_; //; seperated list of info key=value pairs

	//obligatory columns
	QByteArrayList format_; //: seperated list of formats for each sample
	//OrderedHash<QByteArray, FormatToValueHash> sample_; // hash of a sample name to a hash of format entries to values
	QVector<FormatToValueHash> sample_;
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

} //end namespace VcfFormat
