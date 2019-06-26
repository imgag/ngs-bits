#include "VcfToBedpe.h"
#include "zlib.h"
#include "VariantList.h"
#include "Exceptions.h"
#include "Helper.h"

struct VcfToBedpe::vcf_line
{
	QByteArray chr = ".";
	QByteArray pos = ".";
	QByteArray id = ".";
	QByteArray ref = ".";
	QByteArray alt = ".";
	QByteArray qual = ".";
	QByteArray filter = ".";
	QByteArray info = ".";
	QByteArray format = ".";
	QList<QByteArray> samples;

	vcf_line()
	{}

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

	//Adds Chromosome, StartA, EndA including confidence interval (CIPOS) if available
	void addCoordinatesA(const VcfToBedpe::vcf_line& line_in)
	{
		CHROM_A = line_in.chr;
		QMap<QByteArray,QByteArray> info = VcfToBedpe::parseInfoField(line_in.info);
		START_A = line_in.pos.toInt();
		END_A = line_in.pos.toInt();
		//Add confidence intervals
		if(info.value("CIPOS","") != "")
		{
			QList<QByteArray> vals = info.value("CIPOS").split(',');

			bool ok_start = false, ok_end = false;

			int conf_start = vals[0].toInt(&ok_start);
			int conf_end = vals[1].toInt(&ok_end);

			if(ok_start && ok_end)
			{
				START_A = START_A + conf_start;
				END_A = END_A + conf_end;


			}
		}
	}

	void addCoordinatesB(const VcfToBedpe::vcf_line& line_in)
	{
		QMap<QByteArray,QByteArray> info = VcfToBedpe::parseInfoField(line_in.info);

		CHROM_B = info.value("CHR2"); //get chr from "CHR2" entry (delly files)
		START_B = info.value("END").toInt();
		END_B = info.value("END").toInt();
		if(info.value("CIEND","") != "")
		{
			QList<QByteArray> vals = info.value("CIEND").split(',');
			bool ok_start = false, ok_end = false;

			int conf_start = vals[0].toInt(&ok_start);
			int conf_end = vals[1].toInt(&ok_end);
			if(conf_start && conf_end)
			{
				START_B = START_B + conf_start;
				END_B = END_B + conf_end;
			}
		}
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

VcfToBedpe::bedpe_line VcfToBedpe::convertSingleLine(const VcfToBedpe::vcf_line &line_in, bool single_manta_bnd)
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

	res.addCoordinatesA(line_in);

	res.addCoordinatesB(line_in);

	//Set
	if(res.CHROM_B.isEmpty())
	{
		res.CHROM_B = res.CHROM_A;
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
	res.INFO_A = VcfToBedpe::newInfoFieldAfterKey(line_in.info,"SVTYPE","POS",line_in.pos);

	res.NAME_B = ".";
	res.REF_B = ".";
	res.ALT_B= ".";
	res.INFO_B = ".";
	if(single_manta_bnd) res.INFO_B = "MISSING";

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

VcfToBedpe::bedpe_line VcfToBedpe::convertComplexLine(const VcfToBedpe::vcf_line &line_a, const VcfToBedpe::vcf_line &line_b)
{
	bedpe_line out;

	out.addCoordinatesA(line_a);


	out.ALT_A = line_a.alt;

	out.NAME_A = line_a.id;
	out.INFO_A = line_a.info;
	out.REF_A = line_a.ref;

	out.CHROM_B = line_b.chr;
	out.START_B = line_b.pos.toInt();
	out.END_B = line_b.pos.toInt();
	QMap<QByteArray,QByteArray> info_b = parseInfoField(line_b.info);
	if(info_b.keys().contains("CIPOS"))
	{
		QList<QByteArray> vals = info_b.value("CIPOS").split(',');

		bool ok_start = false, ok_end = false;

		int conf_start = vals[0].toInt(&ok_start);
		int conf_end = vals[1].toInt(&ok_end);

		if(ok_start && ok_end)
		{
			out.START_B = out.START_B + conf_start;
			out.END_B = out.END_B + conf_end;
		}
	}
	out.NAME_B = line_b.id;
	out.REF_B = line_b.ref;
	out.ALT_B = line_b.alt;
	out.INFO_B = line_b.info;



	out.ID = line_b.id;
	if(line_a.qual == line_b.qual)
	{
		out.QUAL = line_b.qual;
	}

	if(line_a.filter == line_b.filter) //set filter if equal
	{
		out.FILTER = line_a.filter;
	}
	else //if both BNDs have different filters, merge
	{
		QList<QByteArray> filters = line_a.filter.split(';');
		for(const auto& filter_b : line_b.filter.split(';'))
		{
			if(!filters.contains(filter_b)) filters << filter_b;
		}
		out.FILTER = filters.join(';');
	}

	if(line_a.format == line_b.format)
	{
		out.FORMAT_DESC = line_a.format;
	}
	else
	{
		out.FORMAT_DESC = "CONFLICTING";
	}

	if(line_a.samples == line_b.samples)
	{
		out.samples = line_a.samples;
	}
	else
	{
		out.samples = {"CONFLICTING"};
	}


	QChar orientation1 = '+';
	QChar orientation2 = '-';

	if(info_b.keys().contains("STRANDS") && info_b.value("STRANDS").count() == 2)
	{
		orientation1 = info_b.value("STRANDS").at(0);
		orientation2 = info_b.value("STRANDS").at(1);
	}

	out.STRAND_A = QString(orientation1).toUtf8();
	out.STRAND_B = QString(orientation2).toUtf8();

	out.TYPE = "BND";

	return out;
}


void VcfToBedpe::convert(const QString& out_file)
{
	QSharedPointer<QFile> out = Helper::openFileForWriting(out_file);

	for(const auto& header : out_headers_)
	{
		out->write(header + "\n");
	}

	QByteArray heading ="#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\tID\tQUAL\tSTRAND_A\tSTRAND_B\tTYPE\tFILTER\tNAME_A\tREF_A\tALT_A\tNAME_B\tREF_B\tALT_B\tINFO_A\tINFO_B\tFORMAT";
	for(const auto& sample : samples_) heading += "\t" + sample;
	out->write(heading + "\n");


	QMap<QByteArray,vcf_line> complex_lines; //mateid <-> vcf pairs of compley BNDs

	while(!gzeof(file_))
	{
		vcf_line line_in(getLine());

		QMap<QByteArray,QByteArray> line_info = parseInfoField(line_in.info);

		QByteArray converted_line = "";

		if(!line_info.keys().contains("MATEID") || line_info.value("SVTYPE","") != "BND")
		{
			if(line_info.value("SVTYPE").contains("MantaBND")) converted_line =  convertSingleLine(line_in,true).toText();
			else converted_line =  convertSingleLine(line_in).toText();
		}
		else if(line_info.keys().contains("MATEID")) //complex breakpoints that have a Mate
		{
			complex_lines.insert(line_in.id,line_in);
			continue;
		}
		out->write(converted_line + "\n");
	}

	//BND ids that have already been parsed, have to be skipped
	QList<QByteArray> parsed_ids;

	for(const auto& id : complex_lines.keys())
	{
		const vcf_line& line_a = complex_lines.value(id);

		QMap<QByteArray,QByteArray> info_a = parseInfoField(line_a.info);
		const QByteArray mate_id = info_a.value("MATEID","");

		if(parsed_ids.contains(id) || parsed_ids.contains(mate_id)) continue;

		if(mate_id == "")
		{
			THROW(FileParseException,"Could not find mate ID in line info of breakpoint ID " + id);
		}

		vcf_line line_b;
		if(!complex_lines.keys().contains(mate_id))
		{
			//TODO: Parse BNDs that have an entry for MATE but MATE is not included in SV file
			qDebug() << "NO MATE FOUND FOR id " + id + " and MATE ID " + mate_id << endl;
			line_b.samples = {"."};

			//THROW(FileParseException,"Could not find mate with ID " + mate_id);
		}
		else
		{
			line_b = complex_lines.value(mate_id); //mate
		}

		QByteArray converted_line = convertComplexLine(line_a,line_b).toText();

		out->write(converted_line + "\n");

		parsed_ids.append(id);
		parsed_ids.append(mate_id);
	}
}

QByteArray VcfToBedpe::newInfoFieldAfterKey(const QByteArray& info_old,const QByteArray& key_before, const QByteArray& key, const QByteArray& data)
{
	QList<QByteArray> parts = info_old.split(';');
	if(parts.count() <= 1) return info_old;

	int i_insert_after = -1;

	for(int i=0;i<parts.count();++i)
	{
		QList<QByteArray> tmp = parts[i].split('=');
		if(tmp.count() != 2) continue;

		if(tmp[0].contains(key_before))
		{
			i_insert_after = i;
			break;
		}
	}

	if(i_insert_after == -1) return info_old;

	parts.insert(i_insert_after+1,key + "=" + data);

	return parts.join(';');
}
