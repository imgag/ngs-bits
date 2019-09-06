#include "BedpeFile.h"
#include "Helper.h"
#include "TSVFileStream.h"
#include <QSharedPointer>

BedpeLine::BedpeLine()
	: chr1_(".")
	, start1_(-1)
	, end1_(-1)
	, chr2_(".")
	, start2_(-1)
	, end2_(-1)
{
}

BedpeLine::BedpeLine(const QList<QByteArray> &input_fields)
{
	if(input_fields.count() < 6)
	{
		THROW(FileParseException,"Could not parse BedpeLine because it has less than 6 input fields.");
	}
	//First six input fields are fixed
	chr1_ = Chromosome(input_fields[0].trimmed());
	start1_ = parsePosIn(input_fields[1]);
	end1_ = parsePosIn(input_fields[2]);
	chr2_ = Chromosome(input_fields[3].trimmed());
	start2_ = parsePosIn(input_fields[4]);
	end2_ = parsePosIn(input_fields[5]);

	annotations_ = input_fields.mid(6);
}

BedpeLine::BedpeLine(const Chromosome& chr1, int start1, int end1, const Chromosome& chr2, int start2, int end2, const QList<QByteArray>& annotations)
	: chr1_(chr1)
	, start1_(start1)
	, end1_(end1)
	, chr2_(chr2)
	, start2_(start2)
	, end2_(end2)
	, annotations_(annotations)
{
}

QByteArray BedpeLine::toTsv() const
{
	QByteArrayList tmp_out;

	tmp_out << chr1_.str() << parsePosOut(start1_) << parsePosOut(end1_) << chr2_.str() << parsePosOut(start2_) << parsePosOut(end2_);
	foreach(QByteArray anno, annotations_) tmp_out << anno;

	return tmp_out.join("\t");
}

int BedpeLine::parsePosIn(QByteArray in) const
{
	bool ok = false;

	int out = in.trimmed().toInt(&ok);
	if(!ok) out = -1;

	return out;
}

QByteArray BedpeLine::parsePosOut(int in) const
{
	if(in == -1) return ".";
	else return QByteArray::number(in);
}


BedpeFile::SV_TYPE BedpeFile::analysisType()
{
	if(lines_.isEmpty()) return UNKNOWN;

	int i_ID = annotationIndexByName("ID",false);

	int i_info_a =annotationIndexByName("INFO_A",false);
	int i_info_b =annotationIndexByName("INFO_B",false);

	foreach(BedpeLine line, lines_)
	{
		//identify Manta from Read IDs
		if(i_ID > -1)
		{
			if(line.annotations().at(i_ID).toUpper().contains("MANTA")) return MANTA;
		}
		if(i_info_a > -1)
		{
			if(line.annotations().at(i_info_a).toUpper().contains("DELLY")) return DELLY;
		}
		if(i_info_b > -1)
		{
			if(line.annotations().at(i_info_b).toUpper().contains("DELLY")) return DELLY;
		}

	}

	return UNKNOWN;
}



BedpeFile::BedpeFile()
{
}

void BedpeFile::load(const QString& file_name)
{
	clear();

	TSVFileStream file(file_name);

	comments_ = file.comments();

	while(!file.atEnd())
	{
		QByteArrayList fields = file.readLine();

		if(fields.isEmpty()) continue;

		//error when less than 6 fields
		if (fields.count()<6)
		{
			THROW(FileParseException, "BEDPE file line with less than six fields found: '" + fields.join("\t") + "'");
		}

		//first 6 fields are fixed, remaining fields are optional/user-specific
		lines_.append(fields);
	}

	//Get headers for annotations
	for(int i=6;i<file.header().count();++i)
	{
		annotation_headers_ << file.header()[i];
	}
}

int BedpeFile::annotationIndexByName(const QByteArray& name, bool error_on_mismatch)
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
