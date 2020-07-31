#include "VcfFile.h"

#include "Helper.h"
#include <zlib.h>

void VcfFile::clear()
{
	vcf_lines_.clear();
	column_headers_.clear();
	vcf_header_.clear();
}

void VcfFile::parseVcfHeader(const int line_number, QByteArray& line)
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
	else if(line.startsWith("##INFO") || line.startsWith("##FORMAT"))
	{
		if (line.startsWith("##INFO"))
		{
			vcf_header_.setInfoFormatLine(line, INFO, line_number);
		}
		else
		{
			vcf_header_.setInfoFormatLine(line, FORMAT, line_number);
		}
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
void VcfFile::parseHeaderFields(QByteArray& line, bool allow_multi_sample)
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
		sample_id_to_idx = SampleIDToIdxPtr(new OrderedHash<QByteArray, unsigned char>);
		if(column_headers_.count() >= 10)
		{
			if( column_headers_.count() - 9 > UCHAR_MAX)
			{
				THROW(ArgumentException, "Number of sample entries exceeds the maximum of " + UCHAR_MAX);
			}
			for(int i = 9; i < column_headers_.count(); ++i)
			{
				sample_id_to_idx->push_back(column_headers_.at(i), static_cast<char>(i-9));
			}
		}
		else if(header_fields.count()==9) //if we have a FORMAT column with no sample
		{
			column_headers_.push_back("Sample");
			sample_id_to_idx->push_back("Sample", static_cast<char>(0));
		}
		else if(header_fields.count()==8)
		{
			column_headers_.push_back("FORMAT");
			column_headers_.push_back("Sample");
			sample_id_to_idx->push_back("Sample", static_cast<char>(0));
		}
	}
}
void VcfFile::parseVcfEntry(const int line_number, QByteArray& line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids, QSet<QByteArray> filter_ids, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert)
{

	QList<QByteArray> line_parts = line.split('\t');
	if (line_parts.count()<VCFHeader::MIN_COLS)
	{
		THROW(FileParseException, "VCF data line needs at least 8 tab-separated columns! Found " + QString::number(line_parts.count()) + " column(s) in line number " + QString::number(line_number) + ": " + line);
	}
	VCFLine vcf_line;
	vcf_line.setChromosome(strToPointer(line_parts[0]));
	if(!vcf_line.chr().isValid())
	{
		THROW(ArgumentException, "Invalid variant chromosome string in line " + QString::number(line_number) + ": " + vcf_line.chr().str() + ".");
	}
	vcf_line.setPos(atoi(line_parts[1]));
	if(vcf_line.pos() < 1)
	{
		THROW(ArgumentException, "Invalid variant position range in line " + QString::number(line_number) + ": " + QString::number(vcf_line.pos()) + ".");
	}
	vcf_line.setRef(strToPointer(line_parts[3].toUpper()));
	if (vcf_line.ref()!="-" && !QRegExp("[ACGTN]+").exactMatch(vcf_line.ref()))
	{
		THROW(ArgumentException, "Invalid variant reference sequence in line " + QString::number(line_number) + ": " + vcf_line.ref() + ".");
	}

	//Skip variants that are not in the target region (if given)
	if (roi_idx!=nullptr)
	{
		int end =  vcf_line.pos() +  vcf_line.ref().length() - 1;
		bool in_roi = roi_idx->matchingIndex(vcf_line.chr(), vcf_line.pos(), end) != -1;
		if ((!in_roi && !invert) || (in_roi && invert))
		{
			return;
		}
	}
	QByteArrayList id_list = line_parts[2].split(';');
	for(QByteArray& single_id : id_list)
	{
		single_id = strToPointer(single_id);
	}
	vcf_line.setId(id_list);
	vcf_line.addAlt(line_parts[4].split(','));
	for(Sequence alt_seq : vcf_line.alt())
	{
		if (alt_seq!="-" && alt_seq!="." && !QRegExp("[ACGTN,]+").exactMatch(alt_seq))
		{
			THROW(ArgumentException, "Invalid variant alternative sequence in line " + QString::number(line_number) + ": " + alt_seq + ".");
		}
	}

	if(line_parts[5]==".")
	{
		 vcf_line.setQual(-1);
	}
	else
	{
		bool quality_ok;
		double qual = line_parts[5].toDouble(&quality_ok);
		if (!quality_ok) THROW(ArgumentException, "Quality '" + line_parts[5] + "' is no float - variant.");
		vcf_line.setQual(qual);
	}

	//FILTER
	vcf_line.setFilter(line_parts[6].split(';'));
	for(const QByteArray& filter : vcf_line.filter())
	{
		if(filter == "PASS" || filter == "pass" || filter == "." || filter.isEmpty()) continue;
		if(!filter_ids.contains(filter))
		{
			FilterLine new_filter_line;
			new_filter_line.id = strToPointer(filter);
			new_filter_line.description = strToPointer("no description available");
			vcf_header_.addFilterLine(new_filter_line);
		}
	}

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
				new_info_line.id = strToPointer(key_value_pair[0]);
				new_info_line.number = strToPointer("1");
				new_info_line.type = strToPointer("String");
				new_info_line.description = strToPointer("no description available");
				vcf_header_.addInfoLine(new_info_line);
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
	if(line_parts.count() >= 9 && line_parts[8] != ".")
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
				new_format_line.id = strToPointer(format);
				new_format_line.number = strToPointer("1");
				new_format_line.type = strToPointer("String");
				new_format_line.description = strToPointer("no description available");
				vcf_header_.addFormatLine(new_format_line);

				if(format == "GT")
				{
					vcf_header_.moveFormatLine(vcf_header_.formatLines().count()-1, 0);
				}
			}
			format_entries.push_back(strToPointer(format));
		}
		vcf_line.setFormat(format_entries);

		//set format indices
		ListOfFormatIds format_ids_string = format_entries.join();
		if(format_id_to_idx_list.contains(format_ids_string))
		{
			vcf_line.setFormatIdToIdxPtr(format_id_to_idx_list[format_ids_string]);
		}
		else
		{
			FormatIDToIdxPtr new_format_id_to_idx_entry = FormatIDToIdxPtr(new OrderedHash<QByteArray, unsigned char>);
			for(int i = 0; i < format_entries.count(); ++i)
			{
				new_format_id_to_idx_entry->push_back(format_entries.at(i), static_cast<char>(i));
			}
			format_id_to_idx_list.insert(format_ids_string, new_format_id_to_idx_entry);
			vcf_line.setFormatIdToIdxPtr(new_format_id_to_idx_entry);
		}

		//set samples idices
		vcf_line.setSampleIdToIdxPtr(sample_id_to_idx);

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

				int format_entry_count = vcf_line.format().count();
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
					QByteArray value = sample_id_list.at(sample_id);
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
				vcf_line.addFormatValues(format_values_for_sample);
			}
		}
		else
		{

			//a FORMAT is given, however no SAMPLE data
			int format_count = 0;
			while(format_count < vcf_line.format().count())
			{
				QByteArrayList format_values_for_sample;
				format_values_for_sample.append(strToPointer(""));
				//since SAMPLE is empty, there MUST be only one sampleID (this is set in parseHeaderFields)
				vcf_line.addFormatValues(format_values_for_sample);

				++format_count;
			}
		}
	}

	vcf_lines_.push_back(vcf_line);

}


void VcfFile::processVcfLine(int& line_number, QByteArray line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids, QSet<QByteArray> filter_ids, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert)
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
		parseHeaderFields(line, allow_multi_sample);
	}
	else
	{
		for(InfoFormatLine format : vcf_header_.formatLines())
		{
			format_ids.insert(format.id);
		}
		for(InfoFormatLine info : vcf_header_.infoLines())
		{
			info_ids.insert(info.id);
		}
		for(FilterLine filter : vcf_header_.filterLines())
		{
			filter_ids.insert(filter.id);
		}
		parseVcfEntry(line_number, line, info_ids, format_ids, filter_ids, allow_multi_sample, roi_idx, invert);
	}
}

void VcfFile::loadFromVCF(const QString& filename, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert)
{
	//clear content in case we load a second file
	clear();
	//parse from stream
	int line_number = 0;
	QSharedPointer<QFile> file = Helper::openFileForReading(filename, true);

	//Sets holding all INFO and FORMAT IDs defined in the header (might be extended if a vcf line contains new ones)
	QSet<QByteArray> info_ids_in_header;
	QSet<QByteArray> format_ids_in_header;
	QSet<QByteArray> filter_ids_in_header;
	while(!file->atEnd())
	{
		processVcfLine(line_number, file->readLine(), info_ids_in_header, format_ids_in_header, filter_ids_in_header, allow_multi_sample, roi_idx, invert);
	}
}

void VcfFile::loadFromVCFGZ(const QString& filename, bool allow_multi_sample, ChromosomalIndex<BedFile>* roi_idx, bool invert)
{
	//clear content in case we load a second file
	clear();
	//parse from stream
	int line_number = 0;

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
		QSet<QByteArray> filter_ids_in_header;
		processVcfLine(line_number, QByteArray(read_line), info_ids_in_header, format_ids_in_header, filter_ids_in_header, allow_multi_sample, roi_idx, invert);
	}
	gzclose(file);
	delete[] buffer;
}

void VcfFile::load(const QString& filename, bool allow_multi_sample, const BedFile* roi, bool invert)
{
	//create ROI index (if given)
	QScopedPointer<ChromosomalIndex<BedFile>> roi_idx;
	if (roi!=nullptr)
	{
		if (!roi->isSorted())
		{
			THROW(ArgumentException, "Target region unsorted, but needs to be sorted (given for reading file " + filename + ")!");
		}
		roi_idx.reset(new ChromosomalIndex<BedFile>(*roi));
	}

	QString fn_lower = filename.toLower();
	if (fn_lower.endsWith(".vcf"))
	{
		loadFromVCF(filename, allow_multi_sample, roi_idx.data(), invert);
	}
	else if (fn_lower.endsWith(".vcf.gz"))
	{
		loadFromVCFGZ(filename, allow_multi_sample, roi_idx.data(), invert);
	}
	else
	{
		THROW(ArgumentException, "Could not determine format of file '" + fn_lower + "' from file extension. Valid extensions are 'vcf' and 'vcf.gz'.");
	}
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
			stream << "\t" << format_line.id << "_format_" << sample_id;
		}
	}

	//vcf lines
	for(VCFLine& v : vcfLines())
	{
		v.normalize("-", true);
		stream << "\n";
		stream << v.chr().str() << "\t" << QByteArray::number(v.start()) << "\t" << QByteArray::number(v.end()) << "\t" << v.ref()
			   << "\t" << v.altString() << "\t" << v.id().join(';') << "\t" << QByteArray::number(v.qual());
		if(v.filter().empty())
		{
			stream << "\t.";
		}
		else
		{
			stream << "\t" << v.filter().join(';');
		}

		for(const QByteArray& info_key : informationIDs())
		{
			stream << "\t" << v.info(info_key);
		}
		for(const QByteArray& sample_id : sampleIDs())
		{
			for(const QByteArray& format_key : formatIDs())
			{
				stream << "\t" << v.formatValueFromSample(format_key, sample_id);
			}
		}
	}
}

void VcfFile::store(const QString& filename,  bool stdout_if_file_empty, bool compress, int compression_level) const
{

	//open stream
	QString vcf_file;
	QTextStream stream(&vcf_file);

	//write header information
	vcf_header_.storeHeaderInformation(stream);

	//write header columns
	stream << "#" << column_headers_.at(0);
	for(int i = 1; i < column_headers_.count(); ++i)
	{
		stream << "\t" << column_headers_.at(i);
	}

	for(int i = 0; i < vcf_lines_.count(); ++i)
	{
		stream << "\n";
		storeLineInformation(stream, vcfLine(i));
	}

	if(compress)
	{
		gzFile gz_file = gzopen(filename.toLatin1().data(),"wb");
		if (gz_file == NULL)
		{
			THROW(FileAccessException, "Could not open file '" + filename + "' for writing!");
		}
		gzsetparams(gz_file, compression_level, Z_DEFAULT_STRATEGY);

		int written = gzputs(gz_file, vcf_file.toLocal8Bit().data());
		if (written==0)
		{
			THROW(FileAccessException, "Could not write to file '" + filename + "'!");
		}
		gzclose(gz_file);
	}
	else
	{
		//open stream
		QSharedPointer<QFile> file = Helper::openFileForWriting(filename, stdout_if_file_empty);
		QTextStream file_stream(file.data());

		file_stream << vcf_file;
	}
}

void VcfFile::leftNormalize(QString reference_genome)
{
	for(VCFLine& variant_line : vcfLines())
	{
		variant_line.leftNormalize(reference_genome);
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
	QVector<VCFLine> output;
	output.reserve(vcfLines().count());
	for (int i=0; i<vcfLines().count()-1; ++i)
	{
		int j = i+1;
		if (vcf_lines_.at(i).chr() != vcf_lines_.at(j).chr() || vcf_lines_.at(i).pos() != vcf_lines_.at(j).pos() || vcf_lines_.at(i).ref() !=vcf_lines_.at(j).ref() || !qEqual(vcf_lines_.at(i).alt().begin(),  vcf_lines_.at(i).alt().end(), vcf_lines_.at(j).alt().begin()))
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
	if(sample_id_to_idx->empty())
	{
		THROW(ArgumentException, "Sample IDS are not set.");
	}
	return sample_id_to_idx->keys();
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

AnalysisType VcfFile::type(bool allow_fallback_germline_single_sample) const
{
	return vcfHeader().type(allow_fallback_germline_single_sample);
}

void VcfFile::storeLineInformation(QTextStream& stream, VCFLine line) const
{
	//chr
	stream << line.chr().str()  << "\t" << line.pos();

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
		if(quality=="0") quality = "0.0";
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
	if(line.infos().empty())
	{
		stream << "\t.";
	}
	else
	{
		//if info is only TRUE, print key only
		QByteArray info_line_value = line.infos().at(0).value();
		QByteArray info_line_key = line.infos().at(0).key();
		if(info_line_value == "TRUE" && vcfHeader().infoLineByID(info_line_key, false).type == "Flag")
		{
			stream  << "\t"<< line.infos().at(0).key();
		}
		else
		{
			stream  << "\t"<< line.infos().at(0).key() << "=" << line.infos().at(0).value();;
		}
		if(line.infos().size() > 1)
		{
			for(int i = 1; i < line.infos().size(); ++i)
			{
				QByteArray info_line_value = line.infos().at(i).value();
				QByteArray info_line_key = line.infos().at(i).key();
				if(info_line_value == "TRUE" && vcfHeader().infoLineByID(info_line_key).type == "Flag")
				{
					stream  << ";"<< line.infos().at(i).key();
				}
				else
				{
					stream  << ";"<< line.infos().at(i).key() << "=" << line.infos().at(i).value();;
				}
			}
		}
	}

	//if format exists
	if(!line.format().empty())
	{
		stream  << "\t"<< line.format().at(0);
		for(int format_entry_id = 1; format_entry_id < line.format().count(); ++format_entry_id)
		{
			stream << ":" << line.format().at(format_entry_id);
		}
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
				//stream << "\t" << sample_entry.at(0).value();
				stream << "\t" << sample_entry.at(0);
				//for all entries in the sample (e.g. 'GT':'DP':...)
				for(int sample_entry_id = 1; sample_entry_id < sample_entry.size(); ++sample_entry_id)
				{
					//stream << ":" << sample_entry.at(sample_entry_id).value();
					stream << ":" << sample_entry.at(sample_entry_id);
				}
			}
		}
	}
	else
	{
		stream << "\t.";
	}
}

QString VcfFile::lineToString(int pos) const
{
	QString line;
	QTextStream stream(&line);
	storeLineInformation(stream, vcfLine(pos));
	return line;
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
		QByteArray format = "##fileformat=unavailable";
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
	SampleHeaderInfo genotype_columns = variant_list.getSampleHeader(false);
	if(genotype_columns.empty() || (genotype_columns.size() == 1 && genotype_columns.first().column_name == "genotype") )
	{
		vcf_file.column_headers_ << "Sample";
	}
	else
	{
		for(const SampleInfo& genotype : genotype_columns)
		{
			vcf_file.column_headers_ << genotype.column_name.toUtf8();
		}
	}

	//write genotype Format into header
	if(!genotype_columns.empty())
	{
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
	SampleIDToIdxPtr sample_id_to_idx = SampleIDToIdxPtr(new OrderedHash<QByteArray, unsigned char>);
	sample_id_to_idx = SampleIDToIdxPtr(new OrderedHash<QByteArray, unsigned char>);

	for(int i = 0; i < genotype_columns.size(); ++i)
	{
		const SampleInfo& genotype = genotype_columns.at(i);
		sample_id_to_idx->push_back(genotype.column_name.toUtf8(), i);
	}

	//format contains only GT
	QHash<ListOfFormatIds, FormatIDToIdxPtr> format_id_to_idx_list;
	ListOfFormatIds list_storing_genotype_only;
	list_storing_genotype_only.push_back("GT");
	FormatIDToIdxPtr gt_to_first_position = FormatIDToIdxPtr(new OrderedHash<QByteArray, unsigned char>);
	gt_to_first_position->push_back("GT", 1);
	format_id_to_idx_list.insert(list_storing_genotype_only, gt_to_first_position);

	//add variant lines
	for(int i = 0; i < variant_list.count(); ++i)
	{
		Variant v = variant_list[i];
		VCFLine vcf_line;

		QByteArrayList id_list;
		id_list.push_back(".");
		vcf_line.setId(id_list);
		if(qual_index >= 0)
		{
			vcf_line.setQual(v.annotations().at(qual_index).toDouble());
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
						vcf_line.setQual(qual);
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
			vcf_line.setFilter(v.annotations().at(filter_index).split(';'));
		}
		vcf_line.setChromosome(v.chr());
		vcf_line.setPos(v.start());
		vcf_line.setRef(v.ref());
		QByteArrayList alt_list;
		alt_list.push_back(v.obs());
		vcf_line.addAlt(alt_list);

		//add all columns into info
		OrderedHash<QByteArray , QByteArray> info;
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
					info.push_back(anno_header.name().toUtf8(), "TRUE");
				}
				else //everything else is just added to info
				{
					info.push_back(anno_header.name().toUtf8(), anno_val);
				}
			}
		}
		vcf_line.setInfo(info);

		//write genotype
		if(!genotype_columns.empty())
		{

			QByteArrayList format_list;
			format_list.push_back("GT");
			vcf_line.setFormat(format_list);

			QList<QByteArrayList> all_samples_new;

			for(const SampleInfo& genotype : genotype_columns)
			{
				QByteArrayList formats_new;

				int genotype_index = variant_list.annotationIndexByName(genotype.column_name);

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
				all_samples_new.push_back(formats_new);
			}
			vcf_line.setSampleNew(all_samples_new);
		}

		vcf_file.vcf_lines_.push_back(vcf_line);
	}

	for(VCFLine& v_line : vcf_file.vcfLines())
	{
		//add base for INSERTION
		QByteArray ref = v_line.ref().toUpper();
		if (ref.size() == 1 && !(ref=="N" || ref=="A" || ref=="C" || ref=="G" || ref=="T")) //empty seq symbol in ref
		{
			FastaFileIndex reference(reference_genome);
			QByteArray base = reference.seq(v_line.chr(), v_line.pos() - 1, 1);

			QByteArrayList alt_seq;
			//for GSvar there is only one alternative sequence (alt(0) stores VariantList.obs(0))
			QByteArray new_alt = base + v_line.alt(0);
			alt_seq.push_back(new_alt);
			v_line.setAlt(alt_seq);
			v_line.setRef(base);
		}

		//add base for DELETION
		QByteArray alt = v_line.alt(0).toUpper();
		if (alt.size() == 1 && !(alt=="N" || alt=="A" || alt=="C" || alt=="G" || alt=="T")) //empty seq symbol in alt
		{
			FastaFileIndex reference(reference_genome);
			QByteArray base = reference.seq(v_line.chr(), v_line.pos() - 1, 1);
			QByteArray new_ref = base + v_line.ref();
			v_line.setSingleAlt(base);
			v_line.setRef(new_ref);
			v_line.setPos(v_line.pos() - 1);
		}

	}
	vcf_file.leftNormalize(reference_genome);

	return vcf_file;

}
