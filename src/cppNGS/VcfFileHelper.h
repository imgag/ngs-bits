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

///struct representing a vcf header.
/// most important information is stored in seperate variables, additional information
/// is in in 'unspecific_header_lines'

class CPPNGSSHARED_EXPORT VCFHeader
{
public:
	QByteArray fileformat;
	QVector<VcfHeaderLine> file_comments;

	QVector<InfoFormatLine> info_lines;
	QVector<FilterLine> filter_lines;
	QVector<InfoFormatLine> format_lines;

	static const int MIN_COLS = 8;

	void storeHeaderInformation(QTextStream& stream) const;

	void setFormat(QByteArray& line);
	void setInfoFormatLine(QByteArray& line, InfoFormatType type, const int line_number);
	void setFilterLine(QByteArray& line, const int line_number);
	void setCommentLine(QByteArray& line, const int line_number);

private:

	static const QByteArrayList InfoTypes;
	static const QByteArrayList FormatTypes;

	bool parseInfoFormatLine(QByteArray& line,InfoFormatLine& info_format_line, QByteArray type, const int line_number);
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

	//returns if the chromosome is valid
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

	//returns all not passed filters
	QByteArrayList failedFilters()
	{
		QByteArrayList filters;
		foreach(QByteArray tag, filter)
		{
			tag = tag.trimmed();

			if (tag!="" && tag!="." && tag.toUpper()!="PASS" && tag.toUpper()!="PASSED")
			{
				filters.append(tag);
			}
		}
		return filters;
	}

	void checkValid() const
	{
		if (!chr.isValid())
		{
			THROW(ArgumentException, "Invalid variant chromosome string in variant '" + chr.str() + " " + QString::number(pos));
		}

		if (pos < 1 || ref.length() < 1)
		{
			THROW(ArgumentException, "Invalid variant position range in variant '" +  chr.str() + " " + QString::number(pos));
		}

		if (ref!="-" && !QRegExp("[ACGTN]+").exactMatch(ref))
		{
			THROW(ArgumentException, "Invalid variant reference sequence in variant '" +  chr.str() + " " + QString::number(pos));
		}
		for(Sequence alt_seq : alt)
		{
			if (alt_seq!="-" && alt_seq!="." && !QRegExp("[ACGTN,]+").exactMatch(alt_seq))
			{
				THROW(ArgumentException, "Invalid variant alternative sequence in variant '" +  chr.str() + " " + QString::number(pos));
			}
		}
	}

};



