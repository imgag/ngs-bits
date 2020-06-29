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
			header_line_ptr->storeLine(stream);
		}
	}

	void setFormat(QByteArray& line)
	{
		file_format = std::make_shared<VcfHeaderLine>(VcfHeaderLine(parseLineInformation(line, "fileformat"), "fileformat"));
		header_line_order.push_back(file_format);
	}
	void setDate(QByteArray& line)
	{
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
		file_reference = std::make_shared<VcfHeaderLine>(VcfHeaderLine(parseLineInformation(line, "reference"), "reference"));
		header_line_order.push_back(file_reference);
	}
	void setContig(QByteArray& line)
	{
		file_contig.push_back(std::make_shared<VcfHeaderLine>(VcfHeaderLine(parseLineInformationContig(line, "contig"), "contig")));
		header_line_order.push_back(file_contig.back());
	}
	void setPhasing(QByteArray& line)
	{
		file_phasing = std::make_shared<VcfHeaderLine>(VcfHeaderLine(parseLineInformation(line, "phasing"), "phasing"));
		header_line_order.push_back(file_phasing);
	}
	void setInfoFormatLine(QByteArray& line, InfoFormatType type, const int line_number)
	{
		if(type == INFO)
		{
			line=line.mid(8);//remove "##INFO=<"
			InfoFormatLinePtr info_line = parseInfoFormatLine(line, "INFO", line_number);
			if(info_line)
			{
				file_info_style.push_back(info_line);
				header_line_order.push_back(file_info_style.back());
			}
		}
		else
		{
			line=line.mid(10);//remove "##FORMAT=<"
			InfoFormatLinePtr format_line = parseInfoFormatLine(line, "FORMAT", line_number);
			if(format_line)
			{
				file_format_style.push_back(format_line);
				header_line_order.push_back(file_format_style.back());
				//make sure the "GT" format field is always the first format field
				if(format_line->id == "GT" && file_format_style.size() > 1)
				{
					header_line_order.move(header_line_order.count()-1, header_line_order.count() - file_format_style.count());
				}
			}
		}
	}
	void setFilterLine(QByteArray& line, const int line_number)
	{
		file_filter_style.push_back(std::make_shared<FilterLine>(parseFilterLine(line, line_number)));
		header_line_order.push_back(file_filter_style.back());
	}

private:
	//Contaimner storing the order of header information
	QVector<VcfHeaderLineBasePtr> header_line_order;

	static const QByteArrayList InfoTypes;
	static const QByteArrayList FormatTypes;

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
		for(int i = 2; i < splitted_line.size(); ++i)
		{
			splitted_line[1].append("=").append(splitted_line[i]);
		}
		return splitted_line[1];
	}
	InfoFormatLinePtr parseInfoFormatLine(QByteArray& line, QByteArray type, const int line_number)
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
			THROW(FileParseException, "Malformed " + type + " line: does not start with ID-field " + splitted_ID_entry[0]);
		}
		ID_entry = ID_entry.split('=')[1];
		info_format_line.id = ID_entry;
		comma_splitted_line.pop_front();//pop ID-field
		//parse number field
		QByteArray number_entry=comma_splitted_line.first();
		QList <QByteArray> splitted_number_entry=number_entry.split('=');
		if (!(splitted_number_entry[0].trimmed().startsWith("Number")))
		{
			THROW(FileParseException, "Malformed " + type + " line: second field is not a number field " + splitted_number_entry[0]);
		}
		info_format_line.number = splitted_number_entry[1];
		comma_splitted_line.pop_front();//pop number-field
		//parse type field
		QList <QByteArray> splitted_type_entry=comma_splitted_line.first().split('=');
		if (splitted_type_entry[0].trimmed()!="Type")
		{
			THROW(FileParseException, "Malformed " + type + " line: third field is not a type field " + line.trimmed() + "'");
		}
		QByteArray s_type=splitted_type_entry[1];
		if ( (type == "INFO" && !(InfoTypes.contains(s_type))) || (type == "FORMAT" &&  !(FormatTypes.contains(s_type))) )
		{
			THROW(FileParseException, "Malformed " + type +" line: undefined value for type " + line.trimmed() + "'");
		}
		info_format_line.type = s_type;
		comma_splitted_line.pop_front();//pop type-field
		//parse description field
		QByteArray description_entry=comma_splitted_line.front();
		QList <QByteArray> splitted_description_entry=description_entry.split('=');
		if (splitted_description_entry[0].trimmed()!="Description")
		{
			THROW(FileParseException, "Malformed " + type + " line: fourth field is not a description field " + line.trimmed());
		}
		//ugly, but because the description may content commas, too...
		comma_splitted_line.pop_front();//pop type-field
		comma_splitted_line.push_front(splitted_description_entry[1]);//re-add description value between '=' and possible ","
		QStringList description_value_parts;//convert to QStringList
		for(int i=0; i<comma_splitted_line.size(); ++i)
		{
			description_value_parts.append(comma_splitted_line[i]);
		}
		QString description_value=description_value_parts.join(",");//join parts
		description_value=description_value.mid(1);//remove '"'
		description_value.chop(2);//remove '">'
		info_format_line.description = description_value;

		if(type == "INFO")
		{
			foreach(const InfoFormatLinePtr info_line, file_info_style)
			{
				if(info_line->id == info_format_line.id)
				{
					Log::warn("Duplicate metadata information for field named '" + info_format_line.id + "'. Skipping metadata line " + QString::number(line_number) + ".");
					return nullptr;
				}
			}
		}
		else if(type == "FORMAT")
		{
			foreach(const InfoFormatLinePtr format_line, file_format_style)
			{
				if(format_line->id == info_format_line.id)
				{
					Log::warn("Duplicate metadata information for field named '" + info_format_line.id + "'. Skipping metadata line " + QString::number(line_number) + ".");
					return nullptr;
				}
			}
		}

		return std::make_shared<InfoFormatLine>(info_format_line);
	}
	FilterLine parseFilterLine(QByteArray& line, const int line_number)
	{
			//split at '=' to get id and description part
			QByteArrayList parts = line.mid(13, line.length()-15).split('=');
			if(parts.count()!=2) THROW(FileParseException, "Malformed FILTER line " + QString::number(line_number) + " : conains more/less than two parts: " + line);

			//remove 'Description' from first part
			QByteArrayList first_part = parts[0].split(',');
			if ( first_part.count()!=2 || first_part[1].trimmed()!="Description")
			{
				THROW(FileParseException, "Malformed FILTER line " + QString::number(line_number) + ": second field is not a description field " + line.trimmed());
			}

			FilterLine filter_line(line);
			filter_line.id = first_part[0];
			filter_line.description = QString(parts[1].mid(1)); //remove '\"'
			return filter_line;
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

