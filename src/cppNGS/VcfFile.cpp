#include "VcfFile.h"
#include "Helper.h"
#include <QFileInfo>
#include <zlib.h>

VcfFile::VcfFile()
	: vcf_lines_()
	, vcf_header_()
	, sample_names_()
{
}

void VcfFile::clear()
{
	vcf_lines_.clear();
	vcf_header_.clear();
	sample_names_.clear();
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
		if (header_fields[0].trimmed()!="CHROM"||header_fields[1].trimmed()!="POS"||header_fields[2].trimmed()!="ID"||header_fields[3].trimmed()!="REF"||header_fields[4].trimmed()!="ALT"||header_fields[5].trimmed()!="QUAL"||header_fields[6].trimmed()!="FILTER"||header_fields[7].trimmed()!="INFO")
		{
			THROW(FileParseException, "VCF file header line with at least one inaccurately named mandatory column: '" + line.trimmed() + "'");
		}
		if(header_fields.count() >= 9 && header_fields[8].trimmed() != "FORMAT")
		{
			THROW(FileParseException, "VCF file header line with an inaccurately named FORMAT column: '" + line.trimmed() + "'");
		}
		if (header_fields.count() == 9)
		{
			THROW(FileParseException, "VCF file header line has only FORMAT column but no sample columns.");
		}

		//determine column and sample names
		int header_count = allow_multi_sample ? header_fields.count() : std::min(10, header_fields.count());
		for(int i = 9; i < header_count; ++i)
		{
			sample_names_ << header_fields.at(i);
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

	VcfLine vcf_line;
	vcf_line.setChromosome(strCache(line_parts[CHROM]));
	if(!vcf_line.chr().isValid())
	{
		THROW(ArgumentException, "Invalid variant chromosome string in line " + QString::number(line_number) + ": " + vcf_line.chr().str() + ".");
	}
	vcf_line.setPos(Helper::toInt(line_parts[POS], "VCF position"));
	if(vcf_line.start() < 0)
	{
		THROW(ArgumentException, "Invalid variant position range in line " + QString::number(line_number) + ": " + QString::number(vcf_line.start()) + ".");
	}
	vcf_line.setRef(strCache(line_parts[REF].toUpper()));

	//Skip variants that are not in the target region (if given)
	if (roi_idx!=nullptr)
	{
		int end =  vcf_line.start() +  vcf_line.ref().length() - 1;
		bool in_roi = roi_idx->matchingIndex(vcf_line.chr(), vcf_line.start(), end) != -1;
		if ((!in_roi && !invert) || (in_roi && invert))
		{
			return;
		}
	}

	QByteArrayList id_list = line_parts[ID].split(';');
	for (int i=0; i<id_list.count(); ++i)
	{
		id_list[i] = strCache(id_list[i]);
	}
	vcf_line.setId(strArrayCache(id_list));

	foreach(const QByteArray& alt, line_parts[ALT].split(','))
	{
		vcf_line.addAlt(strCache(alt.toUpper()));
	}
	if(line_parts[QUAL]==".")
	{
		vcf_line.setQual(-1);
	}
	else
	{
		bool quality_ok;
		double qual = line_parts[QUAL].toDouble(&quality_ok);
		if (!quality_ok) THROW(ArgumentException, "Quality '" + line_parts[QUAL] + "' is no float - variant.");
		vcf_line.setQual(qual);
	}

	//FILTER
	vcf_line.setFilters(line_parts[FILTER].split(';'));
	foreach(const QByteArray& filter, vcf_line.filters())
	{
		if(!filter_ids.contains(filter) && filter!="PASS" && filter!=".")
		{
			FilterLine new_filter_line;
			new_filter_line.id = strCache(filter);
			new_filter_line.description = strCache("no description available");
			vcf_header_.addFilterLine(new_filter_line);

			filter_ids.insert(strCache(filter));
		}
	}

	//INFO
	if(line_parts[INFO]!=".")
	{
		QByteArrayList info_keys;
		QByteArrayList info_values;

		foreach(const QByteArray& info, line_parts[INFO].split(';'))
		{
			int sep_index = info.indexOf('=');
			const QByteArray& key = strCache(sep_index==-1 ? info : info.left(sep_index));

			//check if the info is known in header
			if(!info_ids.contains(key))
			{
				InfoFormatLine new_info_line;
				new_info_line.id = key;
				new_info_line.number = strCache("1");
				new_info_line.type = strCache("String");
				new_info_line.description = strCache("no description available");
				vcf_header_.addInfoLine(new_info_line);

				info_ids.insert(key);
			}

			if(sep_index==-1) //Flag
			{
				info_values.push_back(strCache("TRUE"));
			}
			else //other types
			{
				info_values.push_back(strCache(info.mid(sep_index+1)));
			}
			info_keys.push_back(key);
		}

		vcf_line.setInfo(strArrayCache(info_keys), strArrayCache(info_values));
	}

	//FORMAT && SAMPLE
	if(line_parts.count() >= 9)
	{
		//parse format entries
		bool is_first = true;
		QByteArrayList format_list = line_parts[FORMAT].split(':');
		//QTextStream(stdout) << __FILE__ << " " << __LINE__ << " " << format_list.count() << " " << endl;
		foreach(const QByteArray& format, format_list)
		{
			//first entry must be GT if GT is present
			if(format == "GT" && !is_first)
			{
				THROW(FileParseException, "First Format entry is not a genotype ('GT') for line " + QString::number(line_number) + ": " + line);
			}
			is_first = false;

			if(!format_ids.contains(format) && format!=".")
			{
				InfoFormatLine new_format_line;
				new_format_line.id = format;
				new_format_line.number = strCache("1");
				new_format_line.type = strCache("String");
				new_format_line.description = strCache("no description available");
				vcf_header_.addFormatLine(new_format_line);

				if(format == "GT")
				{
					vcf_header_.moveFormatLine(vcf_header_.formatLines().count()-1, 0);
				}

				format_ids.insert(strCache(format));
			}
		}

		//set format indices
		for(int i = 0; i < format_list.count(); ++i)
		{
			format_list[i] = strCache(format_list[i]);
		}
		vcf_line.setFormatKeys(strArrayCache(format_list));

		//SAMPLE
		if(line_parts.count() >= 10)
		{
			int last_column_to_parse = allow_multi_sample ? line_parts.count() : 10;

			if(allow_multi_sample && sampleIDs().count() != line_parts.count() - 9)
			{
				THROW(FileParseException, "Number of samples in line (" + QString::number(line_parts.count() - 9) + ") not equal to number of samples in header (" + QString::number(sampleIDs().count()) + ")  in line " + QString::number(line_number) + ": " + line);
			}

			for(int i = 9; i < last_column_to_parse; ++i)
			{
				QByteArrayList sample_entries = line_parts[i].split(':');

				//SAMPLE columns can have missing trailing entries, but can not have more than specified in FORMAT
				if(sample_entries.count()!=vcf_line.formatKeys().count())
				{
					THROW(FileParseException, "Sample column has different number of entries than defined in Format column for line " + QString::number(line_number) + ": " + line);
				}

				//parse all available entries
				for(int i=0; i<sample_entries.count(); ++i)
				{
					sample_entries[i] = strCache(sample_entries[i]);
				}
				vcf_line.addFormatValues(strArrayCache(sample_entries));
				//QTextStream(stdout) << __FILE__ << " " << __LINE__ << " " << sample_entries.count() << " " << endl;
			}
		}
		else
		{
			THROW(FileParseException, "Format column but no sample columns present in line " + QString::number(line_number) + ": " + line);
		}
	}

	//set sample names
	vcf_line.setSamplNames(sample_names_);

	vcf_lines_.append(vcf_line);
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
		//all header lines should be read at this point
		foreach(const InfoFormatLine& format, vcf_header_.formatLines())
		{
			format_ids.insert(format.id);
		}
		foreach(const InfoFormatLine& info, vcf_header_.infoLines())
		{
			info_ids.insert(info.id);
		}
		foreach(const FilterLine& filter, vcf_header_.filterLines())
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

	int line_number = 0;
	//Sets holding all INFO and FORMAT IDs defined in the header (might be extended if a vcf line contains new ones)
	QSet<QByteArray> info_ids_in_header;
	QSet<QByteArray> format_ids_in_header;
	QSet<QByteArray> filter_ids_in_header;

	if (Helper::isHttpUrl(filename))
	{
		QSharedPointer<VersatileFile> file = Helper::openVersatileFileForReading(filename, true);
		while(!file->atEnd())
		{
			QByteArray line = file->readLine();
			processVcfLine(line_number, line, info_ids_in_header, format_ids_in_header, filter_ids_in_header, allow_multi_sample, roi_idx, invert);
		}
		file->close();
	}
	else
	{
		//reserve elements according to estimated experiment type
		QFileInfo info(filename);
		double mb = info.size()/1000000;
		QString extension = info.suffix().toUpper();
		bool is_wgs = (extension=="VCF" && mb>200) || (extension=="GZ" && mb>35);
		vcf_lines_.reserve(is_wgs ? 5000000 : 80000);

		const int buffer_size = 1048576; //1MB buffer
		char* buffer = new char[buffer_size];
		//open stream
		FILE* instream = filename.isEmpty() ? stdin : fopen(filename.toUtf8().data(), "rb");
		if (instream==nullptr) THROW(FileAccessException, "Could not open file '" + filename + "' for reading!");
		gzFile file = gzdopen(fileno(instream), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
		if (file==nullptr) THROW(FileAccessException, "Could not open file '" + filename + "' for reading!");

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
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename, true);
	QTextStream stream(file.data());

	foreach(const VcfHeaderLine& comment, vcfHeader().comments())
	{
		comment.storeLine(stream);
	}
	//write all DESCRIPTIONS
	//ID, QUAL and FILTER
	stream << "##DESCRIPTION=ID=ID of the variant, often dbSNP rsnumber\n";
	stream << "##DESCRIPTION=QUAL=Phred-scaled quality score\n";
	stream << "##DESCRIPTION=FILTER=Filter status\n";

	//in tsv format every info entry is a column, and every combination of format and sample
	foreach(const InfoFormatLine& info_line, vcfHeader().infoLines())
	{
		if(info_line.id=="." || info_line.description=="") continue;
		stream << "##DESCRIPTION=" + info_line.id + "_info=" + info_line.description << "\n";
	}
	foreach(const InfoFormatLine& format_line, vcfHeader().formatLines())
	{
		if(format_line.id=="." || format_line.description=="") continue;
		stream << "##DESCRIPTION=" + format_line.id + "_format=" + format_line.description << "\n";
	}

	//filter are added seperately
	foreach(const FilterLine& filter_line, vcfHeader().filterLines())
	{
		stream << "##FILTER=" << filter_line.id << "=" << filter_line.description << "\n";
	}

	//header
	stream << "#chr\tpos\tref\talt\tID\tQUAL\tFILTER";
	//one column for every INFO field
	foreach(const InfoFormatLine& info_line, vcfHeader().infoLines())
	{
		if(info_line.id==".") continue;
		stream << "\t" << info_line.id << "_info";
	}
	//one column for every combination of a FORMAT field and a SAMPLE
	foreach(const QByteArray& sample_id, sampleIDs())
	{
		foreach(const InfoFormatLine& format_line, vcfHeader().formatLines())
		{
			if(format_line.id==".") continue;
			stream << "\t" << sample_id << "_" << format_line.id << "_format";
		}
	}
	stream << "\n";

	//vcf lines
	foreach(const VcfLine& v, vcf_lines_)
	{
		//normalize variants and set symbol for empty sequence
		stream << v.chr().str() << "\t" << QByteArray::number(v.start()) << "\t" << v.ref() << "\t" << v.altString() << "\t" << v.id().join(';') << "\t" << QByteArray::number(v.qual());
		if(v.filters().isEmpty())
		{
			stream << "\t.";
		}
		else
		{
			stream << "\t" << v.filters().join(';');
		}

		foreach(const InfoFormatLine& line, vcfHeader().infoLines())
		{
			stream << "\t" << v.info(line.id);
		}
		foreach(const QByteArray& sample_id, sampleIDs())
		{
			foreach(const InfoFormatLine& line, vcf_header_.formatLines())
			{
				stream << "\t" << v.formatValueFromSample(line.id, sample_id);
			}
		}
		stream << "\n";
	}
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
		foreach (const VcfLine& line, vcf_lines_)
		{
			storeLineInformation(file_stream, line);
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
		BGZF* instream = bgzf_open(filename.toUtf8().data(), compression_mode);
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

		foreach (const VcfLine& line, vcf_lines_)
		{
			storeLineInformation(stream, line);
		}

		writeBGZipped(instream, vcf_file);

		bgzf_close(instream);
	}

}

void VcfFile::leftNormalize(QString reference_genome)
{
	FastaFileIndex reference(reference_genome);

	for(int i=0; i<vcf_lines_.count(); ++i)
	{
		vcf_lines_[i].leftNormalize(reference);
	}
}

void VcfFile::rightNormalize(QString reference_genome)
{
	FastaFileIndex reference(reference_genome);

	for(int i=0; i<vcf_lines_.count(); ++i)
	{
		vcf_lines_[i].rightNormalize(reference);
	}
}

void VcfFile::sort(bool use_quality)
{
	if (vcf_lines_.count()==0) return;
	std::sort(vcf_lines_.begin(), vcf_lines_.end(), LessComparator(use_quality));

}

void VcfFile::sortByFile(QString fai_file)
{
	sortCustom(LessComparatorByFile(fai_file));
}

void VcfFile::removeDuplicates(bool sort_by_quality)
{
	sort(sort_by_quality);

	//remove duplicates (same chr, start, obs, ref) - avoid linear time remove() calls by copying the data to a new vector.
	QList<VcfLine> output;
	output.reserve(vcf_lines_.count());
	for (int i=0; i<vcf_lines_.count()-1; ++i)
	{
		int j = i+1;
		if (vcf_lines_[i].chr() != vcf_lines_[j].chr() || vcf_lines_[i].start() != vcf_lines_[j].start() || vcf_lines_[i].ref() !=vcf_lines_[j].ref() || !qEqual(vcf_lines_[i].alt().begin(),  vcf_lines_[i].alt().end(), vcf_lines_[j].alt().begin()))
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

void VcfFile::storeLineInformation(QTextStream& stream, const VcfLine& line) const
{
	//chr
	stream << line.chr().str()  << '\t' << line.start();

	//if id exists
	stream  << '\t' << (line.id().empty() ? "." : line.id().join(';'));

	//ref and alt
	stream << '\t'<< line.ref();
	stream << '\t' << line.alt()[0];
	for(int i = 1; i < line.alt().size(); ++i)
	{
		stream  << "," <<  line.alt().at(i);
	}
	//quality
	stream  << '\t' << (line.qual()==-1 ? "." : QByteArray::number(line.qual()));

	//filters
	stream  << '\t' << (line.filters().isEmpty() ? "." : line.filters().join(';'));

	//info
	stream << '\t';
	const QByteArrayList& info_keys = line.infoKeys();
	if(info_keys.isEmpty())
	{
		stream << '.';
	}
	else
	{
		bool is_first_entry = true;
		foreach (const QByteArray& key, info_keys)
		{
			if (!is_first_entry) stream  << ';';
			is_first_entry = false;
			const QByteArray& value = line.info(key, true);
			if(value == "TRUE" && vcfHeader().infoLineByID(key).type == "Flag")
			{
				stream << key;
			}
			else
			{
				stream << key << '=' << value;
			}
		}
	}

	//format  and samples
	if (!sample_names_.isEmpty())
	{
		stream << '\t' << line.formatKeys().join(':');

		foreach(const QByteArrayList& sample_entry, line.samples())
		{
			stream << '\t' << (sample_entry.empty() ? "." : sample_entry.join(':'));
		}
	}

	stream << '\n';
}

void VcfFile::storeHeaderColumns(QTextStream &stream) const
{
	stream << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO";

	if (!sample_names_.isEmpty())
	{
		stream << "\tFORMAT";
		foreach(const QByteArray& sample, sample_names_)
		{
			stream << '\t' << sample;
		}
	}

	stream << '\n';
}

void VcfFile::copyMetaData(const VcfFile& rhs)
{
	vcf_header_ = rhs.vcfHeader();
	sample_names_ = rhs.sample_names_;
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

	foreach (const VcfLine& line, vcf_lines_)
	{
		storeLineInformation(stream, line);
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

VcfFile VcfFile::fromGSvar(const VariantList& variant_list, const QString& reference_genome)
{
	FastaFileIndex reference(reference_genome);

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

	//search for genotype on annotations
	SampleHeaderInfo sample_headers;
	try
	{
		sample_headers = variant_list.getSampleHeader();
	}
	catch(...){} //nothing to do here

	//write genotype Format into header
	if(!sample_headers.empty())
	{
		InfoFormatLine format_line;
		format_line.id = "GT";
		format_line.number = "1";
		format_line.type = "String";
		format_line.description = "Genotype";
		vcf_file.vcf_header_.addFormatLine(format_line);
	}

	//determine indices to skip
	int qual_index = variant_list.annotationIndexByName("QUAL", true, false);
	int filter_index = variant_list.annotationIndexByName("FILTER", true, false);
	QSet<int> indices_to_skip;
	indices_to_skip.insert(qual_index);
	indices_to_skip.insert(filter_index);
	foreach(const SampleInfo& sample_header, sample_headers)
	{
		indices_to_skip << sample_header.column_index;
	}

	//determine sample names
	QByteArrayList sample_names;
	foreach(const SampleInfo& sample_header, sample_headers)
	{
		sample_names << sample_header.name.toLatin1();
	}
	vcf_file.setSampleNames(sample_names);

	//format keys (only GT)
	QByteArrayList format_keys;
	format_keys << "GT";

	//add variant lines
	for(int i = 0; i < variant_list.count(); ++i)
	{
		Variant v = variant_list[i];
		VcfLine vcf_line;
		vcf_line.setFormatKeys(format_keys);
		vcf_line.setSamplNames(sample_names);

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
				foreach(const QByteArray& element, quality_list)
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
			vcf_line.setFilters(v.annotations().at(filter_index).split(';'));
		}

		VcfLine tmp = v.toVCF(reference);
		vcf_line.setChromosome(tmp.chr());
		vcf_line.setPos(tmp.start());
		vcf_line.setRef(tmp.ref());
		vcf_line.setSingleAlt(tmp.altString());

		//add all columns into info
		QByteArrayList info;
		QByteArrayList all_info_keys;

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
					info.push_back(strCache("TRUE"));
				}
				else //everything else is just added to info
				{
					info.push_back(strCache(encodeInfoValue(anno_val).toUtf8()));
				}

				all_info_keys.push_back(anno_header.name().toUtf8());
			}
		}

		//set info
		vcf_line.setInfo(all_info_keys, info);

		//write genotype
		foreach(const SampleInfo& sample_header, sample_headers)
		{
			int genotype_index = variant_list.annotationIndexByName(sample_header.name, true, false);
			if(genotype_index == -1)
			{
				//qDebug() << __LINE__ << sample_header.column_name;
				vcf_line.addFormatValues(QByteArrayList() << ".");
				continue;
			}

			QByteArrayList formats_new;
			QByteArray gt = v.annotations().at(genotype_index).trimmed();
			if(gt.isEmpty() || gt == "." || gt == "n/a")
			{
				formats_new.push_back("./.");
			}
			else if(gt == "wt")
			{
				formats_new.push_back("0/0");
			}
			else if(gt == "hom")
			{
				formats_new.push_back("1/1");
			}
			else if(gt == "het")
			{
				formats_new.push_back("1/0");
			}
			else
			{
				THROW(ArgumentException, "Genotype column in GSvar file contains invalid entry '" + gt + "'.");
			}
			vcf_line.addFormatValues(formats_new);
		}

		vcf_file.vcf_lines_.push_back(vcf_line);
	}

	//left-normalize all variants
	vcf_file.leftNormalize(reference_genome);

	return vcf_file;
}

bool VcfFile::isValid(QString filename, QString ref_file, QTextStream& out_stream, bool print_general_information, int max_lines)
{
	//open input file
	FILE* instream = filename.isEmpty() ? stdin : fopen(filename.toUtf8().data(), "rb");
	if (instream==nullptr) THROW(FileAccessException, "Could not open file '" + filename + "' for reading!");
	gzFile file = gzdopen(fileno(instream), "rb"); //always open in binary mode because windows and mac open in text mode
	if (file==nullptr) THROW(FileAccessException, "Could not open file '" + filename + "' for reading!");
	
	//open reference genome
	FastaFileIndex reference(ref_file);

	//load MISO terms
	OntologyTermCollection obo_terms("://Resources/so-xp_3_1_0.obo", true);

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
	invalid_chars << ' '; //space is not valid

	//perform checks
	QMap<QByteArray, DefinitionLine> defined_filters;
	QMap<QByteArray, DefinitionLine> defined_formats;
	QMap<QByteArray, DefinitionLine> defined_infos;
	QByteArrayList defined_samples;
	int expected_parts = MIN_COLS;
	bool in_header = true;
	bool vcf_main_header_found = false;
	bool error_found = false;
	int c_data = 0;
	int l = 0;
	const int buffer_size = 1048576; //1MB buffer
	char* buffer = new char[buffer_size];
	while(!gzeof(file) && c_data<max_lines)
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

		//header lines
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
				vcf_main_header_found = true;

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
			//check that main header line is before first variant
			if (!vcf_main_header_found)
			{
				printError(out_stream, "Main header line missing!", l, line);
				return false;
			}
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
				if (ref.isEmpty())
				{
					printError(out_stream, "Reference base(s) not set!", l, line);
					error_found = true;
				}
				else
				{
					Sequence ref_exp = reference.seq(chr, pos, ref.length());
					if (ref!=ref_exp)
					{
						printError(out_stream, "Reference base(s) not correct. Is '" + ref + "', should be '" + ref_exp + "'!", l, line);
						error_found = true;
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
						error_found = true;
					}
				}
			}

			//check that the first base is the same for insertions, deletions and complex insertions/deletions
			foreach(const QByteArray& alt, alts)
			{
				if (alt.startsWith('<') && alt.endsWith('>')) continue; //special case for structural variant
				if (alt=="*") continue; //special case for missing allele due to downstream deletion
				if (alt.isEmpty()) continue; //warning already above
				if ((alt.length()>1 || ref.length()>1) && alt.length()!=ref.length())
				{
					if (alt[0]!=ref[0])
					{
						printError(out_stream, "First base of insertion/deletion not matching - ref: '" + ref + "' alt: '" + alt + "'!", l, line);
						error_found = true;
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
							printError(out_stream, "Value of INFO entry '" + name + "' has a value which contains the invalid character '" + invalid_char + "' (value: '" + value + "')!", l, line);
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

	return !error_found;
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

const QByteArray& VcfFile::strCache(const QByteArray& str)
{
	static QSet<QByteArray> cache;

	QSet<QByteArray>::iterator it = cache.find(str);
	if (it==cache.end())
	{
		it = cache.insert(str);
	}

	return *it;
}

const QByteArrayList& VcfFile::strArrayCache(const QByteArrayList& str)
{
	static QSet<QByteArrayList> cache;

	QSet<QByteArrayList>::iterator it = cache.find(str);
	if (it==cache.end())
	{
		it = cache.insert(str);
	}

	return *it;
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

void VcfFile::removeUnusedContigHeaders()
{
	//determine used chromosomes
	QSet<QByteArray> chromosomes;
	foreach(const VcfLine& line, vcf_lines_)
	{
		chromosomes << line.chr().str();
	}

	//remove unused contig headers
	for (int i=vcf_header_.comments().count()-1; i>=0; --i)
	{
		const VcfHeaderLine& line = vcf_header_.comments()[i];
		if (line.key!="contig") continue;

		QByteArray tmp = line.value.mid(line.value.indexOf('=')+1);
		QByteArray chr = tmp.left(tmp.indexOf(','));

		if (!chromosomes.contains(chr))
		{
			vcf_header_.removeCommentLine(i);
		}
	}
}



VcfFile::LessComparator::LessComparator(bool use_quality)
	: use_quality(use_quality)
{
}

bool VcfFile::LessComparator::operator()(const VcfLine& a, const VcfLine& b) const
{
	if (a.chr()<b.chr()) return true;//compare chromsomes
	else if (a.chr()>b.chr()) return false;
	else if (a.start()<b.start()) return true;//compare start positions
	else if (a.start()>b.start()) return false;
	else if (a.ref().length()<b.ref().length()) return true;//compare end positions by comparing length of ref
	else if (a.ref().length()>b.ref().length()) return false;
	else if (a.ref()<b.ref()) return true;//compare reference seqs
	else if (a.ref()>b.ref()) return false;
	else if (a.alt(0)<b.alt(0)) return true;//compare alternative seqs
	else if (a.alt(0)>b.alt(0)) return false;
	else if (use_quality)
	{
		double q_a=a.qual();
		double q_b=b.qual();
		if(q_a<q_b) return true;
	}
	return false;
}

VcfFile::LessComparatorByFile::LessComparatorByFile(QString fai_file)
	: filename_(fai_file)
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

bool VcfFile::LessComparatorByFile::operator()(const VcfLine& a, const VcfLine& b) const
{
	int a_chr_num = a.chr().num();
	int b_chr_num = b.chr().num();
	if (!chrom_rank_.contains(a_chr_num))
	{
		THROW(FileParseException, "Reference file for sorting does not contain chromosome '" + a.chr().str() + "'!");
	}
	if (!chrom_rank_.contains(b_chr_num))
	{
		THROW(FileParseException, "Reference file for sorting does not contain chromosome '" + b.chr().str() + "'!");
	}

	if (chrom_rank_[a_chr_num]<chrom_rank_[b_chr_num]) return true; //compare rank of chromosome
	else if (chrom_rank_[a_chr_num]>chrom_rank_[b_chr_num]) return false;
	else if (a.start()<b.start()) return true; //compare start position
	else if (a.start()>b.start()) return false;
	else if (a.ref().length()<b.ref().length()) return true; //compare end position
	else if (a.ref().length()>b.ref().length()) return false;
	else if (a.ref()<b.ref()) return true; //compare ref sequence
	else if (a.ref()>b.ref()) return false;
	else if (a.alt(0)<b.alt(0)) return true; //compare obs sequence
	else if (a.alt(0)>b.alt(0)) return false;
	return false;
}
