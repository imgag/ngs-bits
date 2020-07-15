#include "VcfFileHandler.h"

#include "Helper.h"
#include <zlib.h>

namespace VcfFormat
{

void VcfFileHandler::clear()
{
	vcf_lines_.clear();
	column_headers_.clear();
	vcf_header_.clear();
}

void VcfFileHandler::parseVcfHeader(const int line_number, QByteArray& line)
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
		else if(header_fields.count()==8)
		{
			column_headers_.push_back("FORMAT");
			column_headers_.push_back("Sample");
		}
	}
}
void VcfFileHandler::parseVcfEntry(const int line_number, QByteArray& line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids, ChromosomalIndex<BedFile>* roi_idx, bool invert)
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

	vcf_line.setId(line_parts[2].split(';'));
	vcf_line.setAlt(line_parts[4].split(','));

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
				new_info_line.id = key_value_pair[0];
				new_info_line.number = "1";
				new_info_line.type = "String";
				new_info_line.description = "no description available";
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
				new_format_line.id = format;
				new_format_line.number = "1";
				new_format_line.type = "String";
				new_format_line.description = "no description available";
				vcf_header_.addFormatLine(new_format_line);

				if(format == "GT")
				{
					vcf_header_.moveFormatLine(vcf_header_.formatLines().count()-1, 0);
				}
			}
			format_entries.push_back(strToPointer(format));
		}
		vcf_line.setFormat(format_entries);

		//SAMPLE
		OrderedHash<QByteArray, FormatIDToValueHash> sample_entries;
		if(line_parts.count() >= 10)
		{
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
		}
		else
		{
			//a FORMAT is given, however no SAMPLE data
			for(const QByteArray empty_format : vcf_line.format())
			{
				FormatIDToValueHash sample;
				sample.push_back(empty_format, "");
				//since SAMPLE is empty, there MUST be only one sampleID (this is set in parseHeaderFields)
				QByteArray sample_id = sampleIDs().at(0);
				sample_entries.push_back(strToPointer(sample_id), sample);
			}
		}
		vcf_line.setSample(sample_entries);
	}

	vcf_lines_.push_back(vcf_line);

}


void VcfFileHandler::processVcfLine(int& line_number, QByteArray line, QSet<QByteArray> info_ids, QSet<QByteArray> format_ids, ChromosomalIndex<BedFile>* roi_idx, bool invert)
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
		for(InfoFormatLine format : vcf_header_.formatLines())
		{
			format_ids.insert(format.id);
		}
		for(InfoFormatLine info : vcf_header_.infoLines())
		{
			info_ids.insert(info.id);
		}
		parseVcfEntry(line_number, line, info_ids, format_ids, roi_idx, invert);
	}
}

void VcfFileHandler::loadFromVCF(const QString& filename, ChromosomalIndex<BedFile>* roi_idx, bool invert)
{
	//clear content in case we load a second file
	clear();
	//parse from stream
	int line_number = 0;
	QSharedPointer<QFile> file = Helper::openFileForReading(filename, true);
	//Sets holding all INFO and FORMAT IDs defined in the header (might be extended if a vcf line contains new ones)
	QSet<QByteArray> info_ids_in_header;
	QSet<QByteArray> format_ids_in_header;
	while(!file->atEnd())
	{
		processVcfLine(line_number, file->readLine(), info_ids_in_header, format_ids_in_header, roi_idx, invert);
	}
}

void VcfFileHandler::loadFromVCFGZ(const QString& filename, ChromosomalIndex<BedFile>* roi_idx, bool invert)
{
	//clear content in case we load a second file
	clear();
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
		processVcfLine(line_number, QByteArray(read_line), info_ids_in_header, format_ids_in_header, roi_idx, invert);
	}
	gzclose(file);
	delete[] buffer;
}

void VcfFileHandler::load(const QString& filename, const BedFile* roi, bool invert)
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
		loadFromVCF(filename, roi_idx.data(), invert);
	}
	else if (fn_lower.endsWith(".vcf.gz"))
	{
		loadFromVCFGZ(filename, roi_idx.data(), invert);
	}
	else
	{
		THROW(ArgumentException, "Could not determine format of file '" + fn_lower + "' from file extension. Valid extensions are 'vcf' and 'vcf.gz'.");
	}
}

void VcfFileHandler::storeToTsv(const QString& filename) const
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
		stream << "##DESCRIPTION=" + info_line.id + "=" + info_line.description << "\n";
	}
	for(InfoFormatLine format_line : vcfHeader().formatLines())
	{
		if(format_line.id=="." || format_line.description=="") continue;
		stream << "##DESCRIPTION=" + format_line.id + "_ss=" + format_line.description << "\n";
	}

	//filter are added seperately
	for(const FilterLine& filter_line : vcfHeader().filterLines())
	{
		stream << "##FILTER=" << filter_line.id << "=" << filter_line.description << "\n";
	}

	//header
	stream << "#chr\tstart\tend\tref\tobs\tID\tQUAL\tFILTER";
	for(const InfoFormatLine& info_line : vcfHeader().infoLines())
	{
		if(info_line.id==".") continue;
		stream << "\t" << info_line.id;
	}
	int i = 0;
	while(i < sampleIDs().count())
	{
		for(const InfoFormatLine& format_line : vcfHeader().formatLines())
		{
			if(format_line.id==".") continue;
			stream << "\t" << format_line.id << "_ss";
		}
		++i;
	}

	//vcf lines
	for(const VCFLine& v : vcfLines())
	{
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
				stream << "\t" << v.sample(sample_id, format_key);
			}
		}
	}
}

void VcfFileHandler::storeToVcf(const QString& filename) const
{
	//open stream
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename);
	QTextStream stream(file.data());

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
}

void VcfFileHandler::store(const QString& filename, VariantListFormat format) const
{
	//determine format
	if (format==AUTO)
	{
		QString fn_lower = filename.toLower();
		if (fn_lower.endsWith(".vcf"))
		{
			format = VCF;
		}
		else if (fn_lower.endsWith(".tsv") || fn_lower.contains(".gsvar"))
		{
			format = TSV;
		}
		else
		{
			THROW(ArgumentException, "Could not determine format of file '" + filename + "' from file extension. Valid extensions are 'vcf', 'tsv' and 'GSvar'.")
		}
	}

	if (format==VCF)
	{
		storeToVcf(filename);
	}
	else
	{
		storeToTsv(filename);
	}
}

void VcfFileHandler::checkValid() const
{
	foreach(VCFLine vcf_line, vcf_lines_)
	{
		vcf_line.checkValid();

		//if FORMAT or SAMPLE exist, there must be more than MIN_COLS
		if( (!vcf_line.samples().empty() || !vcf_line.format().empty()) && column_headers_.count() == VCFHeader::MIN_COLS )
		{
			THROW(ArgumentException, "Invalid variant annotation data: Missing headers for VCF line with FORMAT and SAMPLES but only " + QString::number(VCFHeader::MIN_COLS) + " columns.");
		}
		//otherwise there can not be more samples than columns
		else if(column_headers_.count() > VCFHeader::MIN_COLS && vcf_line.samples().size() + 1 + VCFHeader::MIN_COLS > column_headers_.count())
		{
			THROW(ArgumentException, "Invalid variant annotation data: Wrong number of columns.");
		}
	}
}

void VcfFileHandler::sort(bool use_quality)
{
	if (vcfLines().count()==0) return;
	std::sort(vcf_lines_.begin(), vcf_lines_.end(), LessComparator(use_quality));

}
void VcfFileHandler::sortByFile(QString filename)
{
	sortCustom(LessComparatorByFile(filename));
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

const VCFLine& VcfFileHandler::addGSvarVariant(const Variant& var)
{
	//since we expect a variant frm GSvar we only set chr, start, ref and alt
	VCFLine vcf_line;
	vcf_line.setChromosome(var.chr());
	vcf_line.setPos(var.start());
	vcf_line.setRef(var.ref());
	QByteArrayList alt_list;
	alt_list.push_back(var.obs());
	vcf_line.setAlt(alt_list);

	vcf_lines_.push_back(vcf_line);
	return vcf_lines_.last();
}

void VcfFileHandler::storeLineInformation(QTextStream& stream, VCFLine line) const
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
	if(!line.filter().empty())
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
			FormatIDToValueHash sample_entry = line.sample(sample_idx);
			if(sample_entry.empty())
			{
				stream << "\t.";
			}
			else
			{
				stream << "\t" << sample_entry.at(0).value();
				//for all entries in the sample (e.g. 'GT':'DP':...)
				for(int sample_entry_id = 1; sample_entry_id < sample_entry.size(); ++sample_entry_id)
				{
					stream << ":" << sample_entry.at(sample_entry_id).value();
				}
			}
		}
	}
	else
	{
		stream << "\t.";
	}
}

QString VcfFileHandler::lineToString(int pos) const
{
	QString line;
	QTextStream stream(&line);
	storeLineInformation(stream, vcfLine(pos));
	return line;
}

} //end namespace VcfFormat
