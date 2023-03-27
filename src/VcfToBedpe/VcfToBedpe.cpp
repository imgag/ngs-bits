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
	int START_A = -1;
	int END_A = -1;

	QByteArray CHROM_B = ".";
	int START_B = -1;
	int END_B = -1;

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

	QByteArray posToString(int in)
	{
		return (in != -1 ? QByteArray::number(in) : "." );
	}

	QByteArray toText()
	{
		QByteArrayList out;

		out << CHROM_A << posToString(START_A) << posToString(END_A);
		out << CHROM_B << posToString(START_B) << posToString(END_B);
		out << ID << QUAL << STRAND_A << STRAND_B << TYPE  << FILTER  << NAME_A  << REF_A  << ALT_A;
		out << NAME_B << REF_B << ALT_B << INFO_A << INFO_B<< FORMAT_DESC;

		for(const auto& sample : samples) out << sample;

		return out.join('\t');
	}

	//Adds Chromosome, StartA, EndA including confidence interval (CIPOS) if available
	void addCoordinatesA(const VcfToBedpe::vcf_line& line_in)
	{
		CHROM_A = line_in.chr;
		QMap<QByteArray,QByteArray> info = VcfToBedpe::parseInfoField(line_in.info);
		START_A = line_in.pos.toInt();
		END_A = line_in.pos.toInt();
		//Add confidence intervals (CIPOS = Confidence Interval POS)
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
		else if(info.value("STDEV_POS", "") != "") //Sniffles
		{
			//caclulate CI from stddev
			if (info.value("SUPPORT", "") == "") THROW(FileAccessException, "INFO field 'SUPPORT' required to calculate the confidence interval!");
			int n = info.value("SUPPORT").toInt();
			double stdev = info.value("STDEV_POS").toDouble();
			int offset = std::ceil(1.96 * (stdev / std::sqrt(n)));
			START_A = START_A - offset;
			END_A = END_A + offset;
		}
	}

	void addCoordinatesB(const VcfToBedpe::vcf_line& line_in)
	{
		QMap<QByteArray,QByteArray> info = VcfToBedpe::parseInfoField(line_in.info);

		CHROM_B = info.value("CHR2"); //get chr from "CHR2" entry (delly files)
		if(info.value("END", ".") != ".")
		{
			START_B = info.value("END").toInt();
			END_B = info.value("END").toInt();
		}
		else
		{
			//for BNDs: determine position from ALT column (e.g. sniffles, cuteSV or dipdiff)
			if ((info.value("SVTYPE", "") == "BND") && (line_in.alt.startsWith("]") || line_in.alt.startsWith("[")|| line_in.alt.startsWith("N[") || line_in.alt.startsWith("N]")))
			{
				//parse 2nd breakpoint
				QByteArrayList pos_b;
				int str_length = line_in.alt.length() - 3;
				if (line_in.alt.startsWith("N"))
				{
					pos_b = line_in.alt.mid(2, str_length).split(':');
				}
				else
				{
					pos_b = line_in.alt.mid(1, str_length).split(':');
				}
				START_B = pos_b.at(1).toInt();
				END_B = START_B;
				// use chr in ALT entry if no 'CHR2' entry in INFO column was found
				if(CHROM_B.isEmpty()) CHROM_B = Chromosome(pos_b.at(0)).strNormalized(true);
			}
			else
			{
				THROW(FileParseException,"No entry \"END\" found in INFO field, but neccessary for simple breakpoints");
			}

		}

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
		else if(info.value("STDEV_LEN", "") != "") //Sniffles
		{
			//caclulate CI from stddev
			if (info.value("SUPPORT", "") == "") THROW(FileAccessException, "INFO field 'SUPPORT' required to calculate the confidence interval!");
			int n = info.value("SUPPORT").toInt();
			double stdev = info.value("STDEV_LEN").toDouble();
			int offset = std::ceil(1.96 * (stdev / std::sqrt(n)));
			START_B = START_B - offset;
			END_B = END_B + offset;
		}
	}
};

QByteArray VcfToBedpe::getLine()
{
	char* char_array = gzgets(file_,buffer_,buffer_size_);
	
	//handle errors like truncated GZ file
	if (char_array==nullptr)
	{
		int error_no = Z_OK;
		QByteArray error_message = gzerror(file_, &error_no);
		if (error_no!=Z_OK && error_no!=Z_STREAM_END)
		{
			THROW(FileParseException, "Error while reading file '" + filename_ + "': " + error_message);
		}
	}

	QByteArray line(char_array);
	while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

	return line;
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

VcfToBedpe::VcfToBedpe(const QByteArray& filename)
{
	//set buffer size for gz file line
	filename_ = filename;
	buffer_size_ = 1048576; //1MB buffer
	buffer_ = new char[buffer_size_];

	file_ = gzopen(filename.data(),"rb");
	if (file_ == NULL)
	{
		THROW(FileAccessException, "Could not open file '" + filename + "' for reading!");
	}
	
	//Parse headers
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
				}
				else if(line.split('=')[0] == "##fileDate")
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

	QChar orientation1 = '.';
	QChar orientation2 = '.';

	if(info.keys().contains("STRANDS") && info.value("STRANDS").count() == 2)
	{
		orientation1 = info.value("STRANDS").at(0);
		orientation2 = info.value("STRANDS").at(1);
	}

	bedpe_line res;
	res.addCoordinatesA(line_in);

	if(!single_manta_bnd) res.addCoordinatesB(line_in);

	//Set
	if(res.CHROM_B.isEmpty() && !single_manta_bnd)
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

	if(!single_manta_bnd)
	{
		res.NAME_B = ".";
		res.REF_B = ".";
		res.ALT_B = ".";
		res.INFO_B = ".";
	}
	else
	{
		res.NAME_B = "MISSING";
		res.REF_B = "MISSING";
		res.ALT_B = "MISSING";
		res.INFO_B = "MISSING";
	}


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

VcfToBedpe::bedpe_line VcfToBedpe::convertComplexLine(const VcfToBedpe::vcf_line &line_a, const VcfToBedpe::vcf_line &line_b, bool mate_missing)
{
	bedpe_line out;

	out.addCoordinatesA(line_a);


	out.ALT_A = line_a.alt;

	out.NAME_A = line_a.id;
	out.INFO_A = line_a.info;
	out.REF_A = line_a.ref;
	out.ID = line_a.id;



	if(line_b.pos != ".")
	{
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
	}

	if(!mate_missing)
	{
		out.CHROM_B = line_b.chr;
		out.NAME_B = line_b.id;
		out.REF_B = line_b.ref;
		out.ALT_B = line_b.alt;
		out.INFO_B = line_b.info;
	}
	else
	{
		out.NAME_B = "NOT_FOUND";
		out.REF_B = "NOT_FOUND";
		out.ALT_B = "NOT_FOUND";
		out.INFO_B = "NOT_FOUND";
	}


	if(line_a.qual == line_b.qual || mate_missing) out.QUAL = line_a.qual;
	else out.QUAL = "AMBIGUOUS";

	if(line_a.filter == line_b.filter || mate_missing) out.FILTER = line_a.filter;
	else out.FILTER = "AMBIGUOUS";

	if(line_a.format == line_b.format || mate_missing) out.FORMAT_DESC = line_a.format;
	else out.FORMAT_DESC = "AMBIGUOUS";

	if(line_a.samples == line_b.samples || mate_missing) out.samples = line_a.samples;
	else out.samples = {"AMBIGUOUS"};



	QChar orientation1 = '.';
	QChar orientation2 = '.';

	out.STRAND_A = QString(orientation1).toUtf8();
	out.STRAND_B = QString(orientation2).toUtf8();

	out.TYPE = "BND";

	return out;
}


void VcfToBedpe::convert(QString out_file)
{


	//Parse input/output lines
	QSharedPointer<QFile> out = Helper::openFileForWriting(out_file);
	for(const auto& header : out_headers_)
	{
		out->write(header + "\n");
	}

	QByteArray heading ="#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\tID\tQUAL\tSTRAND_A\tSTRAND_B\tTYPE\tFILTER\tNAME_A\tREF_A\tALT_A\tNAME_B\tREF_B\tALT_B\tINFO_A\tINFO_B\tFORMAT";
	for(const auto& sample : samples_) heading += "\t" + sample;
	out->write(heading + "\n");


	QMap<QByteArray,vcf_line> complex_lines; //complex lines: have two parts in original file, need to be treated different

	//Continues after header lines (were parsed in constructor already)
	while(!gzeof(file_))
	{
		QByteArray raw_line = getLine().trimmed();

		if(raw_line.isEmpty() || raw_line.startsWith("#")) continue;

		vcf_line line_in(raw_line);

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

	
	QList<QByteArray> parsed_ids; //lits of BND ids that have already been parsed, have to be skipped to avoid doubles in out file
	for(const auto& id : complex_lines.keys())
	{
		const vcf_line& line_a = complex_lines.value(id);

		QMap<QByteArray,QByteArray> info_a = parseInfoField(line_a.info);
		const QByteArray& mate_id = info_a.value("MATEID","");

		if(parsed_ids.contains(id) || parsed_ids.contains(mate_id)) continue;

		if(mate_id == "")
		{
			THROW(FileParseException,"Could not find mate ID in line info of breakpoint ID " + id);
		}

		QByteArray converted_line;
		vcf_line line_b;

		if(!complex_lines.keys().contains(mate_id)) //Parse Mates for which no MATE-ID is included in VCF file
		{
			bedpe_line temp =  convertComplexLine(line_a,line_b,true);
			converted_line = temp.toText();
		}
		else
		{
			line_b = complex_lines.value(mate_id);
			converted_line  = convertComplexLine(line_a,line_b,false).toText();
		}

		out->write(converted_line + "\n");

		parsed_ids.append(id);
		parsed_ids.append(mate_id);
	}
	out->close();
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
