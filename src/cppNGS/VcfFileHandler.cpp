#include "VcfFileHandler.h"

#include "Helper.h"
#include <zlib.h>

namespace VcfFormat
{

void VcfFileHandler::parseVcfHeader(const int line_number, QByteArray& line)
{
	if(line_number==1)
	{
		if(line.startsWith("##fileformat"))
		{
			VcfHeader_.setFormat(line);
		}
		else
		{
			THROW(FileParseException, "Malformed first line for the fileformat: " + line.trimmed());
		}
	}
	else if(line.startsWith("##INFO") || line.startsWith("##FORMAT"))
	{
		if (line.startsWith("##INFO"))
		{
			VcfHeader_.setInfoFormatLine(line, INFO, line_number);
		}
		else
		{
			VcfHeader_.setInfoFormatLine(line, FORMAT, line_number);
		}
	}
	else if(line.startsWith("##FILTER=<ID="))
	{
		VcfHeader_.setFilterLine(line, line_number);
	}
	else if(line.startsWith("##"))
	{
		VcfHeader_.setCommentLine(line, line_number);
	}
}
void VcfFileHandler::parseHeaderFields(QByteArray& line)
{
	//header line
	if (line.startsWith("#CHROM"))
	{
		QList<QByteArray> header_fields = line.mid(1).split('\t');

		if (header_fields.count()<VCFHeader::MIN_COLS)//8 are mandatory
		{
			THROW(FileParseException, "VCF file header line with less than 8 fields found: '" + line.trimmed() + "'");
		}
		if ((header_fields[0]!="CHROM")||(header_fields[1]!="POS")||(header_fields[2]!="ID")||(header_fields[3]!="REF")||(header_fields[4]!="ALT")||(header_fields[5]!="QUAL")||(header_fields[6]!="FILTER")||(header_fields[7]!="INFO"))
		{
			THROW(FileParseException, "VCF file header line with at least one illegal named mandatory column: '" + line.trimmed() + "'");
		}

		for(const QByteArray column_header : header_fields)
		{			
			column_headers_.push_back(column_header);
		}

		//if we have a FORMAT column with no sample
		if(header_fields.count()==9)
		{
			column_headers_.push_back("Sample");
		}
	}
}
void VcfFileHandler::parseVcfEntry(const int line_number, QByteArray& line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids)
{
	QList<QByteArray> line_parts = line.split('\t');
	if (line_parts.count()<VCFHeader::MIN_COLS)
	{
		THROW(FileParseException, "VCF data line needs at least 7 tab-separated columns! Found " + QString::number(line_parts.count()) + " column(s) in line number " + QString::number(line_number) + ": " + line);
	}
	VCFLine vcf_line;
	vcf_line.setChromosome(strToPointer(line_parts[0]));
	vcf_line.setPos(atoi(line_parts[1]));
	vcf_line.setRef(strToPointer(line_parts[3].toUpper()));

	vcf_line.setId(line_parts[2].split(';'));
	vcf_line.setAlt(line_parts[4].split(','));


	line_parts[5]=="." ? vcf_line.setQual(-1) : vcf_line.setQual(atoi(line_parts[5]));

	//FILTER
	vcf_line.setFilter(line_parts[6].split(';'));

	//INFO
	if(line_parts[7]!=".")
	{
		QByteArrayList info_list = line_parts[7].split(';');
		OrderedHash<QByteArray , QByteArray> info_entries;
		for(const QByteArray& info : info_list)
		{
			QByteArrayList key_value_pair = info.split('=');

			//check if the info is known in header
			if(!info_ids.contains(key_value_pair[0]))
			{
				InfoFormatLine new_info_line;
				new_info_line.id = info;
				new_info_line.number = "1";
				new_info_line.type = "String";
				new_info_line.description = "no description available";
				VcfHeader_.addInfoLine(new_info_line);
			}

			if(key_value_pair.size() == 1)
			{
				info_entries.push_back(strToPointer(key_value_pair[0]), strToPointer(QByteArray("TRUE")));
			}
			else
			{
				info_entries.push_back(strToPointer(key_value_pair[0]), strToPointer(key_value_pair[1]));
			}

		}
		vcf_line.setInfo(info_entries);

	}

	//FORMAT && SAMPLE
	if(line_parts.count() >= 10)
	{
		//FORMAT
		QByteArrayList format_list = line_parts[8].split(':');
		QByteArrayList format_entries;
		//check if the format is known in header
		for(const QByteArray& format : format_list)
		{
			//first entry must be GT if given
			if(format == "GT" && format_list[0]!="GT")
			{
				THROW(FileParseException, "First Format entry is not a genotype ('GT') for line " + QString::number(line_number) + ": " + line);
			}
			if(!format_ids.contains(format))
			{
				InfoFormatLine new_format_line;
				new_format_line.id = format;
				new_format_line.number = "1";
				new_format_line.type = "String";
				new_format_line.description = "no description available";
				VcfHeader_.addFormatLine(new_format_line);

				if(format == "GT")
				{
					VcfHeader_.moveFormatLine(VcfHeader_.formatLines().count()-1, 0);
				}
			}
			format_entries.push_back(strToPointer(format));
		}
		vcf_line.setFormat(format_entries);

		//SAMPLE
		OrderedHash<QByteArray, FormatIDToValueHash> sample_entries;
		QByteArrayList sample_names = sampleIDs();
		if(sample_names.count() != line_parts.count() - 9)
		{
			THROW(FileParseException, "Number of samples does not equal number of samples in header for line " + QString::number(line_number) + ": " + line);
		}
		for(int i = 9; i < line_parts.count(); ++i)
		{

			QByteArray sample_id = sample_names.at(i-9);
			QByteArrayList sample_id_list = line_parts[i].split(':');

			int format_entry_count = vcf_line.format().count();
			int sample_entry_count = sample_id_list.count();
			//SAMPLE columns can have missing trailing entries, but can not have more than specified in FORMAT
			if(sample_entry_count > format_entry_count)
			{
				THROW(FileParseException, "Sample column has more entries than defined in Format for line " + QString::number(line_number) + ": " + line);
			}

			FormatIDToValueHash sample;
			//parse all available entries
			for(int sample_id = 0; sample_id < sample_entry_count; ++sample_id)
			{
				QByteArray value = "";
				if(sample_id_list.at(sample_id) != ".") value = sample_id_list.at(sample_id);
				sample.push_back(vcf_line.format().at(sample_id), sample_id_list.at(sample_id));
			}
			//set missing trailing entries
			if(sample_entry_count < format_entry_count)
			{
				for(int trailing_sample_id = (sample_entry_count - format_entry_count); trailing_sample_id < format_entry_count; trailing_sample_id++)
				{
					sample.push_back(vcf_line.format().at(trailing_sample_id), "");
				}
			}

			sample_entries.push_back(strToPointer(sample_id), sample);
		}

		vcf_line.setSample(sample_entries);
	}

	VcfLines_.push_back(vcf_line);

}


void VcfFileHandler::processVcfLine(int& line_number, QByteArray line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids)
{

	while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);
	++line_number;

	//skip empty lines
	if(line.length()==0) return;

	//parse header
	if (line.startsWith("##"))
	{
		parseVcfHeader(line_number, line);
	}
	else if (line.startsWith("#CHROM"))
	{
		parseHeaderFields(line);
	}
	else
	{
		for(InfoFormatLine format : VcfHeader_.formatLines())
		{
			format_ids.insert(format.id);
		}
		for(InfoFormatLine info : VcfHeader_.infoLines())
		{
			info_ids.insert(info.id);
		}
		parseVcfEntry(line_number, line, info_ids, format_ids);
	}
}

void VcfFileHandler::loadFromVCF(const QString& filename)
{
	//parse from stream
	int line_number = 0;
	QSharedPointer<QFile> file = Helper::openFileForReading(filename, true);
	//Sets holding all INFO and FORMAT IDs defined in the header (might be extended if a vcf line contains new ones)
	QSet<QByteArray> info_ids_in_header;
	QSet<QByteArray> format_ids_in_header;
	while(!file->atEnd())
	{
		processVcfLine(line_number, file->readLine(), info_ids_in_header, format_ids_in_header);
	}
}

void VcfFileHandler::loadFromVCFGZ(const QString& filename)
{

	//parse from stream
	int line_number = 1;

	gzFile file = gzopen(filename.toLatin1().data(), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
	if (file==NULL)
	{
		THROW(FileAccessException, "Could not open file '" + filename + "' for reading!");
	}

	char* buffer = new char[1048576]; //1MB buffer
	while(!gzeof(file))
	{

		char* read_line = gzgets(file, buffer, 1048576);

		//handle errors like truncated GZ file
		if (read_line==nullptr)
		{
			int error_no = Z_OK;
			QByteArray error_message = gzerror(file, &error_no);
			if (error_no!=Z_OK && error_no!=Z_STREAM_END)
			{
				THROW(FileParseException, "Error while reading file '" + filename + "': " + error_message);
			}
		}
		//Sets holding all INFO and FORMAT IDs defined in the header (might be extended if a vcf line contains new ones)
		QSet<QByteArray> info_ids_in_header;
		QSet<QByteArray> format_ids_in_header;
		processVcfLine(line_number, QByteArray(read_line), info_ids_in_header, format_ids_in_header);
	}
	gzclose(file);
	delete[] buffer;
}

void VcfFileHandler::load(const QString& filename)
{
	QString fn_lower = filename.toLower();

	if (fn_lower.endsWith(".vcf"))
	{
		loadFromVCF(filename);
	}
	else if (fn_lower.endsWith(".vcf.gz"))
	{
		loadFromVCFGZ(filename);
	}
	else
	{
		THROW(ArgumentException, "Could not determine format of file '" + fn_lower + "' from file extension. Valid extensions are 'vcf' and 'vcf.gz'.");
	}
}

void VcfFileHandler::store(const QString& filename) const
{
	//open stream
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename);
	QTextStream stream(file.data());

	//write header information
	VcfHeader_.storeHeaderInformation(stream);

	//write header columns
	stream << "#" << column_headers_.at(0);
	for(int i = 1; i < column_headers_.count(); ++i)
	{
		stream << "\t" << column_headers_.at(i);
	}

	for(VCFLine vcf_line : VcfLines_)
	{
		stream << "\n" << vcf_line.chr().str()  << "\t" << vcf_line.pos();

		stream  << "\t"<< vcf_line.id().join(';');
		stream  << "\t"<< vcf_line.ref();

		stream << "\t" << vcf_line.alt().at(0);
		if(vcf_line.alt().count() > 1)
		{
			for(int i = 1; i < vcf_line.alt().size(); ++i)
			{
				stream  << "," <<  vcf_line.alt().at(i);
			}
		}
		QByteArray quality;
		if(vcf_line.qual() == -1)
		{
			quality = ".";
		}
		else
		{
			quality.setNum(vcf_line.qual());
		}
		stream  << "\t"<< quality;

		stream  << "\t"<< vcf_line.filter().join(':');

		stream  << "\t"<< vcf_line.infos().at(0).first << "=" << vcf_line.infos().at(0).second;
		if(vcf_line.infos().size() > 1)
		{
			for(int i = 1; i < vcf_line.infos().size(); ++i)
			{
				if(vcf_line.infos().at(i).second == "TRUE")
				{
					stream  << ";"<< vcf_line.infos().at(i).first;
				}
				else
				{
					stream  << ";"<< vcf_line.infos().at(i).first << "=" << vcf_line.infos().at(i).second;;
				}
			}
		}

		//if format exists there must also be at least one sample
		if(!vcf_line.format().empty())
		{
			stream  << "\t"<< vcf_line.format().at(0);
			for(int format_entry_id = 1; format_entry_id < vcf_line.format().count(); ++format_entry_id)
			{
				stream << ":" << vcf_line.format().at(format_entry_id);
			}

			//for every sample
			for(const QByteArray& name : sampleIDs())
			{
				FormatIDToValueHash sample = vcf_line.sample(name);
				stream << "\t" << sample.at(0).second;
				//for all entries in the sample (e.g. 'GT':'DP':...)
				for(int sample_entry_id = 1; sample_entry_id < sample.size(); ++sample_entry_id)
				{
					stream << ":" << sample.at(sample_entry_id).second;
				}
			}
		}
	}
}

void VcfFileHandler::checkValid() const
{
	foreach(VCFLine vcf_line, VcfLines_)
	{
		vcf_line.checkValid();

		if(vcf_line.samples().empty() && column_headers_.count() != VCFHeader::MIN_COLS)
		{
			THROW(ArgumentException, "Invalid variant annotation data: A VCF Line without samples should have " + QString::number(VCFHeader::MIN_COLS) + " columns.");
		}
		else if (vcf_line.samples().size() + 9 != column_headers_.count())
		{
			THROW(ArgumentException, "Invalid variant annotation data: Wrong number of columns.");
		}
	}
}

void VcfFileHandler::sort(bool use_quality)
{
	if (vcfLines().count()==0)
	{
		std::sort(VcfLines_.begin(), VcfLines_.end(), LessComparator(use_quality));
	}
}

void VcfFileHandler::removeDuplicates(bool sort_by_quality)
{
	sort(sort_by_quality);

	//remove duplicates (same chr, start, obs, ref) - avoid linear time remove() calls by copying the data to a new vector.
	QVector<VCFLine> output;
	output.reserve(vcfLines().count());
	for (int i=0; i<vcfLines().count()-1; ++i)
	{
		int j = i+1;
		if (VcfLines_.at(i).chr() != VcfLines_.at(j).chr() || VcfLines_.at(i).pos() != VcfLines_.at(j).pos() || VcfLines_.at(i).ref() !=VcfLines_.at(j).ref() || !qEqual(VcfLines_.at(i).alt().begin(),  VcfLines_.at(i).alt().end(), VcfLines_.at(j).alt().begin()))
		{
			output.append(VcfLines_.at(i));
		}
	}
	if (!VcfLines_.isEmpty())
	{
		output.append(VcfLines_.last());
	}

	//swap the old and new vector
	VcfLines_.swap(output);
}

QByteArrayList VcfFileHandler::sampleIDs() const
{
	QByteArrayList samples;
	//samples are all columns after the 10th
	if(column_headers_.count() >= 10)
	{
		for(int i = 9; i < column_headers_.count(); ++i)
		{
			samples.append(column_headers_.at(i));
		}
	}
	return samples;
}

QByteArrayList VcfFileHandler::informationIDs() const
{
	QByteArrayList informations;
	for(const InfoFormatLine& info : vcfHeader().infoLines())
	{
		informations.append(info.id);
	}
	return informations;
}
QByteArrayList VcfFileHandler::filterIDs() const
{
	QByteArrayList filters;
	for(const FilterLine& filter : vcfHeader().filterLines())
	{
		filters.append(filter.id);
	}
	return filters;
}
QByteArrayList VcfFileHandler::formatIDs() const
{
	QByteArrayList formats;
	for(const InfoFormatLine& format : vcfHeader().formatLines())
	{
		formats.append(format.id);
	}
	return formats;
}

AnalysisType VcfFileHandler::type(bool allow_fallback_germline_single_sample) const
{
	return vcfHeader().type(allow_fallback_germline_single_sample);
}


} //end namespace VcfFormat
