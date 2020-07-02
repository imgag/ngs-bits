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

template<typename K, typename V>
class CPPNGSSHARED_EXPORT OrderedHash {

public:

  //add key=value pair to the end of OrderedHash
  void push_back(K key, V value) {
	auto iter = hash_.find(key);
	if (iter != hash_.end()) {
	  return;
	}

	ordered_keys_.push_back(key);
	hash_.insert(key, value);
  }

  //access value by key
  V operator[](K key) const {
	  auto iter = hash_.find(key);
	  if (iter == hash_.end())
		THROW(ArgumentException, "Key" + key + "not found");
	  return iter->second;
  }

  //access value by order
  std::pair<K, V> at(int i)
  {
	  K key = ordered_keys_.at(i);
	  return std::make_pair(key, hash_[key]);
  }

  //get the number of inserted key=value pairs
  int size() const
  {
	  return ordered_keys_.size();
  }

private:

  QVector<K> ordered_keys_;
  QHash<K, V> hash_;

};

struct CPPNGSSHARED_EXPORT VcfHeaderLineBase
{
	VcfHeaderLineBase(QByteArray line);
	VcfHeaderLineBase():
		VcfHeaderLineBase(""){}

	//string following ## in the header line: e.g. fileformat for ##fileformat=VCFv4.2
	QByteArray line_key;

	virtual void storeLine(QTextStream& stream) = 0;

};
struct CPPNGSSHARED_EXPORT VcfHeaderLine : VcfHeaderLineBase
{
	VcfHeaderLine(QByteArray line, QByteArray name);
	VcfHeaderLine():
		VcfHeaderLineBase(""){}

	QByteArray name_value;

	void storeLine(QTextStream& stream)
	{
		stream << "##" << line_key << "=" << name_value << "\n";
	}
};
struct CPPNGSSHARED_EXPORT InfoFormatLine : VcfHeaderLineBase
{
	InfoFormatLine(QByteArray line):
		VcfHeaderLineBase(line){}
	InfoFormatLine():
		VcfHeaderLineBase(""){}

	QByteArray id;
	QByteArray number;
	QByteArray type;
	QString description;

	void storeLine(QTextStream& stream)
	{
		stream << line_key << "=<ID=" << id << ",Number=" << number << ",Type=" << type << ",Description=\"" << description << "\">" << "\n";
	}
};
struct CPPNGSSHARED_EXPORT FilterLine : VcfHeaderLineBase
{
	FilterLine(QByteArray line):
		VcfHeaderLineBase(line){}
	FilterLine():
		VcfHeaderLineBase(""){}

	QByteArray id;
	QString description;

	void storeLine(QTextStream& stream)
	{
		stream << "##FILTER=<ID=" << id << ",Description=\"" << description  << "\">" << "\n";
	}
};

enum InfoFormatType {INFO, FORMAT};

using VcfHeaderLineBasePtr = std::shared_ptr<VcfHeaderLineBase>;
using VcfHeaderLinePtr = std::shared_ptr<VcfHeaderLine>;
using InfoFormatLinePtr = std::shared_ptr<InfoFormatLine>;
using FilterLinePtr = std::shared_ptr<FilterLine>;

const QByteArray& strToPointer(const QByteArray& str);
const QChar* strToPointer(const QString& str);

///struct representing a vcf header.
/// most important information is stored in seperate variables, additional information
/// is in in 'unspecific_header_lines'

class CPPNGSSHARED_EXPORT VCFHeader
{
public:
	VcfHeaderLinePtr file_format;
	VcfHeaderLinePtr file_date;
	VcfHeaderLinePtr file_source;
	VcfHeaderLinePtr file_reference;
	VcfHeaderLinePtr file_phasing;
	QVector<VcfHeaderLinePtr> file_contig;

	QVector<VcfHeaderLinePtr> unspecific_header_lines;

	QVector<InfoFormatLinePtr> file_info_style;
	QVector<FilterLinePtr> file_filter_style;
	QVector<InfoFormatLinePtr> file_format_style;

	static const int MIN_COLS = 8;
	QVector<QByteArray> columns;

	void storeHeaderInformation(QTextStream& stream) const;

	void setFormat(QByteArray& line);
	void setDate(QByteArray& line);
	void setSource(QByteArray& line);
	void setReference(QByteArray& line);
	void setContig(QByteArray& line);
	void setPhasing(QByteArray& line);
	void setInfoFormatLine(QByteArray& line, InfoFormatType type, const int line_number);
	void setFilterLine(QByteArray& line, const int line_number);
	void setUnspecificLine(QByteArray& line, const int line_number);

private:
	//vector storing the order of header information
	QVector<VcfHeaderLineBasePtr> header_line_order;

	static const QByteArrayList InfoTypes;
	static const QByteArrayList FormatTypes;

	QByteArray parseLineInformation(QByteArray line, const QByteArray& information);
	QByteArray parseLineInformationContig(QByteArray line, const QByteArray& information);
	InfoFormatLinePtr parseInfoFormatLine(QByteArray& line, QByteArray type, const int line_number);
	FilterLinePtr parseFilterLine(QByteArray& line, const int line_number);

};

///representation of a line of a vcf file
struct VCFLine
{
	Chromosome chr;
	int pos;
	Sequence ref;
	QVector<Sequence> alt; //comma seperated list of alternative sequences

	QByteArrayList id; //; seperated list of id-strings
	int qual;

	QByteArrayList filter; //; seperated list of failed filters or "PASS"
	OrderedHash<QByteArray , QByteArray> info; //; seperated list of info key=value pairs

	//obligatory columns
	QByteArrayList format; //: seperated list of ids for sample
	QVector<OrderedHash<QByteArray, QByteArray>> sample; // hash of format entries to values

	//Returns if the chromosome is valid
	bool isValidGenomicPosition() const
	{
		bool is_valid_ref_base = true;
		for(int i = 0; i < this->ref.size(); ++i)
		{
			if(ref.at(i) != 'A' && ref.at(i) != 'C' && ref.at(i) != 'G' && ref.at(i) != 'T' && ref.at(i) != 'N' &&
			   ref.at(i) != 'a' && ref.at(i) != 'c' && ref.at(i) != 'g' && ref.at(i) != 't' && ref.at(i) != 'n')
			{
				is_valid_ref_base = false;
				break;
			}
		}
		return chr.isValid() && is_valid_ref_base && pos>=0 && ref.size()>=0 && !ref.isEmpty() && !alt.isEmpty();
	}

};



