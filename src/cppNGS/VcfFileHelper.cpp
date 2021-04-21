#include "VcfFileHelper.h"
#include "Helper.h"

VcfLine::VcfLine()
	: chr_()
	, pos_(-1)
	, ref_()
	, alt_()
	, id_()
	, qual_(-1)
	, filter_()
	, infoIdxOf_()
	, info_()
	, sampleIdxOf_()
	, formatIdxOf_()
	, sample_values_()
{
}

VcfLine::VcfLine(const Chromosome& chr, int pos, const Sequence& ref, const QVector<Sequence>& alt, QByteArrayList format_ids, QByteArrayList sample_ids, QList<QByteArrayList> list_of_format_values)
	: chr_(chr)
	, pos_(pos)
	, ref_(ref)
	, alt_(alt)
	, id_()
	, qual_(-1)
	, filter_()
	, infoIdxOf_()
	, info_()
	, sampleIdxOf_()
	, formatIdxOf_()
	, sample_values_(list_of_format_values)
{
	if(list_of_format_values.size() != sample_ids.size())
	{
		THROW(ArgumentException, "number of samples must equal the number of QByteArrayLists in list_of_format_values.")
	}
	//generate Hash for Format entries
	FormatIDToIdxPtr format_id_to_idx_entry = FormatIDToIdxPtr(new OrderedHash<QByteArray, int>);
	for(int i=0; i < format_ids.size(); ++i)
	{
		if(list_of_format_values.at(i).size() != format_ids.size())
		{
			THROW(ArgumentException, "number of formats must equal the number of QByteArray elements in each list of list_of_format_values.")
		}
		format_id_to_idx_entry->push_back(format_ids.at(i), i);
	}
	formatIdxOf_ = format_id_to_idx_entry;

	//generate Hash for smaple entries
	SampleIDToIdxPtr sample_id_to_idx_entry = SampleIDToIdxPtr(new OrderedHash<QByteArray, int>);
	for(int i=0; i < sample_ids.size(); ++i)
	{
		sample_id_to_idx_entry->push_back(sample_ids.at(i), i);
	}
	sampleIdxOf_ = sample_id_to_idx_entry;
}

VcfFormat::LessComparator::LessComparator(bool use_quality)
	: use_quality(use_quality)
{
}
bool VcfFormat::LessComparator::operator()(const VcfLinePtr& a, const VcfLinePtr& b) const
{
	if (a->chr()<b->chr()) return true;//compare chromsomes
	else if (a->chr()>b->chr()) return false;
	else if (a->start()<b->start()) return true;//compare start positions
	else if (a->start()>b->start()) return false;
	else if (a->ref().length()<b->ref().length()) return true;//compare end positions by comparing length of ref
	else if (a->ref().length()>b->ref().length()) return false;
	else if (a->ref()<b->ref()) return true;//compare reference seqs
	else if (a->ref()>b->ref()) return false;
	else if (a->alt(0)<b->alt(0)) return true;//compare alternative seqs
	else if (a->alt(0)>b->alt(0)) return false;
	else if (use_quality)
	{
		double q_a=a->qual();
		double q_b=b->qual();
		if(q_a<q_b) return true;
	}
	return false;
}

VcfFormat::LessComparatorByFile::LessComparatorByFile(QString filename)
	: filename_(filename)
{
	//build chromosome (as QString) to rank (as int) dictionary from file (rank=position in file)
	QStringList lines=Helper::loadTextFile(filename_);
	int rank=0;
	foreach(const QString& line, lines)
	{
		rank++;
		Chromosome chr(line.split('\t')[0]);
		chrom_rank_[chr.num()]=rank;
	}
}

bool VcfFormat::LessComparatorByFile::operator()(const VcfLinePtr& a, const VcfLinePtr& b) const
{
	int a_chr_num = a->chr().num();
	int b_chr_num = b->chr().num();
	if (!chrom_rank_.contains(a_chr_num))
	{
		THROW(FileParseException, "Reference file for sorting does not contain chromosome '" + a->chr().str() + "'!");
	}
	if (!chrom_rank_.contains(b_chr_num))
	{
		THROW(FileParseException, "Reference file for sorting does not contain chromosome '" + b->chr().str() + "'!");
	}

	if (chrom_rank_[a_chr_num]<chrom_rank_[b_chr_num]) return true; //compare rank of chromosome
	else if (chrom_rank_[a_chr_num]>chrom_rank_[b_chr_num]) return false;
	else if (a->start()<b->start()) return true; //compare start position
	else if (a->start()>b->start()) return false;
	else if (a->ref().length()<b->ref().length()) return true; //compare end position
	else if (a->ref().length()>b->ref().length()) return false;
	else if (a->ref()<b->ref()) return true; //compare ref sequence
	else if (a->ref()>b->ref()) return false;
	else if (a->alt(0)<b->alt(0)) return true; //compare obs sequence
	else if (a->alt(0)>b->alt(0)) return false;
	return false;
}

const QByteArrayList VcfHeader::InfoTypes = {"Integer", "Float", "Flag", "Character", "String"};
const QByteArrayList VcfHeader::FormatTypes =  {"Integer", "Float", "Character", "String"};

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

void VcfHeader::clear()
{
	fileformat_.clear();
	file_comments_.clear();

	info_lines_.clear();
	filter_lines_.clear();
	format_lines_.clear();
}

InfoFormatLine VcfHeader::lineByID(const QByteArray& id, const QVector<InfoFormatLine>& lines, bool error_not_found) const
{
	bool found_multiple = false;

	int index = -1;
	for(int i=0; i<lines.count(); ++i)
	{
		if(lines.at(i).id==id)
		{
			if(index!=-1)	found_multiple = true;
			index = i;
		}
	}

	if(error_not_found && index==-1)	THROW(ProgrammingException, "Could not find column description '" + id + "'.");
	if(error_not_found && found_multiple)	THROW(ProgrammingException, "Description for '" + id + "' occurs more than once.");

	if(!error_not_found && (found_multiple || index==-1))
	{
		return InfoFormatLine();
	}
	return lines.at(index);
}

InfoFormatLine VcfHeader::infoLineByID(const QByteArray& id, bool error_not_found) const
{
	return lineByID(id, infoLines(), error_not_found);
}

InfoFormatLine VcfHeader::formatLineByID(const QByteArray& id, bool error_not_found) const
{
	return lineByID(id, formatLines(), error_not_found);
}

FilterLine VcfHeader::filterLineByID(const QByteArray& id, bool error_not_found) const
{
	bool found_multiple = false;

	int index = -1;
	for(int i=0; i<filterLines().count(); ++i)
	{
		if(filterLines().at(i).id==id)
		{
			if(index!=-1)	found_multiple = true;
			index = i;
		}
	}

	if(error_not_found && index==-1)	THROW(ProgrammingException, "Could not find column description '" + id + "'.");
	if(error_not_found && found_multiple)	THROW(ProgrammingException, "Description for '" + id + "' occurs more than once.");

	if(!error_not_found && (found_multiple || index==-1))
	{
		return FilterLine();
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

	if (csq.count()>0)
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
	stream << "##fileformat=" << fileformat_ << "\n";
	//store all comment lines
	for(VcfHeaderLine comment : file_comments_)
	{
		comment.storeLine(stream);
	}
	//store info, filter, format
	for(InfoFormatLine info : info_lines_)
	{
		info.storeLine(stream, INFO_DESCRIPTION);
	}
	for(FilterLine filter : filter_lines_)
	{
		filter.storeLine(stream);
	}
	for(InfoFormatLine format : format_lines_)
	{
		format.storeLine(stream, FORMAT_DESCRIPTION);
	}
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

//returns if the chromosome is valid
bool VcfLine::isValidGenomicPosition() const
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

bool VcfLine::isMultiAllelic() const
{
	if(alt().count() > 1)
	{
		return true;
	}
	return false;
}

bool VcfLine::isInDel() const
{
	if(alt().count() > 1)
	{
		THROW(NotImplementedException, "Can not determine if multi allelic variant is InDEl.")
	}

	if(alt(0).length() > 1 || ref().length() > 1)
	{
		return true;
	}
	return false;
}

//returns all not passed filters
QByteArrayList VcfLine::failedFilters() const
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

QString VcfLine::variantToString() const
{
	return chr_.str() + ":" + QString::number(start()) + "-" + QString::number(end()) + " " + ref_ + ">" + altString();
}

bool VcfLine::operator==(const VcfLine& rhs) const
{
	return pos_==rhs.start() && chr_==rhs.chr() && ref_==rhs.ref() && altString()==rhs.altString();
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

void VcfLine::normalize(const Sequence& empty_seq, bool to_gsvar_format)
{
	//skip multi-allelic and empty variants
	if(isMultiAllelic() || alt().empty())	return;

	Variant::normalize(pos_, ref_, alt_[0]);

	if (ref_.isEmpty())
	{
		ref_ = empty_seq;
	}
	if (alt(0).isEmpty())
	{
		alt_[0] = empty_seq;
	}

	if (to_gsvar_format && ref_==empty_seq)
	{
		pos_ -= 1;
	}
}

void VcfLine::leftNormalize(QString reference_genome)
{

	FastaFileIndex reference(reference_genome);

	//leave multi-allelic variants unchanged
	if (isMultiAllelic())
	{
		return;
	}

	//write out SNVs unchanged
	if (ref_.length()==1 && alt_[0].length()==1)
	{
		return;
	}

	//skip all variants starting at first base of chromosome
	if (pos_==1)
	{
		return;
	}

	//skip SNVs disguised as indels (e.g. ACGT => AXGT)
	Variant::normalize(pos_, ref_, alt_[0]);

	if (ref_.length()==1 && alt(0).length()==1)
	{
		return;
	}

	//skip complex indels (e.g. ACGT => CA)
	if (ref_.length()!=0 && alt(0).length()!=0)
	{
		return;
	}

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
		while(ref_[0]==alt(0)[alt(0).count()-1])
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
		while(ref_[ref_.count()-1]==alt(0)[0])
		{
			pos_ -= 1;
			setSingleAlt(reference.seq(chr_, pos_, 1));
			ref_ = alt(0) + ref_.left(ref_.length()-1);
		}
	}
}

