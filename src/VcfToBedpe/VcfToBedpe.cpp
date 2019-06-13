#include "VcfToBedpe.h"
#include "zlib.h"
#include "VariantList.h"
#include "Exceptions.h"
#include "Helper.h"

struct VcfToBedpe::vcf_line
{
	QByteArray chr;
	QByteArray pos;
	QByteArray id;
	QByteArray ref;
	QByteArray alt;
	QByteArray qual;
	QByteArray filter;
	QByteArray info;
	QByteArray format;
	QList<QByteArray> samples;

	vcf_line(const QByteArray& raw_line)
	{
		QList<QByteArray> parts = raw_line.split('\t');
		if(parts.count() < 9)
		{
			THROW(FileParseException,"Could not parse vcf line containing Structural variants!");
		}

		chr = parts[0];
		pos = parts[1];
		id = parts[2];
		ref = parts[3];
		alt = parts[4];
		qual = parts[5];
		filter = parts[6];
		info = parts[7];
		format = parts[8];
		for(int i=9;i<parts.count();++i)
		{
			samples << parts[i];
		}
	}

};

struct VcfToBedpe::bedpe_line
{
	QByteArray CHROM_A = ".";
	int START_A = std::numeric_limits<int>::quiet_NaN();
	int END_A = std::numeric_limits<int>::quiet_NaN();

	QByteArray CHROM_B = ".";
	int START_B = std::numeric_limits<int>::quiet_NaN();
	int END_B = std::numeric_limits<int>::quiet_NaN();

	QByteArray ID = ".";

	QByteArray QUAL = ".";
	QByteArray STRAND_A = ".";
	QByteArray STRAND_B = ".";

	QByteArray TYPE = ".";

	QByteArray FILTER = ".";

	QByteArray NAME_A = ".";
	QByteArray REF_A = ".";
	QByteArray ALT_A = ".";

	QByteArray NAME_B = ".";
	QByteArray REF_B = ".";
	QByteArray ALT_B = ".";

	QByteArray INFO_A = ".";
	QByteArray INFO_B = ".";

	QByteArray FORMAT_DESC = ".";

	QList<QByteArray> samples;

	QByteArray toText()
	{
		QByteArray out;

		out = CHROM_A + "\t" + QByteArray::number(START_A) + "\t" + QByteArray::number(END_A) + "\t" + CHROM_B + "\t" + QByteArray::number(START_B) + "\t" + QByteArray::number(END_B) + "\t" + ID +"\t" + QUAL + "\t" +
				STRAND_A + "\t" + STRAND_B + "\t" + TYPE + "\t" + FILTER + "\t" + NAME_A +"\t" + REF_A + "\t" + ALT_A + "\t" +
				NAME_B + "\t" + REF_B + "\t" + ALT_B +"\t" + INFO_A + "\t" + INFO_B + "\t" + FORMAT_DESC;
		for(const auto& sample : samples) out += "\t" + sample;

		return out;
	}
};

QByteArray VcfToBedpe::getLine()
{
	gzgets(file_,buffer_,buffer_size_);
	int i=0;
	while(i<buffer_size_-1 && buffer_[i]!='\n' && buffer_[i]!='\0' && buffer_[i]!='\n' && buffer_[i]!='\r')
	{
		++i;
	}
	return QByteArray(buffer_, i);
}

void VcfToBedpe::addHeaderInfoFieldAfter(const QByteArray& before, const QByteArray &key, const QByteArray &type, int number, const QByteArray &desc)
{
	for(int i=0;i<out_headers_.count();++i)
	{
		if(out_headers_.at(i).contains(before))
		{
			QByteArray text = "##INFO=<ID="+ key+",Number=" + QByteArray::number(number)+ ",Type=" + type + ",Description=\"" + desc + "\">";
			out_headers_.insert(i+1,text);
			break;
		}
	}
}

VcfToBedpe::VcfToBedpe(const QByteArray &in_file)
{
	//set buffer size for gz file line
	buffer_size_ = 4096;
	buffer_ = new char[buffer_size_];

	file_ = gzopen(in_file.data(),"rb");
	if (file_ == NULL)
	{
		THROW(FileAccessException, "Could not open file '" + in_file + "' for reading!");
	}


	while(!gzeof(file_))
	{
		QByteArray line = getLine();

		//Parse vcf header
		if(line.startsWith("#"))
		{
			if(line.startsWith("##"))
			{
				QByteArray bedpe_header_line = line;
				if(line.split('=')[0] == "##fileformat")
				{
					bedpe_header_line = "##fileformat=BEDPE";
				} else if(line.split('=')[0] == "##fileDate")
				{
					bedpe_header_line = "##fileDate=" + QDate::currentDate().toString("yyyyMMdd").toUtf8();
				}

				out_headers_ << bedpe_header_line;
				continue;
			}

			//Make list of all samples IDs contained in original vcf
			if(line.size() > 1 && line[1] != '#')
			{
				QList<QByteArray> parts = line.split('\t');
				if(parts.count() < 10)
				{
					THROW(FileParseException, "VCF with too few columns in header line");
				}

				for(int i=9;i<parts.count();++i)
				{
					samples_ << parts[i];
					continue;
				}

				break;//stop after header lines
			}
		}
	}

	addHeaderInfoFieldAfter("SVTYPE","POS","Integer",1,"Position of the variant described in the original VCF file.");
}


QMap<QByteArray,QByteArray> VcfToBedpe::parseInfoField(const QByteArray &field)
{
	QList<QByteArray> parts = field.split(';');

	QMap<QByteArray,QByteArray> out;

	for(const auto& part : parts)
	{
		QList<QByteArray> data = part.split('=');
		if(data.count() != 2) continue;
		out.insert(data[0],data[1]);
	}
	return out;
}

VcfToBedpe::bedpe_line VcfToBedpe::convertSingleLine(const VcfToBedpe::vcf_line &line_in)
{
	QMap<QByteArray,QByteArray> info = parseInfoField(line_in.info);
	//2nd breakpoint
	QByteArray bnd2 = info.value("END","DEFAULT");
	if(bnd2 == "DEFAULT")
	{
		THROW(FileParseException,"No entry \"END\" found in INFO field, but neccessary for simple breakpoints");
	}

	QChar orientation1 = '+';
	QChar orientation2 = '-';

	if(info.keys().contains("STRANDS") && info.value("STRANDS").count() == 2)
	{
		orientation1 = info.value("STRANDS").at(0);
		orientation2 = info.value("STRANDS").at(1);
	}

	bedpe_line res;
	res.CHROM_A = line_in.chr;



	res.START_A = line_in.pos.toInt();
	res.END_A = line_in.pos.toInt();
	//Add confidence intervals
	if(info.value("CIPOS","") != "")
	{
		QList<QByteArray> vals = info.value("CIPOS").split(',');

		bool ok_start = false, ok_end = false;

		int conf_start =vals[0].toInt(&ok_start);
		int conf_end = vals[1].toInt(&ok_end);

		if(ok_start && ok_end)
		{
			res.START_A = res.START_A +conf_start;
			res.END_A = res.END_A + conf_end;

			//add original position to info
		}
	}

	res.CHROM_B = info.value("CHR2");
	res.START_B = info.value("END").toInt();
	res.END_B = info.value("END").toInt();
	if(info.value("CIEND","") != "")
	{
		QList<QByteArray> vals = info.value("CIEND").split(',');
		bool ok_start = false, ok_end = false;

		int conf_start = vals[0].toInt(&ok_start);
		int conf_end = vals[1].toInt(&ok_end);
		if(conf_start && conf_end)
		{
			res.START_B = res.START_B + conf_start;
			res.END_B = res.END_B + conf_end;
		}
	}

	res.ID = line_in.id;

	res.FILTER = line_in.filter;
	res.TYPE = info.value("SVTYPE");
	res.QUAL = line_in.qual;

	res.STRAND_A = QString(orientation1).toUtf8();
	res.STRAND_B = QString(orientation2).toUtf8();

	res.NAME_A = line_in.id;
	res.REF_A = line_in.ref;
	res.ALT_A = line_in.alt;
	res.INFO_A = line_in.info;

	res.NAME_B = ".";
	res.REF_B = ".";
	res.ALT_B= ".";
	res.INFO_B = "MISSING";

	res.FORMAT_DESC = line_in.format;

	if(line_in.samples.count() != samples_.count())
	{
		THROW(FileParseException, "Number of sample annotation differs in line " + res.ID + " and bedpe header!");
	}

	for(const auto& sample : line_in.samples)
	{
		res.samples << sample;
	}


	return res;
}

VcfToBedpe::bedpe_line VcfToBedpe::parseLine(const QByteArray &line_in)
{
	vcf_line line(line_in);

	QMap<QByteArray,QByteArray> line_info = parseInfoField(line.info);
	if(!line_info.keys().contains("MATEID") || line_info.value("SVTYPE","") != "BND")
	{
		return convertSingleLine(line);
	}

	bedpe_line out;
	return out;
}

void VcfToBedpe::writeFile(const QString& out_file)
{
	QSharedPointer<QFile> out = Helper::openFileForWriting(out_file);

	for(const auto& header : out_headers_)
	{
		out->write(header + "\n");
	}

	QByteArray heading ="#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\tID\tQUAL\tSTRAND_A\tSTRAND_B\tTYPE\tFILTER\tNAME_A\tREF_A\tALT_A\tNAME_B\tREF_B\tALT_B\tINFO_A\tINFO_B\tFORMAT";
	for(const auto& sample : samples_) heading += "\t" + sample;
	out->write(heading + "\n");

	while(!gzeof(file_))
	{
		QByteArray converted_line = parseLine( getLine()).toText();
		out->write(converted_line + "\n");
	}
}
