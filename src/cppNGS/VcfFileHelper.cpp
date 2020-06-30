#include "VcfFileHelper.h"

const QByteArrayList VCFHeader::InfoTypes = {"Integer", "Float", "Flag", "Character", "String"};
const QByteArrayList VCFHeader::FormatTypes =  {"Integer", "Float", "Character", "String"};

	const QByteArray& strToPointer(const QByteArray& str)
	{
		static QSet<QByteArray> uniq_8;

		auto it = uniq_8.find(str);
		if (it==uniq_8.cend())
		{
			it = uniq_8.insert(str);
		}
		//return str;
		return *it;
	}

	const QChar* strToPointer(const QString& str)
	{
		static QSet<QString> uniq_16;

		auto it = uniq_16.find(str);
		if (it==uniq_16.cend())
		{
			it = uniq_16.insert(str);
		}

		return it->constData();
	}

VcfHeaderLineBase::VcfHeaderLineBase(QByteArray line):
	line_key(line){}

VcfHeaderLine::VcfHeaderLine(QByteArray line, QByteArray name):
	VcfHeaderLineBase(line), name_value(name){}

void VCFHeader::storeHeaderInformation(QTextStream& stream) const
{
	foreach (VcfHeaderLineBasePtr header_line_ptr, header_line_order)
	{
		header_line_ptr->storeLine(stream);
	}
}

void VCFHeader::setFormat(QByteArray& line)
{
	file_format = std::make_shared<VcfHeaderLine>(VcfHeaderLine("fileformat", parseLineInformation(line, "fileformat")));
	header_line_order.push_back(file_format);
}
void VCFHeader::setDate(QByteArray& line)
{
	file_date = std::make_shared<VcfHeaderLine>(VcfHeaderLine("fileDate", parseLineInformation(line, "fileDate")));
	header_line_order.push_back(file_date);
}
void VCFHeader::setSource(QByteArray& line)
{
	file_source = std::make_shared<VcfHeaderLine>(VcfHeaderLine("source", parseLineInformation(line, "source")));
	header_line_order.push_back(file_source);
}
void VCFHeader::setReference(QByteArray& line)
{
	file_reference = std::make_shared<VcfHeaderLine>(VcfHeaderLine("reference", parseLineInformation(line, "reference")));
	header_line_order.push_back(file_reference);
}
void VCFHeader::setContig(QByteArray& line)
{
	file_contig.push_back(std::make_shared<VcfHeaderLine>(VcfHeaderLine("contig", parseLineInformationContig(line, "contig"))));
	header_line_order.push_back(file_contig.back());
}
void VCFHeader::setPhasing(QByteArray& line)
{
	file_phasing = std::make_shared<VcfHeaderLine>(VcfHeaderLine("phasing", parseLineInformation(line, "phasing")));
	header_line_order.push_back(file_phasing);
}
void VCFHeader::setInfoFormatLine(QByteArray& line, InfoFormatType type, const int line_number)
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
			if(QByteArray(format_line->id) == "GT" && file_format_style.size() > 1)
			{
				header_line_order.move(header_line_order.count()-1, header_line_order.count() - file_format_style.count());
			}
		}
	}
}
void VCFHeader::setFilterLine(QByteArray& line, const int line_number)
{
	file_filter_style.push_back(parseFilterLine(line, line_number));
	header_line_order.push_back(file_filter_style.back());
}
void VCFHeader::setUnspecificLine(QByteArray& line, const int line_number)
{
	line=line.mid(2);//remove "##"
	QByteArrayList splitted_line=line.split('=');
	if(splitted_line.count()<2)
	{
		THROW(FileParseException, "Malformed header line " + QString::number(line_number) + " is not a key=value pair: " + line.trimmed());
	}
	if(splitted_line.count() > 2)
	{
		for(int i = 2; i < splitted_line.count(); ++i)
		{
			splitted_line[1].append('=').append(splitted_line[i]);
		}
	}
	VcfHeaderLinePtr header_line_ptr = std::make_shared<VcfHeaderLine>(splitted_line[0], splitted_line[1]);
	unspecific_header_lines.push_back(header_line_ptr);
	header_line_order.push_back(unspecific_header_lines.back());
}

QByteArray VCFHeader::parseLineInformation(QByteArray line, const QByteArray& information)
{
	QList<QByteArray> splitted_line=line.split('=');
	if (splitted_line.count()<2)
	{
		THROW(FileParseException, "Malformed " + information + " line " + line.trimmed());
	}
	return splitted_line[1];
}
QByteArray VCFHeader::parseLineInformationContig(QByteArray line, const QByteArray& information)
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
InfoFormatLinePtr VCFHeader::parseInfoFormatLine(QByteArray& line, QByteArray type, const int line_number)
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
	info_format_line.id = strToPointer(ID_entry);
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
	info_format_line.type = strToPointer(s_type);
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
				Log::warn("Duplicate metadata information for field named '" + QByteArray(info_format_line.id) + "'. Skipping metadata line " + QString::number(line_number) + ".");
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
				Log::warn("Duplicate metadata information for field named '" + QByteArray(info_format_line.id) + "'. Skipping metadata line " + QString::number(line_number) + ".");
				return nullptr;
			}
		}
	}

	return std::make_shared<InfoFormatLine>(info_format_line);
}
FilterLinePtr VCFHeader::parseFilterLine(QByteArray& line, const int line_number)
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
		filter_line.id = strToPointer(first_part[0]);
		filter_line.description = QString(parts[1].mid(1)); //remove '\"'

		return std::make_shared<FilterLine>(filter_line);
}
