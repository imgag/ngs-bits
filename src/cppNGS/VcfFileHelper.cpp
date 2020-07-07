#include "VcfFileHelper.h"

namespace VcfFormat
{

LessComparator::LessComparator(bool use_quality)
	: use_quality(use_quality)
{
}
bool LessComparator::operator()(const VCFLine& a, const VCFLine& b) const
{
	if (a.chr()<b.chr()) return true;//compare chromsomes
	else if (a.chr()>b.chr()) return false;
	else if (a.pos()<b.pos()) return true;//compare start positions
	else if (a.pos()>b.pos()) return false;
	else if (a.ref().length()<b.ref().length()) return true;//compare end positions by comparing length of ref
	else if (a.ref().length()>b.ref().length()) return false;
	else if (a.ref()<b.ref()) return true;//compare reference seqs
	else if (a.ref()>b.ref()) return false;
	else if (a.alt()<b.alt()) return true;//compare alternative seqs
	else if (a.alt()>b.alt()) return false;
	else if (use_quality)
	{
		int q_a=a.qual();
		int q_b=b.qual();
		if(q_a<q_b) return true;
	}
	return false;
}

const QByteArrayList VCFHeader::InfoTypes = {"Integer", "Float", "Flag", "Character", "String"};
const QByteArrayList VCFHeader::FormatTypes =  {"Integer", "Float", "Character", "String"};

//minimum 192MB so far (instead of 233MB)
	const QByteArray& strToPointer(const QByteArray& str)
	{
		static QSet<QByteArray> uniq_8;

		auto it = uniq_8.find(str);
		if (it==uniq_8.cend())
		{
			it = uniq_8.insert(str);
		}

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


void VCFHeader::storeHeaderInformation(QTextStream& stream) const
{
	//first line should always be the fileformat
	stream << "##fileformat=" << fileformat_ << "\n";
	//store all comment lines
	for(VcfHeaderLine comment : file_comments_)
	{
		comment.storeLine(stream);
	}
	//store info, filter, format
	for(InfoFormatLine info : info_lines_)
	{
		info.storeLine(stream, INFO);
	}
	for(FilterLine filter : filter_lines_)
	{
		filter.storeLine(stream);
	}
	for(InfoFormatLine format : format_lines_)
	{
		format.storeLine(stream, FORMAT);
	}
}

void VCFHeader::setFormat(QByteArray& line)
{
	QList<QByteArray> splitted_line=line.split('=');
	if (splitted_line.count()<2)
	{
		THROW(FileParseException, "Malformed fileformat line " + line.trimmed());
	}
	fileformat_ =  splitted_line[1];
}
void VCFHeader::setInfoFormatLine(QByteArray& line, InfoFormatType type, const int line_number)
{
	if(type == INFO)
	{
		line=line.mid(8);//remove "##INFO=<"
		InfoFormatLine info_line;
		if(parseInfoFormatLine(line, info_line, "INFO", line_number))
		{
			info_lines_.push_back(info_line);
		}
	}
	else
	{
		line=line.mid(10);//remove "##FORMAT=<"
		InfoFormatLine format_line;
		if(parseInfoFormatLine(line, format_line, "FORMAT", line_number))
		{
			format_lines_.push_back(format_line);
			//make sure the "GT" format field is always the first format field
			if(format_line.id == "GT" && format_lines_.size() > 1)
			{
				format_lines_.move(format_lines_.count()-1, 0);
			}
		}
	}
}
void VCFHeader::setFilterLine(QByteArray& line, const int line_number)
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

	FilterLine filter_line;
	filter_line.id = strToPointer(first_part[0]);
	filter_line.description = QString(parts[1].mid(1)); //remove '\"'

	filter_lines_.push_back(filter_line);
}
void VCFHeader::setCommentLine(QByteArray& line, const int line_number)
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
	VcfHeaderLine header_line;
	header_line.key = splitted_line[0];
	header_line.value = splitted_line[1];

	file_comments_.push_back(header_line);
}

bool VCFHeader::parseInfoFormatLine(QByteArray& line,InfoFormatLine& info_format_line, QByteArray type, const int line_number)
{
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
		foreach(const InfoFormatLine info_line, info_lines_)
		{
			if(info_line.id == info_format_line.id)
			{
				Log::warn("Duplicate metadata information for field named '" + QByteArray(info_format_line.id) + "'. Skipping metadata line " + QString::number(line_number) + ".");
				return false;
			}
		}
	}
	else if(type == "FORMAT")
	{
		foreach(const InfoFormatLine format_line, format_lines_)
		{
			if(format_line.id == info_format_line.id)
			{
				Log::warn("Duplicate metadata information for field named '" + QByteArray(info_format_line.id) + "'. Skipping metadata line " + QString::number(line_number) + ".");
				return false;
			}
		}
	}

	return true;
}

AnalysisType VCFHeader::type(bool allow_fallback_germline_single_sample) const
{
	foreach(const VcfHeaderLine& line, file_comments_)
	{
		if (line.key == ("##ANALYSISTYPE="))
		{
			QString type = line.value;
			if (type=="GERMLINE_SINGLESAMPLE") return GERMLINE_SINGLESAMPLE;
			else if (type=="GERMLINE_TRIO") return GERMLINE_TRIO;
			else if (type=="GERMLINE_MULTISAMPLE") return GERMLINE_MULTISAMPLE;
			else if (type=="SOMATIC_SINGLESAMPLE") return SOMATIC_SINGLESAMPLE;
			else if (type=="SOMATIC_PAIR") return SOMATIC_PAIR;
			else THROW(FileParseException, "Invalid analysis type '" + type + "' found in variant list!");
		}
	}

	if (allow_fallback_germline_single_sample)
	{
		return GERMLINE_SINGLESAMPLE; //fallback for old files without ANALYSISTYPE header
	}
	else
	{
		THROW(FileParseException, "No analysis type found in variant list!");
	}
}

//returns if the chromosome is valid
bool VCFLine::isValidGenomicPosition() const
{
	bool is_valid_ref_base = true;
	for(int i = 0; i < this->ref_.size(); ++i)
	{
		if(ref_.at(i) != 'A' && ref_.at(i) != 'C' && ref_.at(i) != 'G' && ref_.at(i) != 'T' && ref_.at(i) != 'N' &&
		   ref_.at(i) != 'a' && ref_.at(i) != 'c' && ref_.at(i) != 'g' && ref_.at(i) != 't' && ref_.at(i) != 'n')
		{
			is_valid_ref_base = false;
			break;
		}
	}
	return chr_.isValid() && is_valid_ref_base && pos_>=0 && ref_.size()>=0 && !ref_.isEmpty() && !alt_.isEmpty();
}

//returns all not passed filters
QByteArrayList VCFLine::failedFilters()
{
	QByteArrayList filters;
	foreach(QByteArray tag, filter_)
	{
		tag = tag.trimmed();

		if (tag!="" && tag!="." && tag.toUpper()!="PASS" && tag.toUpper()!="PASSED")
		{
			filters.append(tag);
		}
	}
	return filters;
}

void VCFLine::checkValid() const
{
	if (!chr_.isValid())
	{
		THROW(ArgumentException, "Invalid variant chromosome string in variant '" + chr_.str() + " " + QString::number(pos_));
	}

	if (pos_ < 1 || ref_.length() < 1)
	{
		THROW(ArgumentException, "Invalid variant position range in variant '" +  chr_.str() + " " + QString::number(pos_));
	}

	if (ref_!="-" && !QRegExp("[ACGTN]+").exactMatch(ref_))
	{
		THROW(ArgumentException, "Invalid variant reference sequence in variant '" +  chr_.str() + " " + QString::number(pos_));
	}
	for(Sequence alt_seq : alt_)
	{
		if (alt_seq!="-" && alt_seq!="." && !QRegExp("[ACGTN,]+").exactMatch(alt_seq))
		{
			THROW(ArgumentException, "Invalid variant alternative sequence in variant '" +  chr_.str() + " " + QString::number(pos_));
		}
	}
}

} //end namespace VcfFormat

