#include "BedpeFile.h"
#include "Helper.h"
#include "TSVFileStream.h"
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
			THROW(ArgumentException, "StructuralVariantType::UNKNOWN can only be used for the default constructer.");
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
	if (type_string == "UNKNOWN") THROW(ArgumentException, "StructuralVariantType::UNKNOWN can only be used for the default constructer.");
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
		return end2() - start1();
	}
	else if (t==StructuralVariantType::INS || t==StructuralVariantType::BND)
	{
		return -1;
	}

	THROW(ProgrammingException, "Unhandled variant type (int): " + BedpeFile::typeToString(t));
}

BedpeFile::BedpeFile()
{
}

void BedpeFile::load(const QString& file_name)
{
	clear();

	TSVFileStream file(file_name);

	//comments
	comments_ = file.comments();

	//header (first 6 fields are fixed)
	const int fixed_cols = 6;
	for(int i=fixed_cols; i<file.header().count(); ++i)
	{
		annotation_headers_ << file.header()[i];
	}
	int i_type = annotationIndexByName("TYPE");

	//fields
	while(!file.atEnd())
	{
		QByteArrayList fields = file.readLine();

		if(fields.isEmpty()) continue;

		//error when less than 6 fields
		if (fields.count()<fixed_cols)
		{
			THROW(FileParseException, "BEDPE file line with less than six fields found: '" + fields.join("\t") + "'");
		}

		//Add line
		lines_.append(BedpeLine(fields[0], parsePosIn(fields[1]), parsePosIn(fields[2]), fields[3], parsePosIn(fields[4]), parsePosIn(fields[5]), stringToType(fields[fixed_cols + i_type]), fields.mid(fixed_cols)));
	}
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

	THROW(ProgrammingException, "Unknown structural variant type '" + typeToString(type) + "'!");
}

QList< QMap<QByteArray,QByteArray> > BedpeFile::getInfos(QByteArray name)
{
	if(!name.contains("=")) name.append("=");

	QList< QMap<QByteArray,QByteArray> > result;
	foreach(QByteArray comment,comments_)
	{
		comment.replace("##","");
		if(!comment.startsWith(name)) continue;
		comment.replace(name,"");
		result.append(parseInfoField(comment));
	}
	return result;
}

QMap <QByteArray,QByteArray> BedpeFile::annotationDescriptionByID(const QByteArray &name)
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
	for(const auto& comment : comments_)
	{
		file->write(comment + "\n");
	}
	file->write("#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + annotation_headers_.join("\t") + "\n");
	for(const BedpeLine& line : lines_)
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
	for(auto comment : comments_) //TODO: return type for germline samples and add test > AXEL
	{
		if(comment.contains("fileformat=BEDPE_TUMOR_NORMAL_PAIR")) return BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL;
		if(comment.contains("fileformat=BEDPE_TUMOR_ONLY")) return BedpeFileFormat::BEDPE_SOMATIC_TUMOR_ONLY;
	}

	return BedpeFileFormat::BEDPE_UNKNOWN;
}
