#include "BedpeFile.h"
#include "Helper.h"
#include <QSharedPointer>

QString StructuralVariantTypeToString(StructuralVariantType type)
{
	switch (type)
	{
		case StructuralVariantType::DEL:
			return "DEL";
		case StructuralVariantType::DUP:
			return "DUP";
		case StructuralVariantType::INS:
			return "INS";
		case StructuralVariantType::INV:
			return "INV";
		case StructuralVariantType::BND:
			return "BND";
		case StructuralVariantType::UNKNOWN:
			THROW(ArgumentException, "StructuralVariantType::UNKNOWN can only be used for the default constructor.");
		default:
			THROW(NotImplementedException, "Invalid StructuralVariantType!");
	}
	return "";
}

QString StructuralVariantTypeToFullString(StructuralVariantType type)
{
	switch (type)
	{
		case StructuralVariantType::DEL:
			return "Deletion";
		case StructuralVariantType::DUP:
			return "Duplication";
		case StructuralVariantType::INS:
			return "Insertion";
		case StructuralVariantType::INV:
			return "Inversion";
		case StructuralVariantType::BND:
			return "Translocation";
		case StructuralVariantType::UNKNOWN:
			THROW(ArgumentException, "StructuralVariantType::UNKNOWN can only be used for the default constructor.");
		default:
			THROW(NotImplementedException, "Invalid StructuralVariantType!");
	}
	return "";
}

StructuralVariantType StructuralVariantTypeFromString(QString type_string)
{
	if (type_string == "DEL") return StructuralVariantType::DEL;
	if (type_string == "DUP") return StructuralVariantType::DUP;
	if (type_string == "INS") return StructuralVariantType::INS;
	if (type_string == "INV") return StructuralVariantType::INV;
	if (type_string == "BND") return StructuralVariantType::BND;
	if (type_string == "UNKNOWN") THROW(ArgumentException, "StructuralVariantType::UNKNOWN can only be used for the default constructor.");
	THROW(ArgumentException, "No matching StructuralVariantType found for '" + type_string + "'!");
	return StructuralVariantType::UNKNOWN;
}

BedpeLine::BedpeLine()
	: chr1_(".")
	, start1_(-1)
	, end1_(-1)
	, chr2_(".")
	, start2_(-1)
	, end2_(-1)
	, type_(StructuralVariantType::UNKNOWN)
	, annotations_()
{
}

BedpeLine::BedpeLine(const Chromosome& chr1, int start1, int end1, const Chromosome& chr2, int start2, int end2, StructuralVariantType type, const QList<QByteArray>& annotations)
	: chr1_(chr1)
	, start1_(start1)
	, end1_(end1)
	, chr2_(chr2)
	, start2_(start2)
	, end2_(end2)
	, type_(type)
	, annotations_(annotations)
{
}

QByteArray BedpeLine::toTsv() const
{
	QByteArrayList tmp_out;

	tmp_out << chr1_.str() << posToString(start1_) << posToString(end1_) << chr2_.str() << posToString(start2_) << posToString(end2_);
	foreach(const QByteArray& anno, annotations_)
	{
		tmp_out << anno;
	}

	return tmp_out.join("\t");
}

bool BedpeLine::intersectsWith(const BedFile& regions, bool imprecise_breakpoints) const
{
	StructuralVariantType t = type();
	if (t==StructuralVariantType::DEL || t==StructuralVariantType::DUP || t==StructuralVariantType::INV)
	{
		return regions.overlapsWith(chr1(), start1(), end2());
	}
	else if (t==StructuralVariantType::INS || t==StructuralVariantType::BND)
	{
		if (imprecise_breakpoints) return regions.overlapsWith(chr1(), start1(), end1()) || regions.overlapsWith(chr2(), start2(), end2());
		return regions.overlapsWith(chr1(), start1(), start1()) || regions.overlapsWith(chr2(), start2(), start2());
	}

	THROW(ProgrammingException, "Unhandled variant type (int): " + BedpeFile::typeToString(t));
}

QString BedpeLine::position1() const
{
	return chr1().str() + ":" + QString::number(start1()) + "-" + QString::number(end1());
}

QString BedpeLine::position2() const
{
	return chr2().str() + ":" + QString::number(start2()) + "-" + QString::number(end2());
}

QString BedpeLine::positionRange() const
{
	StructuralVariantType t = type();
	if (t==StructuralVariantType::DEL || t==StructuralVariantType::DUP || t==StructuralVariantType::INV)
	{
		return chr1().str() + ":" + QString::number(start1()) + "-" + QString::number(end2());
	}
	else if (t==StructuralVariantType::INS || t==StructuralVariantType::BND)
	{
		return position1();
	}

	THROW(ProgrammingException, "Unhandled variant type (int): " + BedpeFile::typeToString(t));
}

int BedpeLine::size() const
{
	StructuralVariantType t = type();
	if (t==StructuralVariantType::DEL || t==StructuralVariantType::DUP || t==StructuralVariantType::INV)
	{
		return end2() - start1() +1;
	}
	else if (t==StructuralVariantType::INS || t==StructuralVariantType::BND)
	{
		return -1;
	}

	THROW(ProgrammingException, "Unhandled variant type (int): " + BedpeFile::typeToString(t));
}

BedFile BedpeLine::affectedRegion(bool plus_one) const
{
	BedFile sv_region;
	int offset = plus_one ? 1 : 0;

	// determine region based on SV type
	switch (type())
	{
		case StructuralVariantType::INV:
		case StructuralVariantType::DEL:
		case StructuralVariantType::DUP:
			// whole area
			sv_region.append(BedLine(chr1(), start1() + offset, end2() + offset));
			break;

		case StructuralVariantType::BND:
			// consider pos 1 and pos 2 seperately
			sv_region.append(BedLine(chr1(), start1() + offset, end1() + offset));
			sv_region.append(BedLine(chr2(), start2() + offset, end2() + offset));
			break;

		case StructuralVariantType::INS:
			// compute CI of insertion
			sv_region.append(BedLine(chr1(), std::min(start1(), start2()) + offset, std::max(end1(), end2()) + offset)); //workaround: CI of insertions is sometimes not defined correctly
			break;

		default:
			THROW(ProgrammingException, "Unhandled variant type (int): " + BedpeFile::typeToString(type()));
			break;
	}
	return sv_region;
}

QString BedpeLine::toString(bool add_type) const
{
	BedFile region = affectedRegion(false);
	if (add_type)
	{
		if(type() == StructuralVariantType::BND)
		{
			return "BND from " + region[0].toString(true) + " to " +region[1].toString(true);
		}
		else
		{
			return BedpeFile::typeToString(type()) + " at " + region[0].toString(true);
		}
	}
	else
	{
		if(type() == StructuralVariantType::BND)
		{
			return region[0].toString(true) + " <> " +region[1].toString(true);
		}
		else
		{
			return region[0].toString(true);
		}
	}
}

QByteArray BedpeLine::genotype(const QList<QByteArray>& annotation_headers, bool error_if_not_found, int sample_idx) const
{
	//determine format column
	int format_idx = annotation_headers.indexOf("FORMAT");
	if (format_idx == -1)
	{
		if (error_if_not_found) THROW(ArgumentException, "Column \"FORMAT\" not found in annotation header!");
		return "";
	}

	//check sample column exists
	if (sample_idx < 0) THROW(ArgumentException, "Sample index " + QByteArray::number(sample_idx) + " must not be less than 0!");
	sample_idx = format_idx + 1 + sample_idx;
	if (sample_idx >= annotations_.count()) THROW(ArgumentException, "Sample index " + QByteArray::number(sample_idx) + " points to a annotation column that does not exist!");

	//get keys and values
	QByteArrayList keys = annotations_[format_idx].split(':');
	QByteArrayList values = annotations_[sample_idx].split(':');
	if (keys.size() != values.size())
	{
		THROW(ArgumentException, "Format and value column differ in length!");
	}

	//get value for the given key
	int field_idx = keys.indexOf("GT");
	if (field_idx==-1)
	{
		if (error_if_not_found) THROW(ArgumentException, "Key \"GT\" was not found in FORMAT column!");
		return "";
	}

	return values[field_idx];
}

void BedpeLine::setGenotype(const QList<QByteArray>& annotation_headers, QByteArray value, int sample_idx)
{
	//determine format column
	int format_idx = annotation_headers.indexOf("FORMAT");
	if (format_idx == -1)
	{
		THROW(ArgumentException, "Column \"FORMAT\" not found in annotation header!");
	}

	//check sample column exists
	if (sample_idx < 0) THROW(ArgumentException, "Sample index " + QByteArray::number(sample_idx) + " must not be less than 0!");
	sample_idx = format_idx + 1 + sample_idx;
	if (sample_idx >= annotations_.count()) THROW(ArgumentException, "Sample index " + QByteArray::number(sample_idx) + " points to a annotation column that does not exist!");

	//get keys and values
	QByteArrayList keys = annotations_[format_idx].split(':');
	QByteArrayList values = annotations_[sample_idx].split(':');
	if (keys.size() != values.size())
	{
		THROW(ArgumentException, "Format and value column differ in length!");
	}

	//get value for the given key
	int field_idx = keys.indexOf("GT");
	if (field_idx==-1)
	{
		THROW(ArgumentException, "Key \"GT\" was not found in FORMAT column!");
	}

	values[field_idx] = value;
	annotations_[sample_idx] = values.join(':');
}

QByteArray BedpeLine::genotypeHumanReadable(const QList<QByteArray>& annotation_headers, bool error_if_not_found, int sample_idx) const
{
	QByteArray gt = genotype(annotation_headers, error_if_not_found, sample_idx);

	//normalize
	gt.replace("|", "/");

	//convert
	if (gt=="1/1") return "hom";
	else if (gt=="0/1") return "het";
	else if (gt=="1/0") return "het";
	else if (gt=="0/0") return "wt";
	else if (gt=="") return "n/a";
	else if (gt=="./.") return "n/a";
	else THROW(ArgumentException, "Unhandled SV genotype '" + gt + "'!");
}

QMap<QByteArray, QByteArray> BedpeLine::getSampleFormatData(int anno_idx_format, int anno_idx_sample)
{
	if (anno_idx_format>=annotations().count()) THROW(ArgumentException, "Format index out of bounds in BedpeLine::getSampleFormatData");
	if (anno_idx_sample>=annotations().count()) THROW(ArgumentException, "Sample index out of bounds in BedpeLine::getSampleFormatData");

	QByteArrayList desc = annotations()[anno_idx_format].split(':');
	QByteArrayList data = annotations()[anno_idx_sample].split(':');
	if (desc.count()!=data.count()) THROW(ArgumentException, "Lenth of format and sample array not matching in BedpeLine::getSampleFormatData");

	QMap<QByteArray, QByteArray> output;
	for (int i=0; i<desc.count(); ++i)
	{
		output.insert(desc[i], data[i]);
	}
	return output;
}

QByteArray BedpeLine::getSampleFormatData(int anno_idx_format, int anno_idx_sample, QByteArray key)
{
	if (anno_idx_format>=annotations().count()) THROW(ArgumentException, "Format index out of bounds in BedpeLine::getSampleFormatData");
	if (anno_idx_sample>=annotations().count()) THROW(ArgumentException, "Sample index out of bounds in BedpeLine::getSampleFormatData");

	QByteArrayList desc = annotations()[anno_idx_format].split(':');
	QByteArrayList data = annotations()[anno_idx_sample].split(':');
	if (desc.count()!=data.count()) THROW(ArgumentException, "Lenth of format and sample array not matching in BedpeLine::getSampleFormatData");

	int index = desc.indexOf(key);
	if (index==-1) THROW(ArgumentException, "Key '" + key + "' not found in BedpeLine::getSampleFormatData. Valid keys are: " + desc.join(", "));

	return data[index];
}

GeneSet BedpeLine::genes(const QList<QByteArray>& annotation_headers, bool error_on_mismatch) const
{
	int gene_idx = annotation_headers.indexOf("GENES");
	if (gene_idx == -1)
	{
		if (error_on_mismatch) THROW(ArgumentException, "Column \"GENES\" not found in annotation header!");
		return GeneSet();
	}

	return GeneSet() << annotations_.at(gene_idx).split(',');
}

void BedpeLine::setGenes(const QList<QByteArray>& annotation_headers, const GeneSet& genes)
{
	int gene_idx = annotation_headers.indexOf("GENES");
	if (gene_idx == -1)
	{
		THROW(ArgumentException, "Column \"GENES\" not found in annotation header!");
	}

	annotations_[gene_idx] = genes.join(',');
}

BedpeFile::BedpeFile()
{
}

void BedpeFile::parseHeader(const TSVFileStream& file)
{
	//comments
	headers_ = file.comments();
	foreach(const QByteArray& comment, headers_)
	{
		if(comment.startsWith("##DESCRIPTION="))
		{
			QList<QByteArray> parts = comment.split('=');
			if(parts.count() < 3) continue;
			annotation_descriptions_.insert(parts[1], parts[2]);
		}
	}

	//header (first 6 fields are fixed)
	for(int i=6; i<file.header().count(); ++i)
	{
		annotation_headers_ << file.header()[i];
	}

	// parse sample info of multi sample BEDPE files
	sample_header_info_.clear();
	BedpeFileFormat file_format = format();
	if (file_format==BEDPE_GERMLINE_SINGLE || file_format==BEDPE_GERMLINE_MULTI || file_format==BEDPE_GERMLINE_TRIO)
	{
		parseSampleHeaderInfo();
	}
}

void BedpeFile::load(const QString& file_name)
{
	//clear
	clear();

	//header
	TSVFileStream file(file_name);
	parseHeader(file);

	//content
	int i_type = annotationIndexByName("TYPE");
	while(!file.atEnd())
	{
		QByteArrayList fields = file.readLine();

		if(fields.isEmpty()) continue;

		//error when less than 6 fields
		if (fields.count()<6)
		{
			THROW(FileParseException, "BEDPE file line with less than six fields found: '" + fields.join("\t") + "'");
		}

		//Add line
		lines_.append(BedpeLine(fields[0], parsePosIn(fields[1]), parsePosIn(fields[2]), fields[3], parsePosIn(fields[4]), parsePosIn(fields[5]), stringToType(fields[6 + i_type]), fields.mid(6)));
	}
}

void BedpeFile::loadHeaderOnly(const QString& file_name)
{
	//clear
	clear();

	//header
	TSVFileStream file(file_name);
	parseHeader(file);
}

bool BedpeFile::isValid() const
{
	try
	{
		format();
	}
	catch (Exception& e)
	{
		return false;
	}

	return true;
}

int BedpeFile::annotationIndexByName(const QByteArray& name, bool error_on_mismatch) const
{
	QList<int> matches;

	for(int i=0;i<annotation_headers_.count();++i)
	{
		if(name == annotation_headers_[i]) matches << i;
	}

	if(matches.count() > 1)
	{
		if(error_on_mismatch)
		{
			THROW(ArgumentException, "Found multiple column annotations for '" + name + "' in " + "BEDPE file!");
		}
		else
		{
			return -2;
		}
	}

	if(matches.count() == 0)
	{
		if(error_on_mismatch)
		{
			THROW(ArgumentException, "Found no column annotations for '" + name + "' in " + "BEDPE file!");
		}
		else
		{
			return -1;
		}
	}

	return matches.first();
}

QMap <QByteArray,QByteArray> BedpeFile::parseInfoField(QByteArray unparsed_fields)
{
	QMap<QByteArray,QByteArray> map = {};
	QStringList tmp_list = QString(unparsed_fields).replace(">","").replace("<","").replace("##","").simplified().split("\"",QString::SkipEmptyParts);
	bool in_quotes = unparsed_fields.at(0) == '\"';

	QStringList parts;

	//Separate Entries by ",", but conserve chars within quotes ""
	foreach(QString tmp,tmp_list)
	{
		if(in_quotes) //do not split by , if  within ""
		{
			parts.append(tmp);
		}
		else //split by , if outside ""
		{
			parts.append(tmp.split(',',QString::SkipEmptyParts));
		}
		in_quotes = !in_quotes;
	}

	//Stick together again, in case some entries were splitted at breakpoint between =-sign and quote-sign:   ="
	for(int i=0;i<parts.count();++i)
	{
		if(parts.at(i).endsWith("=") && i < parts.count()-1)
		{
			parts[i] = parts[i] + parts[i+1];
		}
	}

	//remove remaining entries that were already resticked to their key, recogniizable at their missing "="-sign
	for(int i=0;i<parts.count();++i)
	{
		if(!parts.at(i).contains('=')) parts.removeAt(i);
	}

	foreach(QString part,parts)
	{
		QStringList key_value = part.split("=");
		QByteArray key = key_value.at(0).simplified().toUtf8();
		QByteArray value = key_value.at(1).simplified().toUtf8();
		map.insert(key,value);
	}

	return map;
}

StructuralVariantType BedpeFile::stringToType(const QByteArray& str)
{
	if (str=="DEL") return StructuralVariantType::DEL;
	if (str=="DUP") return StructuralVariantType::DUP;
	if (str=="INS") return StructuralVariantType::INS;
	if (str=="INV") return StructuralVariantType::INV;
	if (str=="BND") return StructuralVariantType::BND;

	THROW(FileParseException, "Unsupported structural variant type '" + str + "'!");
}

QByteArray BedpeFile::typeToString(StructuralVariantType type)
{
	if (type==StructuralVariantType::DEL) return "DEL";
	if (type==StructuralVariantType::DUP) return "DUP";
	if (type==StructuralVariantType::INS) return "INS";
	if (type==StructuralVariantType::INV) return "INV";
	if (type==StructuralVariantType::BND) return "BND";

	THROW(ProgrammingException, "Unknown structural variant type '" + QString::number((int)type) + "'!");
}

QByteArray BedpeFile::typeToFullString(StructuralVariantType type)
{
	if (type==StructuralVariantType::DEL) return "Deletion";
	if (type==StructuralVariantType::DUP) return "Duplication";
	if (type==StructuralVariantType::INS) return "Insertion";
	if (type==StructuralVariantType::INV) return "Inversion";
	if (type==StructuralVariantType::BND) return "Breakpoint";

	THROW(ProgrammingException, "Unknown structural variant type '" + QString::number((int)type) + "'!");
}

QList< QMap<QByteArray,QByteArray> > BedpeFile::getInfos(QByteArray name)
{
	if(!name.contains("=")) name.append("=");

	QList< QMap<QByteArray,QByteArray> > result;
	foreach(QByteArray comment, headers_)
	{
		comment.replace("##","");
		if(!comment.startsWith(name)) continue;
		comment.replace(name,"");
		result.append(parseInfoField(comment));
	}
	return result;
}

QMap <QByteArray,QByteArray> BedpeFile::metaInfoDescriptionByID(const QByteArray &name)
{
	QList< QMap<QByteArray, QByteArray> > list = getInfos(name);
	QMap <QByteArray,QByteArray> out = {};

	for(int i=0;i<list.count();++i)
	{
		QByteArray id = list.at(i).value("ID");
		QByteArray desc = list.at(i).value("Description");

		if(id.isEmpty() || desc.isEmpty()) continue;
		out.insert(id,desc);
	}
	return out;
}

void BedpeFile::toTSV(QString file_name)
{
	QSharedPointer<QFile> file = Helper::openFileForWriting(file_name,false,false);
	foreach(const QByteArray& comment, headers_)
	{
		file->write(comment + "\n");
	}
	file->write("#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + annotation_headers_.join("\t") + "\n");
	foreach(const BedpeLine& line, lines_)
	{
		file->write(line.toTsv() +"\n");
	}
	file->close();
}

void BedpeFile::sort()
{
	std::sort(lines_.begin(),lines_.end());
}


BedpeFileFormat BedpeFile::format() const
{
	foreach(const QByteArray& comment, headers_)
	{
		if(comment.contains("fileformat=BEDPE_TUMOR_NORMAL_PAIR")) return BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL;
		if(comment.contains("fileformat=BEDPE_TUMOR_ONLY")) return BedpeFileFormat::BEDPE_SOMATIC_TUMOR_ONLY;
		if(comment.contains("fileformat=BEDPE_GERMLINE_MULTI")) return BedpeFileFormat::BEDPE_GERMLINE_MULTI;
        if(comment.contains("fileformat=BEDPE_GERMLINE_TRIO")) return BedpeFileFormat::BEDPE_GERMLINE_TRIO;
		if(comment.contains("fileformat=BEDPE_RNA")) return BedpeFileFormat::BEDPE_RNA;
		if(comment.contains("fileformat=BEDPE")) return BedpeFileFormat::BEDPE_GERMLINE_SINGLE;
	}
	THROW(FileParseException, "Could not determine format of BEDPE file.");
}

bool BedpeFile::isSomatic() const
{
	try
	{
		BedpeFileFormat f = format();
		if (f==BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL || f==BedpeFileFormat::BEDPE_SOMATIC_TUMOR_ONLY) return true;
	}
	catch(...) {} //Nothing to do here

	return false;
}

QByteArray BedpeFile::build()
{
	//parse header line, e.g. "##reference=file:///tmp/local_ngs_data/GRCh37.fa"
	foreach(const QByteArray& line, headers_)
	{
		if (line.startsWith("##reference="))
		{
			QByteArray genome = line.split('=').last();
			genome = genome.split('/').last();
			genome = genome.split('.').first();
			return genome;
		}
	}

	return "";
}


int BedpeFile::estimatedSvSize(int index) const
{
	const BedpeLine sv = lines_[index];
	// return -1 for translocation
	if (sv.type() == StructuralVariantType::BND) return -1;

	int i_info_a = annotationIndexByName("INFO_A");
	int sv_length = 0;
	foreach(const QByteArray& entry, lines_[index].annotations()[i_info_a].split(';'))
	{
		// report exact length
		if (entry.startsWith("SVLEN=")) return std::abs(Helper::toInt(entry.mid(6)));

		// estimate min length by adding the known bases on the left and right side
		if (entry.startsWith("LEFT_SVINSSEQ=")) sv_length += entry.size() - 14;
		if (entry.startsWith("RIGHT_SVINSSEQ=")) sv_length += entry.size() - 15;
	}

	return sv_length;
}

int BedpeFile::findMatch(const BedpeLine& sv, bool deep_ins_compare, bool error_on_mismatch, bool compare_ci) const
{
	int alt_a_idx = -1, info_a_idx = -1, pos_min_query = -1, pos_max_query = -1;
	QByteArray left_seq_query, right_seq_query;

	if(deep_ins_compare)
	{
		// get index columns
		alt_a_idx = annotationIndexByName("ALT_A");
		info_a_idx = annotationIndexByName("INFO_A");

		// precompute left-shift
		pos_min_query = std::min(std::min(sv.start1(), sv.end1()), std::min(sv.start2(), sv.end2()));
		pos_max_query = std::max(std::max(sv.start1(), sv.end1()), std::max(sv.start2(), sv.end2()));

		// extract partial sequences
		foreach (const QByteArray entry, sv.annotations()[info_a_idx].split(';'))
		{
			if(entry.trimmed().startsWith("LEFT_SVINSSEQ=")) left_seq_query = entry.trimmed();
			else if(entry.trimmed().startsWith("RIGHT_SVINSSEQ=")) right_seq_query = entry.trimmed();
		}
	}

	// search in all SVs
	for (int i = 0; i < count(); ++i)
	{
		if(lines_[i].type() != sv.type()) continue;
		if(lines_[i].chr1() != sv.chr1()) continue;
		if(lines_[i].chr2() != sv.chr2()) continue;

		if(sv.type() == StructuralVariantType::INS && deep_ins_compare)
		{
			//compare after left-shift
			int pos_min_ref = std::min(std::min(lines_[i].start1(), lines_[i].end1()), std::min(lines_[i].start2(), lines_[i].end2()));
			if(pos_min_query != pos_min_ref) continue;
			int pos_max_ref = std::max(std::max(lines_[i].start1(), lines_[i].end1()), std::max(lines_[i].start2(), lines_[i].end2()));
			if(pos_max_query != pos_max_ref) continue;

			//compare sequence
			if(lines_[i].annotations()[alt_a_idx] != sv.annotations()[alt_a_idx]) continue;


			// extract partial sequences
			QByteArray left_seq_ref, right_seq_ref;
			foreach (const QByteArray entry, lines_[i].annotations()[info_a_idx].split(';'))
			{
				if(entry.trimmed().startsWith("LEFT_SVINSSEQ=")) left_seq_ref = entry.trimmed();
				else if(entry.trimmed().startsWith("RIGHT_SVINSSEQ=")) right_seq_ref = entry.trimmed();
			}

			if(left_seq_query != left_seq_ref) continue;
			if(right_seq_query != right_seq_ref) continue;

			// match found:
			return i;
		}
		else
		{
			if (compare_ci)
			{
				// perform fuzzy matching (CI overlap)
				if (!BasicStatistics::rangeOverlaps(lines_[i].start1(), lines_[i].end1(), sv.start1(), sv.end1())) continue;
				if (!BasicStatistics::rangeOverlaps(lines_[i].start2(), lines_[i].end2(), sv.start2(), sv.end2())) continue;
				// fuzzy match found
				return i;
			}
			else
			{
				// search for exact match
				// compare SV positions
				if(lines_[i].start1() != sv.start1()) continue;
				if(lines_[i].end1() != sv.end1()) continue;
				if(lines_[i].start2() != sv.start2()) continue;
				if(lines_[i].end2() != sv.end2()) continue;

				// match found:
				return i;
			}

		}
	}

	// Throw error
	if(error_on_mismatch)
	{
		THROW(ArgumentException, "No match found in given SV in BedpeFile!");
	}

	return -1;
}

void BedpeFile::parseSampleHeaderInfo()
{
	//determine from SAMPLE comment lines
	foreach(QByteArray line, headers_)
    {
        line = line.trimmed();

        if (line.startsWith("##SAMPLE=<"))
        {
            //split into key=value pairs
			QByteArrayList parts = line.mid(10, line.length()-11).split(',');
            for (int i=1; i<parts.count(); ++i)
            {
                if (!parts[i].contains("="))
                {
                    parts[i-1] += "," + parts[i];
                    parts.removeAt(i);
                    --i;
                }
            }

			foreach(const QByteArray& part, parts)
            {
                int sep_idx = part.indexOf('=');
                QString key = part.left(sep_idx);
                QString value = part.mid(sep_idx+1);
                if (key=="ID")
                {
                    SampleInfo tmp;
					tmp.name = value;
					sample_header_info_ << tmp;
                }
                else
                {
                    sample_header_info_.last().properties[key] = value;
                }
            }
        }
	}

	//no SAMPLE line, i.e. single sample > fallback to column after FORMAT
	if (sample_header_info_.isEmpty())
	{
		int i_format = annotationIndexByName("FORMAT", false);
		if (i_format>=0 && i_format+1<annotationHeaders().count())
		{
			SampleInfo tmp;
			tmp.name = annotationHeaders()[i_format+1];
			sample_header_info_ << tmp;
		}
	}

	//determine column index
	for (int i=0; i<sample_header_info_.count(); ++i)
	{
		sample_header_info_[i].column_index = annotationIndexByName(sample_header_info_[i].name.toUtf8());
	}
}
