#pragma once

#include "cppNGS_global.h"
#include "KeyValuePair.h"
#include "Exceptions.h"

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

  //insert into the ordered hash
  void insert(K key, V value) {
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
		throw std::out_of_range("key not found");
	  return iter->second;
  }

  int size() const
  {
	  return ordered_keys_.size();
  }

private:

  QList<K> ordered_keys_;
  QHash<K, V> hash_;

};

//############################################# STRUCTS FOR INFO IN HEADER
struct CPPNGSSHARED_EXPORT VcfHeaderLineBase
{
	VcfHeaderLineBase(QByteArray line):
		line_key(line){}
	VcfHeaderLineBase():
		VcfHeaderLineBase(""){}

	//string following ## in the header line: e.g. fileformat for ##fileformat=VCFv4.2
	QByteArray line_key;

	virtual void storeLine(QTextStream& stream) = 0;

};
struct CPPNGSSHARED_EXPORT VcfHeaderLine : VcfHeaderLineBase
{
	VcfHeaderLine(QByteArray line, QByteArray name):
		VcfHeaderLineBase(line), name_value(name){}
	VcfHeaderLine():
		VcfHeaderLineBase(""){}

	QByteArray name_value;

	void storeLine(QTextStream& stream)
	{
		stream << "##" << name_value << "=" << line_key << "\n";
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
	QByteArray description;

	void storeLine(QTextStream& stream)
	{
		qDebug() << "writing info line the first";
		stream << line_key << "=<ID=" << id << ",Number=" << number << ",Type=" << type << ",Description=" << description  << "\n";
	}
};
struct CPPNGSSHARED_EXPORT FilterLine : VcfHeaderLineBase
{
	FilterLine(QByteArray line):
		VcfHeaderLineBase(line){}
	FilterLine():
		VcfHeaderLineBase(""){}

	QByteArray id;
	QByteArray description;

	void storeLine(QTextStream& stream)
	{
		stream << "##FILTER=<ID=" << id << ",Description=" << description  << "\n";
	}
};

enum InfoFormatType {INFO, FORMAT};

using VcfHeaderLineBasePtr = std::shared_ptr<VcfHeaderLineBase>;
using VcfHeaderLinePtr = std::shared_ptr<VcfHeaderLine>;
using InfoFormatLinePtr = std::shared_ptr<InfoFormatLine>;
using FilterLinePtr = std::shared_ptr<FilterLine>;

///struct representing a vcf header.
/// most important information is stored in seperate variables, additional information
/// is in in 'unspecific_header_lines'

//##############################################   HEADER STRUCT
class CPPNGSSHARED_EXPORT VCFHeaderType
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

	void storeHeaderInformation(QTextStream& stream) const
	{
		foreach (VcfHeaderLineBasePtr header_line_ptr, header_line_order) {
			qDebug() << header_line_ptr->line_key;
			header_line_ptr->storeLine(stream);
		}
	}

	void setFormat(QByteArray& line)
	{
		qDebug() << __LINE__;
		file_format = std::make_shared<VcfHeaderLine>(VcfHeaderLine(parseLineInformation(line, "fileformat"), "fileformat"));
		header_line_order.push_back(file_format);
	}
	void setDate(QByteArray& line)
	{
		qDebug() << __LINE__;

		file_date = std::make_shared<VcfHeaderLine>(VcfHeaderLine(parseLineInformation(line, "fileDate"), "fileDate"));
		header_line_order.push_back(file_date);
	}
	void setSource(QByteArray& line)
	{
		file_source = std::make_shared<VcfHeaderLine>(VcfHeaderLine(parseLineInformation(line, "source"), "source"));
		header_line_order.push_back(file_source);
	}
	void setReference(QByteArray& line)
	{
		file_reference =  std::make_shared<VcfHeaderLine>(VcfHeaderLine(parseLineInformation(line, "reference"), "reference"));
		header_line_order.push_back(file_reference);
	}
	void setContig(QByteArray& line)
	{
		qDebug() << __LINE__;

		qDebug() << "set contig";
		file_contig.push_back( std::make_shared<VcfHeaderLine>(VcfHeaderLine(parseLineInformationContig(line, "contig"), "contig")));
		qDebug() << file_contig.size() << &file_contig.back() << file_contig.back()->line_key;
		header_line_order.push_back(file_contig.back());
		qDebug() << header_line_order.back()->line_key;

	}
	void setPhasing(QByteArray& line)
	{
		file_phasing = std::make_shared<VcfHeaderLine>(VcfHeaderLine(parseLineInformation(line, "phasing"), "phasing"));
		header_line_order.push_back(file_phasing);
	}
	void setInfoFormatLine(QByteArray& line, InfoFormatType type)
	{
		if(type == INFO)
		{
			line=line.mid(8);//remove "##INFO=<"
			file_info_style.push_back(std::make_shared<InfoFormatLine>(parseInfoFormatLine(line, "INFO")));
			header_line_order.push_back(file_info_style.back());
		}
		else
		{
			line=line.mid(10);//remove "##FORMAT=<"
			file_format_style.push_back(std::make_shared<InfoFormatLine>(parseInfoFormatLine(line, "FORMAT")));
			header_line_order.push_back(file_format_style.back());
		}
	}

private:
	//Contaimner storing the order of header information
	QVector<VcfHeaderLineBasePtr> header_line_order;

	QByteArray parseLineInformation(QByteArray line, const QByteArray& information)
	{
		QList<QByteArray> splitted_line=line.split('=');
		if (splitted_line.count()<2)
		{
			THROW(FileParseException, "Malformed " + information + " line " + line.trimmed());
		}
		return splitted_line[1];
	}
	QByteArray parseLineInformationContig(QByteArray line, const QByteArray& information)
	{
		QList<QByteArray> splitted_line=line.split('=');
		if (splitted_line.count()<2)
		{
			THROW(FileParseException, "Malformed " + information + " line " + line.trimmed());
		}
		for(int i = 0; i < splitted_line.size(); ++i)
		{
			splitted_line[1].append(splitted_line[i]);
		}
		return splitted_line[1];
	}
	InfoFormatLine parseInfoFormatLine(QByteArray& line, QByteArray type)
	{
		InfoFormatLine info_format_line = InfoFormatLine(type);
		QList <QByteArray> comma_splitted_line=line.split(',');
		if (comma_splitted_line.count()<4)
		{
			THROW(FileParseException, "Malformed " + type + " line: has less than 4 entries " + line.trimmed());
		}

		//parse ID field
		QByteArray ID_entry=comma_splitted_line[0];
		QList <QByteArray> splitted_ID_entry=ID_entry.split('=');
		if (!(splitted_ID_entry[0].startsWith("ID")))
		{
			THROW(FileParseException, "Malformed " + type + " line: does not start with ID-field " + splitted_ID_entry[0] + "'");
		}
		info_format_line.id = ID_entry;
		comma_splitted_line.pop_front();//pop ID-field

		return info_format_line;
	}
};

///representation of a line of a vcf file
//##############################################   VCFLINE STRUCT

struct VCFLineType
{
	Chromosome chr;
	int pos;
	Sequence ref;
	QVector<Sequence> alt; //comma seperated list of alternative sequences

	QByteArrayList id; //; seperated list of id-strings
	int qual;

	//BETTER: elements are of filter_type
	QByteArrayList filter; //; seperated list of failed filters or "PASS"
	//BETTER: key is info_type
	QHash<QString, QString> info;

	//obligatory columns
	//BETTER: elements are of format_type
	QByteArrayList format; //: seperated list of ids for sample
	OrderedHash<QByteArray, QByteArray> sample; // hash of format entries to values

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

