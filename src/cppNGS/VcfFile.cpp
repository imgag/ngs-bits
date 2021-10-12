#include "VcfFile.h"
#include "Helper.h"
#include <zlib.h>

VcfFile::VcfFile()
	: vcf_lines_()
	, vcf_header_()
	, column_headers_()
	, sample_id_to_idx_()
	, format_id_to_idx_list_()
	, info_id_to_idx_list_()
{
}

void VcfFile::clear()
{
	vcf_lines_.clear();
	column_headers_.clear();
	vcf_header_.clear();

	sample_id_to_idx_.clear();
	format_id_to_idx_list_.clear();
	info_id_to_idx_list_.clear();
}

void VcfFile::parseVcfHeader(int line_number, const QByteArray& line)
{
	if(line_number==1)
	{
		if(line.startsWith("##fileformat"))
		{
			vcf_header_.setFormat(line);
		}
		else
		{
			THROW(FileParseException, "Malformed first line for the fileformat: " + line.trimmed());
		}
	}
	else if (line.startsWith("##INFO"))
	{
		vcf_header_.setInfoLine(line, line_number);
	}
	else if(line.startsWith("##FORMAT"))
	{
		vcf_header_.setFormatLine(line, line_number);
	}
	else if(line.startsWith("##FILTER=<ID="))
	{
		vcf_header_.setFilterLine(line, line_number);
	}
	else if(line.startsWith("##"))
	{
		vcf_header_.setCommentLine(line, line_number);
	}
}

void VcfFile::parseHeaderFields(const QByteArray& line, bool allow_multi_sample)
{
	//header line
	if (line.startsWith("#CHROM"))
	{
		QList<QByteArray> header_fields = line.mid(1).split('\t');

		if (header_fields.count()< MIN_COLS)//8 are mandatory
		{
			THROW(FileParseException, "VCF file header line with less than 8 fields found: '" + line.trimmed() + "'");
		}
		if ((header_fields[0]!="CHROM")||(header_fields[1]!="POS")||(header_fields[2]!="ID")||(header_fields[3]!="REF")||(header_fields[4]!="ALT")||(header_fields[5]!="QUAL")||(header_fields[6]!="FILTER")||(header_fields[7]!="INFO"))
		{
			THROW(FileParseException, "VCF file header line with at least one inaccurately named mandatory column: '" + line.trimmed() + "'");
		}
		if(header_fields.count() >= 9 && header_fields[8] != "FORMAT")
		{
			THROW(FileParseException, "VCF file header line with an inaccurately named FORMAT column: '" + line.trimmed() + "'");
		}

		int header_count;
		allow_multi_sample ? header_count=header_fields.count() : header_count=std::min(10, header_fields.count());

		for(int i = 0; i < header_count; ++i)
		{
			column_headers_.push_back(header_fields.at(i));
		}

		//samples are all columns after the 10th
		sample_id_to_idx_ = SampleIDToIdxPtr(new OrderedHash<QByteArray, int>);
		if(column_headers_.count() >= 10)
		{
			for(int i = 9; i < column_headers_.count(); ++i)
			{
				sample_id_to_idx_->push_back(column_headers_.at(i), i-9);
			}
		}
		else if(header_fields.count()==9) //if we have a FORMAT column with no sample
		{
			column_headers_.push_back("Sample");
			sample_id_to_idx_->push_back("Sample", 0);
		}
		else if(header_fields.count()==8)
		{
			column_headers_.push_back("FORMAT");
			column_headers_.push_back("Sample");
			sample_id_to_idx_->push_back("Sample", 0);
		}
	}
}

void VcfFile::parseVcfEntry(int line_number, const QByteArray& line, QSet<QByteArray>& info_ids, QSet<QByteArray>& format_ids, QSet<QByteArray>& filter_ids, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert)
{
	QList<QByteArray> line_parts = line.split('\t');

	if (line_parts.count()< MIN_COLS)
	{
		THROW(FileParseException, "VCF data line needs at least 8 tab-separated columns! Found " + QString::number(line_parts.count()) + " column(s) in line number " + QString::number(line_number) + ": " + line);
	}

	VcfLinePtr vcf_line = VcfLinePtr(new VcfLine);

	vcf_line->setChromosome(strToPointer(line_parts[CHROM]));
	if(!vcf_line->chr().isValid())
	{
		THROW(ArgumentException, "Invalid variant chromosome string in line " + QString::number(line_number) + ": " + vcf_line->chr().str() + ".");
	}
	vcf_line->setPos(atoi(line_parts[POS]));
	if(vcf_line->start() < 1)
	{
		THROW(ArgumentException, "Invalid variant position range in line " + QString::number(line_number) + ": " + QString::number(vcf_line->start()) + ".");
	}
	vcf_line->setRef(strToPointer(line_parts[REF].toUpper()));

	//Skip variants that are not in the target region (if given)
	if (roi_idx!=nullptr)
	{
		int end =  vcf_line->start() +  vcf_line->ref().length() - 1;
		bool in_roi = roi_idx->matchingIndex(vcf_line->chr(), vcf_line->start(), end) != -1;
		if ((!in_roi && !invert) || (in_roi && invert))
		{
			return;
		}
	}
	QByteArrayList id_list = line_parts[ID].split(';');
	for(QByteArray& single_id : id_list)
	{
		single_id = strToPointer(single_id);
	}
	vcf_line->setId(id_list);
	foreach(const QByteArray& alt, line_parts[ALT].split(','))
	{
		vcf_line->addAlt(alt);
	}

	if(line_parts[QUAL]==".")
	{
		vcf_line->setQual(-1);
	}
	else
	{
		bool quality_ok;
		double qual = line_parts[QUAL].toDouble(&quality_ok);
		if (!quality_ok) THROW(ArgumentException, "Quality '" + line_parts[QUAL] + "' is no float - variant.");
		vcf_line->setQual(qual);
	}

	//FILTER
	vcf_line->setFilter(line_parts[FILTER].split(';'));
	for(const QByteArray& filter : vcf_line->filter())
	{
		if(filter == "PASS" || filter == "pass" || filter == "." || filter.isEmpty()) continue;
		if(!filter_ids.contains(filter))
		{
			FilterLine new_filter_line;
			new_filter_line.id = strToPointer(filter);
			new_filter_line.description = strToPointer("no description available");
			vcf_header_.addFilterLine(new_filter_line);
			filter_ids.insert(strToPointer(filter));
		}
	}

	//INFO
	if(line_parts[INFO]!=".")
	{
		QByteArrayList info_list = line_parts[INFO].split(';');
		QByteArrayList info_values;
		QList<ListOfInfoIds> info_ids_string_list;

		for(const QByteArray& info : info_list)
		{
			QByteArrayList key_value_pair = info.split('=');
			//check if the info is known in header
			if(!info_ids.contains(key_value_pair[0]))
			{
				InfoFormatLine new_info_line;
				new_info_line.id = strToPointer(key_value_pair[0]);
				new_info_line.number = strToPointer("1");
				new_info_line.type = strToPointer("String");
				new_info_line.description = strToPointer("no description available");
				vcf_header_.addInfoLine(new_info_line);
				info_ids.insert(strToPointer(key_value_pair[0]));
			}

			if(key_value_pair.size() == 1)
			{
				info_values.push_back(strToPointer(QByteArray("TRUE")));
				info_ids_string_list.push_back(strToPointer(key_value_pair[0]));
			}
			else
			{
				info_values.push_back(strToPointer(key_value_pair[1]));
				info_ids_string_list.push_back(strToPointer(key_value_pair[0]));
			}

		}

		//save info order
		ListOfInfoIds info_ids_string = info_ids_string_list.join();
		if(info_id_to_idx_list_.contains(info_ids_string))
		{
			vcf_line->setInfoIdToIdxPtr(info_id_to_idx_list_[info_ids_string]);
		}
		else
		{
			InfoIDToIdxPtr new_info_id_to_idx_entry = InfoIDToIdxPtr(new OrderedHash<QByteArray, int>);
			for(int i = 0; i < info_ids_string_list.count(); ++i)
			{
				new_info_id_to_idx_entry->push_back(info_ids_string_list.at(i), i);
			}
			info_id_to_idx_list_.insert(info_ids_string, new_info_id_to_idx_entry);
			vcf_line->setInfoIdToIdxPtr(info_id_to_idx_list_[info_ids_string]);
		}
		vcf_line->setInfo(info_values);

	}

	//FORMAT && SAMPLE
	if(line_parts.count() >= 9 && line_parts[FORMAT] != ".")
	{
		//FORMAT
		QByteArrayList format_list = line_parts[FORMAT].split(':');
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
				new_format_line.id = strToPointer(format);
				new_format_line.number = strToPointer("1");
				new_format_line.type = strToPointer("String");
				new_format_line.description = strToPointer("no description available");
				vcf_header_.addFormatLine(new_format_line);
				format_ids.insert(strToPointer(format));

				if(format == "GT")
				{
					vcf_header_.moveFormatLine(vcf_header_.formatLines().count()-1, 0);
				}
			}
		}

		//set format indices
		ListOfFormatIds format_ids_string = format_list.join();
		if(format_id_to_idx_list_.contains(format_ids_string))
		{
			vcf_line->setFormatIdToIdxPtr(format_id_to_idx_list_[format_ids_string]);
		}
		else
		{
			FormatIDToIdxPtr new_format_id_to_idx_entry = FormatIDToIdxPtr(new OrderedHash<QByteArray, int>);
			for(int i = 0; i < format_list.count(); ++i)
			{
				new_format_id_to_idx_entry->push_back(strToPointer(format_list.at(i)), i);
			}
			format_id_to_idx_list_.insert(format_ids_string, new_format_id_to_idx_entry);
			vcf_line->setFormatIdToIdxPtr(new_format_id_to_idx_entry);
		}

		//set samples idices
		vcf_line->setSampleIdToIdxPtr(sample_id_to_idx_);

		//SAMPLE
		if(line_parts.count() >= 10)
		{
			int last_column_to_parse;
			allow_multi_sample ? last_column_to_parse=line_parts.count() : last_column_to_parse=10;

			if(allow_multi_sample && sampleIDs().count() != line_parts.count() - 9)
			{
				THROW(FileParseException, "Number of samples does not equal number of samples in header for line " + QString::number(line_number) + ": " + line);
			}

			for(int i = 9; i < last_column_to_parse; ++i)
			{
				QByteArrayList sample_id_list = line_parts[i].split(':');

				int format_entry_count = vcf_line->formatKeys().count();
				int sample_entry_count = sample_id_list.count();
				//SAMPLE columns can have missing trailing entries, but can not have more than specified in FORMAT
				if(sample_entry_count > format_entry_count)
				{
					THROW(FileParseException, "Sample column has more entries than defined in Format for line " + QString::number(line_number) + ": " + line);
				}

				QByteArrayList format_values_for_sample;
				//parse all available entries
				for(int sample_id = 0; sample_id < sample_entry_count; ++sample_id)
				{
					QByteArray value = "";
					if(sample_id_list.at(sample_id) != ".") value = sample_id_list.at(sample_id);
					format_values_for_sample.append(strToPointer(value));
				}
				//set missing trailing entries
				if(sample_entry_count < format_entry_count)
				{
					for(int trailing_sample_id = (sample_entry_count - format_entry_count); trailing_sample_id < format_entry_count; trailing_sample_id++)
					{
						format_values_for_sample.append(strToPointer(""));
					}
				}
				vcf_line->addFormatValues(format_values_for_sample);
			}
		}
		else
		{
			//a FORMAT is given, however no SAMPLE data
			int format_count = 0;
			while(format_count < vcf_line->formatKeys().count())
			{
				QByteArrayList format_values_for_sample;
				format_values_for_sample.append(strToPointer(""));
				//since SAMPLE is empty, there MUST be only one sampleID (this is set in parseHeaderFields)
				vcf_line->addFormatValues(format_values_for_sample);

				++format_count;
			}
		}
	}

	vcf_lines_.push_back(vcf_line);
}

void VcfFile::processVcfLine(int& line_number, const QByteArray& line, QSet<QByteArray>& info_ids, QSet<QByteArray>& format_ids, QSet<QByteArray>& filter_ids, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert)
{
	++line_number;

	//skip empty lines
	if(line.trimmed().isEmpty()) return;

	//parse header
	if (line.startsWith("##"))
	{
		parseVcfHeader(line_number, line);
	}
	else if (line.startsWith("#CHROM"))
	{
		parseHeaderFields(line, allow_multi_sample);
		//all header lines hsould be read at this point
		for(const InfoFormatLine& format : vcf_header_.formatLines())
		{
			format_ids.insert(format.id);
		}
		for(const InfoFormatLine& info : vcf_header_.infoLines())
		{
			info_ids.insert(info.id);
		}
		for(const FilterLine& filter : vcf_header_.filterLines())
		{
			filter_ids.insert(filter.id);
		}
	}
	else
	{
		parseVcfEntry(line_number, line, info_ids, format_ids, filter_ids, allow_multi_sample, roi_idx, invert);
	}
}

//zlib also opens BGZF files (if index is not needed)
void VcfFile::loadFromVCFGZ(const QString& filename, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert)
{
	//clear content in case we load a second file
	clear();
	
	//open stream
	FILE* instream = filename.isEmpty() ? stdin : fopen(filename.toLatin1().data(), "rb");
	gzFile file = gzdopen(fileno(instream), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
	if (file==NULL)
	{
		THROW(FileAccessException, "Could not open file '" + filename + "' for reading!");
	}
	
	//parse
	int line_number = 0;
	const int buffer_size = 1048576; //1MB buffer
	char* buffer = new char[buffer_size]; 
	//Sets holding all INFO and FORMAT IDs defined in the header (might be extended if a vcf line contains new ones)
	QSet<QByteArray> info_ids_in_header;
	QSet<QByteArray> format_ids_in_header;
	QSet<QByteArray> filter_ids_in_header;
	while(!gzeof(file))
	{
		char* char_array = gzgets(file, buffer, buffer_size);
		//handle errors like truncated GZ file
		if (char_array==nullptr)
		{
			int error_no = Z_OK;
			QByteArray error_message = gzerror(file, &error_no);
			if (error_no!=Z_OK && error_no!=Z_STREAM_END)
			{
				THROW(FileParseException, "Error while reading file '" + filename + "': " + error_message);
			}
			
			continue;
		}
		
		//determine end of read line
		int i=0;
		while(i<buffer_size && char_array[i]!='\0' && char_array[i]!='\n' && char_array[i]!='\r')
		{
			++i;
		}
		
		QByteArray line = QByteArray::fromRawData(char_array, i);
		
		processVcfLine(line_number, line, info_ids_in_header, format_ids_in_header, filter_ids_in_header, allow_multi_sample, roi_idx, invert);
	}
	gzclose(file);
	delete[] buffer;
}

void VcfFile::load(const QString& filename, bool allow_multi_sample)
{
	loadFromVCFGZ(filename, allow_multi_sample);
}

void VcfFile::load(const QString& filename, const BedFile& roi, bool allow_multi_sample, bool invert)
{
	//create ROI index (if given)
	QScopedPointer<ChromosomalIndex<BedFile>> roi_idx;
	if (!roi.isSorted())
	{
		THROW(ArgumentException, "Target region unsorted, but needs to be sorted (given for reading file " + filename + ")!");
	}
	roi_idx.reset(new ChromosomalIndex<BedFile>(roi));

	loadFromVCFGZ(filename, allow_multi_sample, roi_idx.data(), invert);
}

void VcfFile::storeAsTsv(const QString& filename)
{
	//open stream
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename);
	QTextStream stream(file.data());

	for(const VcfHeaderLine& comment : vcfHeader().comments())
	{
		comment.storeLine(stream);
	}
	//write all DESCRIPTIONS
	//ID, QUAL and FILTER
	stream << "##DESCRIPTION=ID=ID of the variant, often dbSNP rsnumber\n";
	stream << "##DESCRIPTION=QUAL=Phred-scaled quality score\n";
	stream << "##DESCRIPTION=FILTER=Filter status\n";

	//in tsv format every info entry is a column, and every combination of format and sample
	for(InfoFormatLine info_line : vcfHeader().infoLines())
	{
		if(info_line.id=="." || info_line.description=="") continue;
		stream << "##DESCRIPTION=" + info_line.id + "_info=" + info_line.description << "\n";
	}
	for(InfoFormatLine format_line : vcfHeader().formatLines())
	{
		if(format_line.id=="." || format_line.description=="") continue;
		stream << "##DESCRIPTION=" + format_line.id + "_format=" + format_line.description << "\n";
	}

	//filter are added seperately
	for(const FilterLine& filter_line : vcfHeader().filterLines())
	{
		stream << "##FILTER=" << filter_line.id << "=" << filter_line.description << "\n";
	}

	//header
	stream << "#chr\tstart\tend\tref\tobs\tID\tQUAL\tFILTER";
	//one column for every INFO field
	for(const InfoFormatLine& info_line : vcfHeader().infoLines())
	{
		if(info_line.id==".") continue;
		stream << "\t" << info_line.id << "_info";
	}
	//one column for every combination of a FORMAT field and a SAMPLE
	for(const QByteArray& sample_id : sampleIDs())
	{
		for(const InfoFormatLine& format_line : vcfHeader().formatLines())
		{
			if(format_line.id==".") continue;
			stream << "\t" << sample_id << "_" << format_line.id << "_format";
		}
	}
	stream << "\n";

	//vcf lines
	for(VcfLinePtr& v : vcfLines())
	{
		//normalize variants and set symbol for empty sequence
		v->normalize("-", true);
		stream << v->chr().str() << "\t" << QByteArray::number(v->start()) << "\t" << QByteArray::number(v->end()) << "\t" << v->ref()
			   << "\t" << v->altString() << "\t" << v->id().join(';') << "\t" << QByteArray::number(v->qual());
		if(v->filter().empty())
		{
			stream << "\t.";
		}
		else
		{
			stream << "\t" << v->filter().join(';');
		}

		for(const QByteArray& info_key : informationIDs())
		{
			stream << "\t" << v->info(info_key);
		}
		for(const QByteArray& sample_id : sampleIDs())
		{
			for(const QByteArray& format_key : formatIDs())
			{
				stream << "\t" << v->formatValueFromSample(format_key, sample_id);
			}
		}
		stream << "\n";
	}
}

void writeZipped(gzFile& gz_file, QString& vcf_file_data, const QString& filename)
{
	int written = gzputs(gz_file, vcf_file_data.toLocal8Bit().data());
	if (written==0)
	{
		THROW(FileAccessException, "Could not write to file '" + filename + "'!");
	}
	vcf_file_data.clear();
}

void writeBGZipped(BGZF* instream, QString& vcf_file_data)
{
	const QByteArray utf8String = vcf_file_data.toUtf8();
	size_t length_bytes = utf8String.size();
	size_t written_bytes = bgzf_write(instream, utf8String.constData(), length_bytes);

	if(length_bytes != written_bytes)
	{
		THROW(FileAccessException, "Writing bgzipped vcf file failed; not all bytes were written.");
	}
}

void VcfFile::store(const QString& filename, bool stdout_if_file_empty, int compression_level) const
{
	if(compression_level == BGZF_NO_COMPRESSION)
	{
		//open stream
		QSharedPointer<QFile> file = Helper::openFileForWriting(filename, stdout_if_file_empty);
		QTextStream file_stream(file.data());

		//write header information
		vcf_header_.storeHeaderInformation(file_stream);

		//write header columns
		storeHeaderColumns(file_stream);

		//write vcf lines
		for(int i = 0; i < vcf_lines_.count(); ++i)
		{
			storeLineInformation(file_stream, vcfLine(i));
		}
	}
	else
	{
		if(filename.isEmpty())
		{
			THROW(ArgumentException, "Conflicting parameters for empty filename and compression level > 0");
		}
		if (compression_level<0 || compression_level>9) THROW(ArgumentException, "Invalid gzip compression level '" + QString::number(compression_level) +"' given for VCF file '" + filename + "'!");

		std::string compression_mode_string = "wb";
		std::string compression_mode_level_string = std::to_string(compression_level);
		compression_mode_string.append(compression_mode_level_string);

		const char* compression_mode = compression_mode_string.c_str();
		BGZF* instream = bgzf_open(filename.toLatin1().data(), compression_mode);
		if (instream==NULL)
		{
			THROW(FileAccessException, "Could not open file '" + filename + "' for writing!");
		}

		//write gzipped informations
		//open stream
		QString vcf_file;
		QTextStream stream(&vcf_file);

		//write header information
		vcf_header_.storeHeaderInformation(stream);
		//write header columns
		storeHeaderColumns(stream);

		for(int i = 0; i < vcf_lines_.count(); ++i)
		{
			storeLineInformation(stream, vcfLine(i));
		}

		writeBGZipped(instream, vcf_file);

		bgzf_close(instream);
	}

}

void VcfFile::leftNormalize(QString reference_genome)
{
	for(VcfLinePtr& variant_line : vcfLines())
	{
		variant_line->leftNormalize(reference_genome);
	}
}

void VcfFile::sort(bool use_quality)
{
	if (vcfLines().count()==0) return;
	std::sort(vcf_lines_.begin(), vcf_lines_.end(), VcfFormat::LessComparator(use_quality));

}

void VcfFile::sortByFile(QString filename)
{
	sortCustom(VcfFormat::LessComparatorByFile(filename));
}

void VcfFile::removeDuplicates(bool sort_by_quality)
{
	sort(sort_by_quality);

	//remove duplicates (same chr, start, obs, ref) - avoid linear time remove() calls by copying the data to a new vector.
	QVector<VcfLinePtr> output;
	output.reserve(vcfLines().count());
	for (int i=0; i<vcfLines().count()-1; ++i)
	{
		int j = i+1;
		if (vcf_lines_.at(i)->chr() != vcf_lines_.at(j)->chr() || vcf_lines_.at(i)->start() != vcf_lines_.at(j)->start() || vcf_lines_.at(i)->ref() !=vcf_lines_.at(j)->ref() || !qEqual(vcf_lines_.at(i)->alt().begin(),  vcf_lines_.at(i)->alt().end(), vcf_lines_.at(j)->alt().begin()))
		{
			output.append(vcf_lines_.at(i));
		}
	}
	if (!vcf_lines_.isEmpty())
	{
		output.append(vcf_lines_.last());
	}

	//swap the old and new vector
	vcf_lines_.swap(output);
}

QByteArrayList VcfFile::sampleIDs() const
{
	if(!sample_id_to_idx_ || sample_id_to_idx_->empty())
	{
		QByteArrayList empty_list;
		return empty_list;
	}
	return sample_id_to_idx_->keys();
}

QByteArrayList VcfFile::informationIDs() const
{
	QByteArrayList informations;
	for(const InfoFormatLine& info : vcfHeader().infoLines())
	{
		informations.append(info.id);
	}
	return informations;
}

QByteArrayList VcfFile::filterIDs() const
{
	QByteArrayList filters;
	for(const FilterLine& filter : vcfHeader().filterLines())
	{
		filters.append(filter.id);
	}
	return filters;
}

QByteArrayList VcfFile::formatIDs() const
{
	QByteArrayList formats;
	for(const InfoFormatLine& format : vcfHeader().formatLines())
	{
		formats.append(format.id);
	}
	return formats;
}

void VcfFile::storeLineInformation(QTextStream& stream, VcfLine line) const
{
	//chr
	stream << line.chr().str()  << "\t" << line.start();

	//if id exists
	if(!line.id().empty())
	{
		stream  << "\t"<< line.id().join(';');
	}
	else
	{
		stream << "\t.";
	}

	//ref and alt
	stream  << "\t"<< line.ref();
	stream << "\t" << line.alt().at(0);
	if(line.alt().count() > 1)
	{
		for(int i = 1; i < line.alt().size(); ++i)
		{
			stream  << "," <<  line.alt().at(i);
		}
	}

	//quality
	QByteArray quality;
	if(line.qual() == -1)
	{
		quality = ".";
	}
	else
	{
		quality.setNum(line.qual());
	}
	stream  << "\t"<< quality;

	//if filter exists
	if(!line.filter().empty() && !(line.filter().count() == 1 && line.filter().first() == ""))
	{
		stream  << "\t"<< line.filter().join(';');
	}
	else
	{
		stream << "\t.";
	}

	//if info exists
	if(line.infoKeys().empty())
	{
		stream << "\t.";
	}
	else
	{
		//if info is only TRUE, print key only
		QByteArray info_line_value = line.infoValues().at(0);
		QByteArray info_line_key = line.infoKeys().at(0);
		if(info_line_value == "TRUE" && vcfHeader().infoLineByID(info_line_key, false).type == "Flag")
		{
			stream  << "\t"<< line.infoKeys().at(0);
		}
		else
		{
			stream  << "\t"<< line.infoKeys().at(0) << "=" << line.infoValues().at(0);;
		}
		if(line.infoKeys().size() > 1)
		{
			for(int i = 1; i < line.infoKeys().size(); ++i)
			{
				QByteArray info_line_value = line.infoValues().at(i);
				QByteArray info_line_key = line.infoKeys().at(i);
				if(info_line_value == "TRUE" && vcfHeader().infoLineByID(info_line_key).type == "Flag")
				{
					stream  << ";"<< line.infoKeys().at(i);
				}
				else
				{
					stream  << ";"<< line.infoKeys().at(i) << "=" << line.infoValues().at(i);;
				}
			}
		}
	}

	//if format exists
	if(!line.formatKeys().empty())
	{
		QString all_format_keys = line.formatKeys().join(":");
		stream << "\t" << all_format_keys;
	}
	else
	{
		stream << "\t.";
	}

	//if sample exists
	if(!line.samples().empty())
	{
		//for every sample
		for(int sample_idx = 0; sample_idx < line.samples().size(); ++sample_idx)
		{
			QByteArrayList sample_entry = line.sample(sample_idx);
			if(sample_entry.empty())
			{
				stream << "\t.";
			}
			else
			{
				//for all entries in the sample (e.g. 'GT':'DP':...)
				QByteArrayList values;
				for(int sample_entry_id = 0; sample_entry_id < sample_entry.size(); ++sample_entry_id)
				{
					QByteArray value = ".";
					if(sample_entry.at(sample_entry_id) != "") value = sample_entry.at(sample_entry_id);
					values.append(value);
				}
				stream << "\t" << values.join(":");
			}
		}
	}
	else
	{
		stream << "\t.";
	}
	stream << "\n";
}

QString VcfFile::lineToString(int pos) const
{
	QString line;
	QTextStream stream(&line);
	storeLineInformation(stream, vcfLine(pos));
	return line;
}

void VcfFile::storeHeaderColumns(QTextStream &stream) const
{
	if(vcfColumnHeader().count() < MIN_COLS)
	{
		THROW(ArgumentException, "Number of column headers is " + QString::number(vcfColumnHeader().count()) + ", but the minimum expected number of columns is: " + QString::number(MIN_COLS) + ".");
	}

	stream << "#";
	for(int i = 0; i < column_headers_.count() - 1; ++i)
	{
		stream << column_headers_.at(i) << "\t";
	}
	stream << column_headers_.at(column_headers_.count() - 1) << "\n";
}

void VcfFile::copyMetaDataForSubsetting(const VcfFile& rhs)
{
	//copy header information
	vcf_header_ = rhs.vcfHeader();

	for(const QByteArray& header : rhs.vcfColumnHeader())
	{
		column_headers_.push_back(header);
	}

	//copy information about samples and possible info/ format lines
	sample_id_to_idx_ = rhs.sampleIDToIdx();
	format_id_to_idx_list_ = rhs.formatIDToIdxList();
	info_id_to_idx_list_ = rhs.infoIDToIdxList();
}

QByteArray VcfFile::toText() const
{
	//open stream
	QString output;
	QTextStream stream(&output);

	//write header information
	vcf_header_.storeHeaderInformation(stream);
	//write header columns
	storeHeaderColumns(stream);

	for(int i = 0; i < vcf_lines_.count(); ++i)
	{
		storeLineInformation(stream, vcfLine(i));
	}

	return output.toUtf8();
}

void VcfFile::fromText(const QByteArray &text)
{
	//clear content in case we load a second file
	clear();
    QByteArrayList lines = text.split('\n');

	int line_number = 0;
	QSet<QByteArray> info_ids_in_header;
	QSet<QByteArray> format_ids_in_header;
	QSet<QByteArray> filter_ids_in_header;

	foreach (const QByteArray& line, lines)
	{
		processVcfLine(line_number, line, info_ids_in_header, format_ids_in_header, filter_ids_in_header, true, nullptr, false);
	}
}

VcfFile VcfFile::convertGSvarToVcf(const VariantList& variant_list, const QString& reference_genome)
{
	VcfFile vcf_file;

	//store comments
	int line=0;
	foreach(const QString& comment, variant_list.comments())
	{
		QByteArray utf8_comment = comment.toUtf8();
		if(utf8_comment.startsWith("##fileformat"))
		{
			vcf_file.vcf_header_.setFormat(utf8_comment);
		}
		else
		{
			vcf_file.vcf_header_.setCommentLine(utf8_comment, line);
		}
	}
	//fileformat must always be set in vcf
	if(vcf_file.vcf_header_.fileFormat().isEmpty())
	{
		QByteArray format = "##fileformat=VCFv4.2";
		vcf_file.vcf_header_.setFormat(format);
	}

	//store all columns as INFO
	for (int j = 0; j < variant_list.annotationDescriptions().count(); ++j)
	{
		const VariantAnnotationDescription& anno_description = variant_list.annotationDescriptions()[j];

		InfoFormatLine info_line;

		info_line.id = anno_description.name().toUtf8();
		info_line.number = ".";

		QByteArray utf8_type;
		switch (anno_description.type()) {
			case VariantAnnotationDescription::INTEGER:
				utf8_type =  "Integer";
				break;
			case VariantAnnotationDescription::FLOAT:
				utf8_type =  "Float";
				break;
			case VariantAnnotationDescription::FLAG:
				utf8_type =  "Flag";
				break;
			case VariantAnnotationDescription::CHARACTER:
				utf8_type =  "Character";
				break;
			case VariantAnnotationDescription::STRING:
				utf8_type =  "String";
				break;
			default:
				THROW(ProgrammingException, "Unknown AnnotationType '" + QString::number(anno_description.type()) + "'!");
		}
		info_line.type = utf8_type;

		QString desc = anno_description.description();
		info_line.description = (desc!="" ? desc : "no description available");

		vcf_file.vcf_header_.addInfoLine(info_line);
	}

	//write filter headers
	auto it = variant_list.filters().cbegin();
	while(it != variant_list.filters().cend())
	{
		FilterLine filter_line;
		filter_line.id = it.key().toUtf8();
		filter_line.description = it.value();
		vcf_file.vcf_header_.addFilterLine(filter_line);
		++it;
	}

	//add header fields
	vcf_file.column_headers_ << "CHROM" << "POS" << "ID" << "REF" << "ALT" << "QUAL" << "FILTER" << "INFO" << "FORMAT";
	//search for genotype on annotations
	SampleHeaderInfo genotype_columns;
	try
	{
		genotype_columns = variant_list.getSampleHeader();
	}
	catch(...){} //nothing to do here

	//write genotype Format into header
	if(!genotype_columns.empty())
	{
		for(const SampleInfo& genotype : genotype_columns)
		{
			vcf_file.column_headers_ << genotype.column_name.toUtf8();
		}

		InfoFormatLine format_line;
		format_line.id = "GT";
		format_line.number = "1";
		format_line.type = "String";
		format_line.description = "Genotype";
		vcf_file.vcf_header_.addFormatLine(format_line);
	}

	int qual_index = variant_list.annotationIndexByName("QUAL", true, false);
	int filter_index = variant_list.annotationIndexByName("FILTER", true, false);
	QSet<int> indices_to_skip;
	indices_to_skip.insert(qual_index);
	indices_to_skip.insert(filter_index);
	for(const SampleInfo& genotype : genotype_columns)
	{
		indices_to_skip.insert(genotype.column_index);
	}

	//sample order is set for all lines
	SampleIDToIdxPtr sample_id_to_idx = SampleIDToIdxPtr(new OrderedHash<QByteArray, int>);
	sample_id_to_idx = SampleIDToIdxPtr(new OrderedHash<QByteArray, int>);

	for(int i = 0; i < genotype_columns.size(); ++i)
	{
		const SampleInfo& genotype = genotype_columns.at(i);
		sample_id_to_idx->push_back(genotype.column_name.toUtf8(), i);
	}

	//format contains only GT
	QHash<ListOfFormatIds, FormatIDToIdxPtr> format_id_to_idx_list;
	ListOfFormatIds list_storing_genotype_only;
	list_storing_genotype_only.push_back("GT");
	FormatIDToIdxPtr gt_to_first_position = FormatIDToIdxPtr(new OrderedHash<QByteArray, int>);
	gt_to_first_position->push_back("GT", 1);
	format_id_to_idx_list.insert(list_storing_genotype_only, gt_to_first_position);

	QHash<ListOfInfoIds, InfoIDToIdxPtr> info_id_to_idx_list;

	//add variant lines
	for(int i = 0; i < variant_list.count(); ++i)
	{
		Variant v = variant_list[i];
		VcfLinePtr vcf_line = VcfLinePtr(new VcfLine);
		vcf_line->setFormatIdToIdxPtr(gt_to_first_position);
		vcf_line->setSampleIdToIdxPtr(sample_id_to_idx);

		QByteArrayList id_list;
		id_list.push_back(".");
		vcf_line->setId(id_list);
		if(qual_index >= 0)
		{
			vcf_line->setQual(v.annotations().at(qual_index).toDouble());
		}
		else
		{
			int quality_index = variant_list.annotationIndexByName("quality", true, false);
			if(quality_index >= 0)
			{
				QByteArrayList quality_list = v.annotations().at(quality_index).split(';');
				for(const QByteArray& element : quality_list)
				{
					if(element.startsWith("QUAL"))
					{
						QByteArrayList quality = element.split('=');
						//could not parse QUAL from quality column
						if(quality.count() < 2) continue;
						bool quality_ok;
						double qual = quality.at(1).toDouble(&quality_ok);
						//could not parse number from quality column
						if (!quality_ok) continue;
						vcf_line->setQual(qual);
						QByteArrayList annotations = v.annotations();
						//remove the QUAL entry from quality column(element length plus ;)
						annotations[quality_index].remove(0, element.length() + 1);
						v.setAnnotations(annotations);
					}
				}
			}

		}
		if(filter_index >= 0)
		{
			vcf_line->setFilter(v.annotations().at(filter_index).split(';'));
		}
		vcf_line->setChromosome(v.chr());
		vcf_line->setPos(v.start());
		vcf_line->setRef(v.ref());
		vcf_line->addAlt(v.obs());

		//add all columns into info
		QByteArrayList info;
		QByteArrayList all_info_keys;
		QList<int> all_positions;

		for (int i=0; i<v.annotations().count(); ++i)
		{
			if(indices_to_skip.contains(i)) continue;
			const VariantAnnotationHeader& anno_header = variant_list.annotations()[i];
			const VariantAnnotationDescription& anno_desc = variant_list.annotationDescriptionByName(anno_header.name(), false);
			QByteArray anno_val = v.annotations()[i];

			if (anno_val!="")
			{

				if (anno_desc.type()==VariantAnnotationDescription::FLAG) //Flags should not have values in VCF
				{
					info.push_back(strToPointer("TRUE"));
				}
				else //everything else is just added to info
				{
					info.push_back(strToPointer(anno_val));
				}

				all_info_keys.push_back(anno_header.name().toUtf8());
				all_positions.push_back(i);
			}
		}

		//save info order
		ListOfInfoIds info_ids_string = all_info_keys.join();
		if(info_id_to_idx_list.contains(info_ids_string))
		{
			vcf_line->setInfoIdToIdxPtr(info_id_to_idx_list[info_ids_string]);
		}
		else
		{
			InfoIDToIdxPtr new_info_id_to_idx_entry = InfoIDToIdxPtr(new OrderedHash<QByteArray, int>);
			for(int i = 0; i < all_info_keys.count(); ++i)
			{
				new_info_id_to_idx_entry->push_back(all_info_keys.at(i), all_positions.at(i));
			}
			info_id_to_idx_list.insert(info_ids_string, new_info_id_to_idx_entry);
			vcf_line->setInfoIdToIdxPtr(new_info_id_to_idx_entry);
		}
		vcf_line->setInfo(info);

		//write genotype
		if(!genotype_columns.empty())
		{

			QList<QByteArrayList> all_samples_new;
			bool all_samples_empty = true;

			for(const SampleInfo& genotype : genotype_columns)
			{
				QByteArrayList formats_new;

				int genotype_index = variant_list.annotationIndexByName(genotype.column_name, true, false);
				if(genotype_index == -1)
				{
					all_samples_new.push_back(formats_new);
					continue;
				}
				if(v.annotations().at(genotype_index).isEmpty() || v.annotations().at(genotype_index) == ".")
				{
					continue;
				}
				else if(v.annotations().at(genotype_index) == "wt")
				{
					formats_new.push_back("0/0");
				}
				else if(v.annotations().at(genotype_index) == "hom")
				{
					formats_new.push_back("1/1");
				}
				else if(v.annotations().at(genotype_index) == "het")
				{
					formats_new.push_back("1/0");
				}
				else
				{
					THROW(ArgumentException, "genotype column in TSV file does not contain a valid entry.");
				}
				all_samples_empty = false;
				all_samples_new.push_back(formats_new);
			}
			vcf_line->setSample(all_samples_new);
			//we store only the genotype in the sample columns, however if it is not present
			//format information has to be reset as well
			if(all_samples_empty) vcf_line->setFormatIdToIdxPtr(nullptr);
		}

		vcf_file.vcf_lines_.push_back(vcf_line);
	}

	for(VcfLinePtr& v_line : vcf_file.vcfLines())
	{
		//add base for INSERTION
		QByteArray ref = v_line->ref().toUpper();
		if (ref.size() == 1 && !(ref=="N" || ref=="A" || ref=="C" || ref=="G" || ref=="T")) //empty seq symbol in ref
		{
			FastaFileIndex reference(reference_genome);
			QByteArray base = reference.seq(v_line->chr(), v_line->start() - 1, 1);

			QList<Sequence> alt_seq;
			//for GSvar there is only one alternative sequence (alt(0) stores VariantList.obs(0))
			Sequence new_alt = base + v_line->alt(0);
			alt_seq.push_back(new_alt);
			v_line->setAlt(alt_seq);
			v_line->setRef(base);
		}

		//add base for DELETION
		QByteArray alt = v_line->alt(0).toUpper();
		if (alt.size() == 1 && !(alt=="N" || alt=="A" || alt=="C" || alt=="G" || alt=="T")) //empty seq symbol in alt
		{
			FastaFileIndex reference(reference_genome);
			QByteArray base = reference.seq(v_line->chr(), v_line->start() - 1, 1);
			QByteArray new_ref = base + v_line->ref();
			v_line->setSingleAlt(base);
			v_line->setRef(new_ref);
			v_line->setPos(v_line->start() - 1);
		}

	}
	vcf_file.leftNormalize(reference_genome);

	return vcf_file;
}

bool VcfFile::isValid(QString filename, QString ref_file, QTextStream& out_stream, bool print_general_information, int max_lines)
{
	//open input file
	FILE* instream = filename.isEmpty() ? stdin : fopen(filename.toLatin1().data(), "rb");
	gzFile file = gzdopen(fileno(instream), "rb"); //always open in binary mode because windows and mac open in text mode
	if (file==NULL)
	{
		THROW(FileAccessException, "Could not open file '" + filename + "' for reading!");
	}
	
	//open reference genome
	FastaFileIndex reference(ref_file);

	//load MISO terms
	OntologyTermCollection obo_terms("://Resources/so-xp_3_0_0.obo", true);

	//ALT allele regexp
	QRegExp alt_regexp("[ACGTN]+");

	//create list of all invalid chars in INFO column values
	QList<char> invalid_chars;
	foreach(KeyValuePair kvp, INFO_URL_MAPPING)
	{
		//skip '%' since it is not forbidden but used for URL encoding
		if (kvp.key.contains('%')) continue;
		//skip ',' since it is allowed to divide multiple INFO values
		if (kvp.key.contains(',')) continue;

		invalid_chars.append(kvp.key[0].toLatin1());
	}


	//perform checks
	QMap<QByteArray, DefinitionLine> defined_filters;
	QMap<QByteArray, DefinitionLine> defined_formats;
	QMap<QByteArray, DefinitionLine> defined_infos;
	QByteArrayList defined_samples;
	int expected_parts = MIN_COLS;
	bool in_header = true;
	int c_data = 0;
	int l = 0;
	const int buffer_size = 1048576; //1MB buffer
	char* buffer = new char[buffer_size];
	while(!gzeof(file) && l<max_lines)
	{
		++l;

		// get next line
		char* char_array = gzgets(file, buffer, buffer_size);

		//handle errors like truncated GZ file
		if (char_array==nullptr)
		{
			int error_no = Z_OK;
			QByteArray error_message = gzerror(file, &error_no);
			if (error_no!=Z_OK && error_no!=Z_STREAM_END)
			{
				THROW(FileParseException, "Error while reading file '" + filename + "': " + error_message);
			}
			
			continue;
		}
		
		//determine end of read line
		int i=0;
		while(i<buffer_size && char_array[i]!='\0' && char_array[i]!='\n' && char_array[i]!='\r')
		{
			++i;
		}
		
		//skip empty lines
		QByteArray line = QByteArray::fromRawData(char_array, i).trimmed();
		if (line.isEmpty()) continue;

		//check first line (VCF format)
		if (l==1)
		{
			if (!line.startsWith("##fileformat=VCFv"))
			{
				printError(out_stream, "First line must be 'fileformat' line!", l, line);
				return false;
			}
			if (print_general_information)
			{
				printInfo(out_stream, "VCF version: " + line.mid(17));
			}
		}

		if (line.startsWith("#"))
		{
			//check all header lines are at the beginning of the file
			if (!in_header)
			{
				printError(out_stream, "Header lines are not allowed in VCF body!", l, line);
				return false;
			}

			//##INFO=<ID=NS,Number=1,Type=Integer,Description="Number of samples with data">
			if (line.startsWith("##INFO=<"))
			{
				DefinitionLine data = parseDefinitionLine(out_stream, l, line);

				//check for duplicates
				if (defined_infos.contains(data.id))
				{
					printError(out_stream, "INFO '" + data.id + "' defined twice!", l, line);
					return false;
				}

				defined_infos[data.id] = data;
			}

			//##FORMAT=<ID=GT,Number=1,Type=String,Description="Genotype">
			else if (line.startsWith("##FORMAT=<"))
			{
				DefinitionLine data = parseDefinitionLine(out_stream, l, line);

				//check for duplicates
				if (defined_formats.contains(data.id))
				{
					printError(out_stream, "FORMAT '" + data.id + "' defined twice!", l, line);
					return false;
				}

				defined_formats[data.id] = data;
			}

			//##FILTER=<ID=off-target,Description="Variant marked as 'off-target'.">
			else if (line.startsWith("##FILTER=<"))
			{
				DefinitionLine data = parseDefinitionLine(out_stream, l, line);

				//check for duplicates
				if (defined_filters.contains(data.id))
				{
					printError(out_stream, "FILTER '" + data.id + "' defined twice!", l, line);
					return false;
				}

				defined_filters[data.id] = data;
			}
			//other ## header lines
			else if (line.startsWith("##"))
			{
				//not much to check here
			}
			else //main header line
			{
				QByteArrayList parts = line.split('\t');

				if (parts.count() < MIN_COLS)
				{
					printError(out_stream, "Header line with less than 8 fields!", l, line);
					return false;
				}
				if (parts.count()==9)
				{
					printError(out_stream, "Header line with FORMAT, but without samples!", l, line);
					return false;
				}
				if (parts.count()>9)
				{
					defined_samples = parts.mid(9);
					expected_parts = 9 + defined_samples.count();
				}

				//flag as not in header anymore
				in_header = false;
			}
		}
		//data line: chr1	62732421	rs11207949	T	C	13777.5	off-target	AB=0.515753;ABP=4.85745;AC=1;AF=0.5;AN=2;AO=442;CIGAR=1X;DP=857;DPB=857;DPRA=0;EPP=3.50158;EPPR=3.89459;GTI=0;LEN=1;MEANALT=1;MQM=60;MQMR=60;NS=1;NUMALT=1;ODDS=3022.34;PAIRED=0.995475;PAIREDR=1;PAO=0;PQA=0;PQR=0;PRO=0;QA=16752;QR=15862;RO=415;RPL=437;RPP=919.863;RPPR=827.694;RPR=5;RUN=1;SAF=221;SAP=3.0103;SAR=221;SRF=210;SRP=3.14111;SRR=205;TYPE=snp;technology.ILLUMINA=1;CSQ=C|missense_variant|MODERATE|KANK4|KANK4|transcript|NM_181712.4|Coding|6/10|c.2302A>G|p.Thr768Ala|2679/5477|2302/2988|768/995||;dbNSFP_Polyphen2_HDIV_pred=B,B;dbNSFP_phyloP100way_vertebrate=2.380000;dbNSFP_MutationTaster_pred=P;dbNSFP_Polyphen2_HVAR_pred=B,B;dbNSFP_SIFT_pred=T;ESP6500EA_AF=0.2449;ESP6500AA_AF=0.2229;1000G_AF=0.305511;EXAC_AF=0.284	GT:GL:DP:RO:QR:AO:QA	0/1:-1506.31,0,-1426.21:857:415:15862:442:16752
		else
		{
			++c_data;

			//split line
			QByteArrayList parts = line.split('\t');

			//check that the number of elements is correct
			if (parts.count()<expected_parts)
			{
				printError(out_stream, "Data line with " + QByteArray::number(parts.count()) + " elements, expected " + QByteArray::number(expected_parts) + "!", l, line);
				return false;
			}

			//chromosome
			Chromosome chr(parts[CHROM]);
			if (chr.str().contains(":"))
			{
				printError(out_stream, "Chromosome '" + parts[CHROM] + "' is not valid!", l, line);
				return false;
			}

			//position
			bool pos_is_valid;
			int pos = parts[POS].toInt(&pos_is_valid);
			if(!pos_is_valid)
			{
				printError(out_stream, "Chromosomal position '" + parts[POS] + "' is not a number!", l, line);
				return false;
			}

			//reference base
			QByteArray ref = parts[REF].toUpper();
			if (pos_is_valid)
			{
				Sequence ref_exp = reference.seq(chr, pos, ref.length());
				if (ref!=ref_exp)
				{
					if (ref_exp=="N" || ref_exp=="A" || ref_exp=="C" || ref_exp=="G" || ref_exp=="T") //ignore ambiguous bases, e.g. M or R.
					{
						printError(out_stream, "Reference base(s) not correct. Is '" + ref + "', should be '" + ref_exp + "'!", l, line);
					}
				}
			}

			//alternate base(s)
			QByteArrayList alts = parts[ALT].split(',');
			if (alts.count()==1 && alts[0]==".")
			{
				printWarning(out_stream, "Missing value '.' used as alternative allele!", l, line);
			}
			else
			{
				foreach(const QByteArray& alt, alts)
				{
					if (alt.startsWith('<') && alt.endsWith('>')) continue; //special case for structural variant
					if (alt=="*") continue; //special case for missing allele due to downstream deletion
					if (alt.isEmpty() || !alt_regexp.exactMatch(alt))
					{
						printError(out_stream, "Invalid alternative allele '" + alt + "'!", l, line);
					}
				}
			}

			//quality
			const QByteArray& qual = parts[QUAL];
			if (qual!=".")
			{
				bool ok = false;
				qual.toDouble(&ok);
				if (!ok)
				{
					printError(out_stream, "Invalid quality value '" + qual + "'!", l, line);
					return false;
				}
			}

			//filter
			const QByteArray& filter = parts[FILTER];
			if (filter!="." && filter!="PASS")
			{
				QByteArrayList filters = filter.split(';');
				foreach(const QByteArray& name, filters)
				{
					if (!defined_filters.contains(name))
					{
						printWarning(out_stream, "FILTER '" + name + "' used but not defined!", l, line);
					}
					else
					{
						defined_filters[name].used +=1;
					}
				}
			}

			//info
			//allow empty INFO column only with missing value:
			if (parts[INFO].trimmed() == "")
			{
				printError(out_stream, "INFO column is empty! Has to contain either INFO values or missing value '.'!", l, line);
				return false;
			}
			//skip INFO validation if column is empty
			if (parts[INFO] != ".")
			{
				QByteArrayList info = parts[INFO].split(';');
				foreach(const QByteArray& entry, info)
				{
					int sep = entry.indexOf('=');
					bool has_value = sep!=-1;
					QByteArray name = has_value ? entry.left(sep) : entry;
					QByteArray value = has_value ? entry.mid(sep+1).trimmed() : "";

					bool is_defined = defined_infos.contains(name);
					if (is_defined)
					{
						defined_infos[name].used +=1;
					}
					else
					{
						printWarning(out_stream, "INFO '" + name + "' used but not defined!", l, line);
					}

					//check flags
					if (is_defined)
					{
						if (defined_infos[name].type!="Flag" && !has_value)
						{
							printError(out_stream, "Non-flag INFO '" + name + "' has no value!", l, line);
							return false;
						}
						if (defined_infos[name].type=="Flag" && has_value)
						{
							printError(out_stream, "Flag INFO '" + name + "' has a value (" + value + ")!", l, line);
							return false;
						}
					}

					//check INFO value for invalid characters
					foreach (char invalid_char, invalid_chars)
					{
						if (value.contains(invalid_char))
						{
							printError(out_stream, "Flag INFO '" + name + "' has a value which contains the invalid character '" + invalid_char + "' (value: '" + value + "')!", l, line);
							return false;
						}
					}



					//check value (number, type)
					if (is_defined && has_value)
					{
						const DefinitionLine& current_info = defined_infos[name];
						QByteArrayList values = value.split(',');
						checkValues(current_info, values, alts.count(), QByteArray(), out_stream, l, line);
					}

					//check MISO ontology entries in CSQ:IMPACT (split by &)
					if (name=="CSQ")
					{
						QByteArrayList csq_defs = defined_infos[name].description.split('|');
						QByteArrayList csq_transcripts = value.split(',');
						int i_consequence = csq_defs.indexOf("Consequence");
						foreach(const QByteArray& csq_transcript, csq_transcripts)
						{
							QByteArrayList csq_parts = csq_transcript.split('|');
							if (csq_parts.count()!=csq_defs.count())
							{
								printError(out_stream, "VEP-based CSQ annotation has " + QByteArray::number(csq_parts.count()) + " entries, expected " + QByteArray::number(csq_defs.count()) + " according to definition in header!", l, line);
								return false;
							}

							QByteArrayList terms = csq_parts[i_consequence].split('&');
							foreach(const QByteArray& term, terms)
							{
								if(!obo_terms.containsByName(term))
								{
									printWarning(out_stream, "Unknown MISO term '" + term + "' used!", l, line);
								}
							}

						}
					}
				}
			}


			//format
			if (parts.count()==8) continue;
			QByteArrayList format_names = parts[FORMAT].split(':');
			foreach(const QByteArray& name, format_names)
			{
				if (!defined_formats.contains(name))
				{
					printWarning(out_stream, "FORMAT '" + name + "' used but not defined!", l, line);
				}
				else
				{
					defined_formats[name].used +=1;
				}

				//special handling of "GT" field
				if (name=="GT" && format_names.indexOf(name)!=0)
				{
					printError(out_stream, "FORMAT 'GT' must be first format field!", l, line);
					return false;
				}
			}

			//samples
			for (int s=0; s<defined_samples.count(); ++s)
			{
				if (parts[9+s] == ".") { // ignore MISSING sample
					continue;
				}

				QByteArrayList sample_data = parts[9+s].split(':');

				//check the number of entries
				if (format_names.count()!=sample_data.count())
				{
					printError(out_stream, "Sample " + defined_samples[s] + " has " + QByteArray::number(sample_data.count()) + " entries, expected " + QByteArray::number(format_names.count()) + " according to FORMAT entry!", l, line);
					return false;
				}

				//check values (number, type)
				for (int i=0; i<format_names.count(); ++i)
				{
					if (sample_data[i] == ".") { // ignore MISSING sample
						continue;
					}
					const QByteArray& name = format_names[i];
					const DefinitionLine& current_format = defined_formats[name];
					QByteArrayList values = sample_data[i].split(',');
					checkValues(current_format, values, alts.count(), defined_samples[s], out_stream, l, line);

					//special handling of GT column
					if (name=="GT")
					{
						QByteArrayList gt_entries = values[0].replace('/', '|').split('|');
						foreach(const QByteArray& gt_entry, gt_entries)
						{
							bool ok;
							int allele_number = gt_entry.toInt(&ok);
							if((gt_entry!="." && !ok) || (ok && allele_number>alts.count()))
							{
								printError(out_stream, "Sample " + defined_samples[s] + " has invalid GT entry '" + values[0] + "'!", l, line);
								return false;
							}
						}
					}
				}
			}
		}
	}
	
	//delete resources
	gzclose(file);
	delete[] buffer;

	//output infos
	if (print_general_information)
	{
		foreach(const DefinitionLine& filter, defined_filters)
		{
			printInfo(out_stream, "FILTER: " + filter.toString());
		}
		foreach(const DefinitionLine& filter, defined_infos)
		{
			printInfo(out_stream, "INFO: " + filter.toString());
		}
		foreach(const DefinitionLine& filter, defined_formats)
		{
			printInfo(out_stream, "FORMAT: " + filter.toString());
		}
		foreach(const QByteArray& sample, defined_samples)
		{
			printInfo(out_stream, "SAMPLE: " + sample);
		}
		printInfo(out_stream, "Finished - checked " + QByteArray::number(l) + " lines - " + QByteArray::number(c_data) + " data lines.");
	}

	return true;
}

VcfFile::DefinitionLine VcfFile::parseDefinitionLine(QTextStream& out, int l, QByteArray line)
{
	if (!line.endsWith(">"))
	{
		printError(out, "Character '>' at end missing!", l ,line);
	}

	int start = line.indexOf('<');
	if (start==-1)
	{
		printError(out, "Character '<' at beginning missing!", l ,line);
	}

	QByteArray def_type = line.mid(2, start-3);

	DefinitionLine output;
	QByteArrayList parts = line.mid(start+1, line.length()-start-2).split(',');
	foreach(const QByteArray& entry, parts)
	{
		int sep = entry.indexOf('=');
		if (sep==-1)
		{
			output.description += entry;
		}
		else
		{
			QByteArray name = entry.left(sep).trimmed();
			QByteArray value = entry.mid(sep+1).trimmed();
			if (name=="ID")
			{
				output.id = value;
			}
			else if (name=="Description")
			{
				output.description = value;
			}
			else if (name=="Number")
			{
				output.number = value;
			}
			else if (name=="Type")
			{
				output.type = value;
			}
		}
	}

	if (output.id.isEmpty())
	{
		printError(out, "Entry 'ID' missing!", l, line);
	}

	if (output.description.isEmpty())
	{
		printError(out, "Entry 'Description' missing!", l, line);
	}

	if (!output.number.isEmpty())
	{
		if (def_type!="FORMAT" && def_type!="INFO")
		{
			printError(out, def_type+" definition cannot have a 'Number' entry!", l, line);
		}

		if (output.type=="Flag" && output.number!="0")
		{
			printError(out, def_type+" definition of 'Flag' has 'Number' value other than '0'", l, line);
		}
		if (output.type!="Flag" && output.number!="." && output.number!="G" && output.number!="A" && output.number!="R" && output.number.toInt()<1)
		{
			printError(out, def_type+" definition has invalid 'Number' field ", l, line);
		}
	}

	if (!output.type.isEmpty())
	{
		if (def_type!="FORMAT" && def_type!="INFO")
		{
			printError(out, def_type+" definition cannot have a 'Number' entry!", l, line);
		}

		if (output.type!="Integer" && output.type!="Float" && output.type!="Character" && output.type!="String")
		{
			if (output.type!="Flag" || def_type!="INFO")
			{
				printError(out, def_type+" definition cannot have a 'Type' entry of '" + output.type + "'!", l, line);
			}
		}
	}


	return output;
}

void VcfFile::checkValues(const DefinitionLine& def, const QByteArrayList& values, int alt_count, const QByteArray& sample, QTextStream& out, int l, const QByteArray& line)
{
	//check number of values
	int expected = -1;
	if (def.number=="A")
	{
		expected = alt_count;
	}
	else if (def.number=="R")
	{
		expected = alt_count + 1;
	}
	else if (def.number.toInt()>0)
	{
		expected = def.number.toInt();
	}
	else
	{
		//"G" and "." are not checked
	}
	if (expected!=-1 && expected!=values.count())
	{
		QByteArray where = sample.isEmpty() ? "INFO" : "sample '" + sample + " / annotation";
		printWarning(out, "For " + where + " '" + def.id + "' (number=" + def.number + "), the number of values is " + QByteArray::number(values.count()) + ", but should be " + QByteArray::number(expected) + "!", l, line);
	}

	//check value type
	foreach(const QByteArray& value, values)
	{
		bool value_valid = true;
		if (def.type=="Integer")
		{
			if (value!=".")
			{
				value.toInt(&value_valid);
			}
		}
		else if (def.type=="Float")
		{
			if (value!=".")
			{
				value.toFloat(&value_valid);
			}
		}
		else if (def.type=="Character")
		{
			value_valid = value.length()==1;
		}
		else if (def.type=="String")
		{
			//nothing to check
		}
		if (!value_valid)
		{
			QByteArray where = sample.isEmpty() ? "INFO" : "sample '" + sample + " / annotation";
			printWarning(out, "For " + where + " '" + def.id + "', the value '" + value + "' is not a '" + def.type + "'!", l, line);
		}
	}
}

//Returns the content of a column by index (tab-separated line)
QByteArray VcfFile::getPartByColumn(const QByteArray& line, int index)
{
	int columns_seen = 0;
	int column_start = 0;
	int column_end = -1;

	for (int i = 0; i < line.length(); ++i)
	{
		if (line[i] == '\t')
		{
			++columns_seen;
			if (columns_seen == index)
			{
				column_start = i + 1;
				column_end = line.length() - 1; // for last column that is not followed by a tab
			}
			else if (columns_seen == index + 1)
			{
				column_end = i;
				break;
			}
		}
	}

	if (column_end==-1)
	{
		THROW(ProgrammingException, "Cannot find column " + QByteArray::number(index) + " in line: " + line);
	}

	return line.mid(column_start, column_end - column_start);
}

//Define URL encoding
const QList<KeyValuePair> VcfFile::INFO_URL_MAPPING =
{
	KeyValuePair("%", "%25"), // has to be the first element to avoid replacement of already encoded characters
	KeyValuePair("\t", "%09"),
	KeyValuePair("\n", "%0A"),
	KeyValuePair("\r", "%0D"),
	KeyValuePair(" ", "%20"),
	KeyValuePair(",", "%2C"),
	KeyValuePair(";", "%3B"),
	KeyValuePair("=", "%3D")
};

//Returns string where all forbidden chars of an info column value are URL encoded
QString VcfFile::encodeInfoValue(QString info_value)
{
	// iterate over the mapping list and replace each character
	foreach(KeyValuePair replacement, INFO_URL_MAPPING)
	{
		info_value.replace(replacement.key, replacement.value);
	}
	return info_value;
}

//Returns string where all URL encoded chars of an info column value are decoded
QString VcfFile::decodeInfoValue(QString encoded_info_value)
{
	// iterate over the mapping list in reverse order and replace each encoded character
	for (int i=INFO_URL_MAPPING.size() - 1; i >= 0; i--)
	{
		encoded_info_value.replace(INFO_URL_MAPPING[i].value, INFO_URL_MAPPING[i].key);
	}
	return encoded_info_value;
}


