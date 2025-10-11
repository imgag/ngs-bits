#include "VcfLine.h"
#include "Log.h"
#include "VariantList.h"

VcfLine::VcfLine()
	: chr_()
	, pos_(-1)
	, ref_()
	, alt_()
	, id_()
	, qual_(-1)
	, filters_()
	, info_()
	, sample_values_()
{
}

VcfLine::VcfLine(const Chromosome& chr, int pos, const Sequence& ref, const QList<Sequence>& alt, QByteArrayList format_ids, QByteArrayList sample_ids, QList<QByteArrayList> list_of_format_values)
	: chr_(chr)
	, pos_(pos)
	, ref_(ref)
	, alt_(alt)
	, id_()
	, qual_(-1)
	, filters_()
	, info_keys_()
	, info_()
	, sample_names_(sample_ids)
	, format_keys_(format_ids)
	, sample_values_(list_of_format_values)
{
	if(list_of_format_values.size() != sample_ids.size())
	{
		THROW(ArgumentException, "number of samples must equal the number of QByteArrayLists in list_of_format_values.")
	}
}

void VcfLine::setInfo(const QByteArrayList& info_keys, const QByteArrayList& info_values)
{
	info_keys_ = info_keys;
	info_ = info_values;

	if (info_.count()!=info_keys_.count()) THROW(ProgrammingException, "Info keys and values have differing counts: " + QString::number(info_keys_.count()) + " / " + QString::number(info_.count()));
}

void VcfLine::addFormatValues(const QByteArrayList& format_values)
{
	sample_values_.push_back(format_values);

	if (format_values.count()!=format_keys_.count()) THROW(ProgrammingException, "Format keys and values have differing counts: " + QString::number(format_keys_.count()) + " / " + QString::number(format_values.count()));
}

void VcfLine::setFormatValues(int sample_index, const QByteArrayList& format_values)
{
	if (sample_index>=sample_values_.count()) THROW(ProgrammingException, "Sample format index exceeds number of sample format entries: " + QString::number(sample_index) + " / " + QString::number(sample_values_.count()));

	sample_values_[sample_index] =format_values;

	if (format_values.count()!=format_keys_.count()) THROW(ProgrammingException, "Format keys and values have differing counts: " + QString::number(format_keys_.count()) + " / " + QString::number(format_values.count()));
}


const QByteArrayList VcfHeader::InfoTypes = {"Integer", "Float", "Flag", "Character", "String"};
const QByteArrayList VcfHeader::FormatTypes =  {"Integer", "Float", "Character", "String"};

void VcfHeader::clear()
{
	fileformat_.clear();
	file_comments_.clear();

	info_lines_.clear();
	filter_lines_.clear();
	format_lines_.clear();
}

const InfoFormatLine& VcfHeader::lineByID(const QByteArray& id, const QVector<InfoFormatLine>& lines, bool error_not_found) const
{
	static InfoFormatLine empty;
	bool found_multiple = false;

	int index = -1;
	for(int i=0; i<lines.count(); ++i)
	{
		if(lines.at(i).id==id)
		{
			if(index!=-1) found_multiple = true;
			index = i;
		}
	}

	if(error_not_found && index==-1) THROW(ProgrammingException, "Could not find column description '" + id + "'.");
	if(error_not_found && found_multiple) THROW(ProgrammingException, "Description for '" + id + "' occurs more than once.");

	if(!error_not_found && (found_multiple || index==-1))
	{
		return empty;
	}
	return lines.at(index);
}

const FilterLine& VcfHeader::filterLineByID(const QByteArray& id, bool error_not_found) const
{
	static FilterLine empty;
	bool found_multiple = false;

	int index = -1;
	for(int i=0; i<filterLines().count(); ++i)
	{
		if(filterLines().at(i).id==id)
		{
			if(index!=-1) found_multiple = true;
			index = i;
		}
	}

	if(error_not_found && index==-1) THROW(ProgrammingException, "Could not find column description '" + id + "'.");
	if(error_not_found && found_multiple) THROW(ProgrammingException, "Description for '" + id + "' occurs more than once.");

	if(!error_not_found && (found_multiple || index==-1))
	{
		return empty;
	}
	return filterLines().at(index);
}

int VcfHeader::vepIndexByName(const QString& name, bool error_if_not_found) const
{
	InfoFormatLine csq_info = infoLineByID("CSQ", error_if_not_found);
	if (csq_info.id.isEmpty())
	{
		if (error_if_not_found)
		{
			THROW(ArgumentException, "Info field 'CSQ' containing VEP annotation not found!");
		}
		else
		{
			return -1;
		}
	}

	QStringList parts = csq_info.description.trimmed().split("|");
	parts[0] = "Allele";
	int i_field = parts.indexOf(name);
	if (i_field==-1 && error_if_not_found)
	{
		THROW(ArgumentException, "Field '" + name + "' not found in VEP CSQ field!");
	}

	return i_field;
}

QByteArrayList VcfLine::vepAnnotations(int field_index) const
{
	QByteArrayList output;

	QByteArray csq = info("CSQ").trimmed();

    if (csq.size()>0)
	{
		QByteArrayList transcripts = csq.split(',');

		foreach(const QByteArray& transcript, transcripts)
		{
			QByteArrayList csq_fields = transcript.split('|');
			output << csq_fields[field_index];
		}
	}

	return output;
}

void VcfHeader::storeHeaderInformation(QTextStream& stream) const
{
	//first line should always be the fileformat
	stream << "##fileformat=" << (fileformat_.isEmpty() ? "VCFv4.2" : fileformat_) << "\n";
	//store all comment lines
	foreach(const VcfHeaderLine& comment, file_comments_)
	{
		comment.storeLine(stream);
	}
	//store info, filter, format
	foreach(const InfoFormatLine& info, info_lines_)
	{
		info.storeLine(stream, INFO_DESCRIPTION);
	}
	foreach(const FilterLine& filter, filter_lines_)
	{
		filter.storeLine(stream);
	}
	foreach(const InfoFormatLine& format, format_lines_)
	{
		format.storeLine(stream, FORMAT_DESCRIPTION);
	}
}

bool VcfHeader::infoIdDefined(const QByteArray& id) const
{
	foreach(const InfoFormatLine& line, info_lines_)
	{
		if (line.id==id) return true;
	}
	return false;
}

bool VcfHeader::formatIdDefined(const QByteArray& id) const
{
	foreach(const InfoFormatLine& line, format_lines_)
	{
		if (line.id==id) return true;
	}
	return false;
}

bool VcfHeader::filterIdDefined(const QByteArray& id) const
{
	foreach(const FilterLine& line, filter_lines_)
	{
		if (line.id==id) return true;
	}
	return false;
}

void VcfHeader::setFormat(const QByteArray& line)
{
	QList<QByteArray> splitted_line=line.split('=');
	if (splitted_line.count()<2)
	{
		THROW(FileParseException, "Malformed fileformat line " + line.trimmed());
	}
	fileformat_ =  splitted_line[1];
}

void VcfHeader::setInfoLine(const QByteArray& line, const int line_number)
{
	QByteArray tmp = line.mid(8);//remove "##INFO=<"
	InfoFormatLine info_line;
	if(parseInfoFormatLine(tmp, info_line, "INFO", line_number))
	{
		info_lines_.push_back(info_line);
	}
}

void VcfHeader::setFormatLine(const QByteArray& line, const int line_number)
{
	QByteArray tmp = line.mid(10);//remove "##FORMAT=<"
	InfoFormatLine format_line;
	if(parseInfoFormatLine(tmp, format_line, "FORMAT", line_number))
	{
		format_lines_.push_back(format_line);
		//make sure the "GT" format field is always the first format field
		if(format_line.id == "GT" && format_lines_.size() > 1)
		{
			format_lines_.move(format_lines_.count()-1, 0);
		}
	}
}

void VcfHeader::setFilterLine(const QByteArray& line, const int line_number)
{
	//split at '=' to get id and description part
	QByteArrayList parts = line.mid(13, line.length()-15).split('=');

	// ignore '=' in description part of filter header
	if(parts.at(0).endsWith("Description"))
	{
		QByteArrayList tmp;
		tmp << parts.at(0);
		parts.removeFirst();
		tmp << parts.join("=");
		parts = tmp;
	}

	if(parts.count()!=2) THROW(FileParseException, "Malformed FILTER line " + QString::number(line_number) + " : conains more/less than two parts: " + line);

	//remove 'Description' from first part
	QByteArrayList first_part = parts[0].split(',');
	if ( first_part.count()!=2 || first_part[1].trimmed()!="Description")
	{
		THROW(FileParseException, "Malformed FILTER line " + QString::number(line_number) + ": second field is not a description field " + line.trimmed());
	}

	FilterLine filter_line;
	filter_line.id = first_part[0];
	filter_line.description = QString(parts[1].mid(1)); //remove '\"'

	addFilterLine(filter_line);
}

void VcfHeader::setCommentLine(const QByteArray& line, const int line_number)
{
	QByteArrayList splitted_line = line.mid(2).split('=');
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

void VcfHeader::addFilter(const QByteArray& filter_id, const QString& description)
{
	FilterLine line;
	line.id = filter_id;
	line.description = description;

	filter_lines_.push_back(line);
}

bool VcfHeader::parseInfoFormatLine(const QByteArray& line,InfoFormatLine& info_format_line, QByteArray type, const int line_number)
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
		foreach(const InfoFormatLine& info_line, info_lines_)
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
		foreach(const InfoFormatLine& format_line, format_lines_)
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

bool VcfLine::isValid() const
{
	//check chr
	if (!chr_.isValid()) return false;

	//check pos
	if (pos_<0) return false;

	//check ref
	if (ref_.isEmpty()) return false;
	if (!ref_.onlyACGT()) return false;

	//check alt(s)
	if (alt_.isEmpty()) return false;
	foreach(const Sequence& alt, alt_)
	{
		if (!alt.onlyACGT() && alt!="<NON_REF>") return false;
	}

	return true;
}

bool VcfLine::isValid(const FastaFileIndex& reference) const
{
	if (!isValid()) return false;

	if (reference.seq(chr_, pos_, ref_.length(), true)!=ref_.toUpper()) return false;

	return true;
}

bool VcfLine::isInDel() const
{
	if(isMultiAllelic()) THROW(Exception, "Can not determine if multi-allelic variant is InDel.");

	return alt(0).length()>1 && ref().length()>1;
}

bool VcfLine::isIns() const
{
	if(isMultiAllelic()) THROW(Exception, "Cannot determine if multi-allelic variant is insertion.");

	return alt(0).length()>1 && ref().length()==1;
}

bool VcfLine::isDel() const
{
	if(isMultiAllelic()) THROW(Exception, "Cannot determine if multi-allelic variant is deletion.");

	return alt(0).length()==1 && ref().length()>1;
}

bool VcfLine::isMNP() const
{
	if(isMultiAllelic()) THROW(Exception, "Cannot determine if multi-allelic variant is MNP.");

	return alt(0).length()>1 && ref().length()>1 && alt(0).length()==ref().length();
}

bool VcfLine::operator<(const VcfLine& rhs) const
{
	if (chr_<rhs.chr_) return true; //compare chromosome
	else if (chr_>rhs.chr_) return false;
	else if (pos_<rhs.start()) return true; //compare start position
	else if (pos_>rhs.start()) return false;
	else if (ref_<rhs.ref()) return true; //compare ref sequence
	else if (ref_>rhs.ref()) return false;
	else if (altString()<rhs.altString()) return true; //compare obs sequence
	else if (altString()>rhs.altString()) return false;
	return false;
}

VcfLine::ShiftResult VcfLine::normalize(ShiftDirection shift_dir, const FastaFileIndex& reference, bool add_prefix_base_to_mnps)
{
	//skip variants with incorrect referance bases
	if (ref_ != reference.seq(chr_, pos_, ref_.length()))
	{
		return ShiftResult::SKIPPED;
	}

    //skip multi-allelic and empty variants
	if(isMultiAllelic() || alt().empty()) return ShiftResult::SKIPPED;

	//skip SNVs and MNPs
	Variant::normalize(pos_, ref_, alt_[0]);
	if (ref_.length()==1 && alt_[0].length()==1) return ShiftResult::SKIPPED;

	//skip complex indels (e.g. ACGT => CA)
	if (ref_.length()!=0 && alt(0).length()!=0)
	{
		//repend prefix base lost during normalization (if not MNP)
		if (ref_.length()!=alt(0).length() || add_prefix_base_to_mnps)
		{
			pos_ -= 1;
			ref_ = reference.seq(chr_, pos_, 1) + ref_;
			alt_[0] = reference.seq(chr_, pos_, 1) + alt_[0];
		}
		return ShiftResult::SKIPPED;
	}

    //skip all variants starting at first/ending at last base of chromosome
    if ((pos_ == 1 && shift_dir == ShiftDirection::LEFT) || (pos_ + ref_.length() - 1 == reference.lengthOf(chr_) && shift_dir == ShiftDirection::RIGHT))
    {
		return ShiftResult::SKIPPED;
    }

    if (shift_dir == ShiftDirection::LEFT)
    {
        //left-align INSERTION
        if (ref_.length()==0)
        {
            //shift block to the left
            Sequence block = Variant::minBlock(alt(0));
            pos_ -= block.length();
            while(pos_ > 0 && reference.seq(chr_, pos_, block.length())==block)
            {
                pos_ -= block.length();
            }
            pos_ += block.length();

            //prepend prefix base
            pos_ -= 1;
            ref_ = reference.seq(chr_, pos_, 1);
            setSingleAlt(ref_ + alt(0));

            //shift single-base to the left
            while(ref_[0]==alt(0)[alt(0).size()-1])
            {
                pos_ -= 1;
                ref_ = reference.seq(chr_, pos_, 1);
                setSingleAlt(ref_ + alt(0).left(alt(0).length()-1));
            }
        }

        //left-align DELETION
        else
        {
            //shift block to the left
            Sequence block = Variant::minBlock(ref_);
            while(pos_ >= 1 && reference.seq(chr_, pos_, block.length())==block)
            {
                pos_ -= block.length();
            }
            pos_ += block.length();
            //prepend prefix base
            pos_ -= 1;
            setSingleAlt(reference.seq(chr_, pos_, 1));
            ref_ = alt(0) + ref_;

            //shift single-base to the left
            while(ref_[ref_.size()-1]==alt(0)[0])
            {
                pos_ -= 1;
                setSingleAlt(reference.seq(chr_, pos_, 1));
                ref_ = alt(0) + ref_.left(ref_.length()-1);
            }
        }
    }
    else if (shift_dir == ShiftDirection::RIGHT)
    {
        //right-align INSERTION
        if (ref_.length()==0)
        {
            //shift block to the right
            Sequence block = Variant::minBlock(alt(0));
            while(pos_ < reference.lengthOf(chr_) - block.length() && reference.seq(chr_, pos_, block.length())==block)
            {
                pos_ += block.length();
            }

            //prepend prefix base
            pos_ -= 1;
            ref_ = reference.seq(chr_, pos_, 1);
            setSingleAlt(ref_ + alt(0));

            //shift single-base to the right
            while(reference.seq(chr_, pos_ + 1, 1)[0]==alt(0)[1])
            {
                pos_ += 1;
                ref_ = reference.seq(chr_, pos_, 1);
                setSingleAlt(ref_ + alt(0).right(alt(0).length()-2) + reference.seq(chr_, pos_, 1));
            }
        }

        //right-align DELETION
        else
        {
            //shift block to the right
            Sequence block = Variant::minBlock(ref_);
            while(pos_ < reference.lengthOf(chr_) - block.length() && reference.seq(chr_, pos_, block.length())==block)
            {
                pos_ += block.length();
            }
            pos_ -= ref_.length();

            //prepend prefix base
            pos_ -= 1;
            setSingleAlt(reference.seq(chr_, pos_, 1));
            ref_ = alt(0) + ref_;

            //shift single-base to the right
            while(ref_[1]==reference.seq(chr_, pos_ + ref_.length(), 1)[0])
            {
                pos_ += 1;
                setSingleAlt(reference.seq(chr_, pos_, 1));
                ref_ = reference.seq(chr_, pos_, ref_.length());
            }
        }
    }

	return ShiftResult::PROCESSED;
}
