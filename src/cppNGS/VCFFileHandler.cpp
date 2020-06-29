#include "VCFFileHandler.h"

#include "Helper.h"
#include <zlib.h>

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
	else if(line.startsWith("##fileDate"))
	{
		VcfHeader_.setDate(line);
	}
	else if(line.startsWith("##source"))
	{
		VcfHeader_.setSource(line);
	}
	else if(line.startsWith("##reference"))
	{
		VcfHeader_.setReference(line);
	}
	else if(line.startsWith("##contig"))
	{
		VcfHeader_.setContig(line);
	}
	else if(line.startsWith("##phasing"))
	{
		VcfHeader_.setPhasing(line);
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

/*
		//parse description field
		QByteArray description_entry=comma_splitted_line.front();
		QList <QByteArray> splitted_description_entry=description_entry.split('=');
		if (splitted_description_entry[0].trimmed()!="Description")
		{
			THROW(FileParseException, "Malformed "+info_or_format +" line: fourth field is not a description field " + line.trimmed() + "'");
		}
		//ugly, but because the description may content commas, too...
		comma_splitted_line.pop_front();//pop type-field
		comma_splitted_line.push_front(splitted_description_entry[1]);//re-add description value between '=' and possible ","
		QStringList description_value_parts;//convert to QStringList
		for(int i=0; i<comma_splitted_line.size(); ++i)
		{
			description_value_parts.append(comma_splitted_line[i]);
		}
		QString description_value=description_value_parts.join(",");//join parts
		description_value=description_value.mid(1);//remove '"'
		description_value.chop(2);//remove '">'
		new_annotation_description.setDescription(description_value);

		//check if annotation description is a possible duplicate
		bool found = false;
		foreach(const VariantAnnotationDescription& vad, annotationDescriptions())
		{
			if(vad.name()==new_annotation_description.name() && vad.sampleSpecific()==new_annotation_description.sampleSpecific())
			{
				Log::warn("Duplicate metadata information for field named '" + new_annotation_description.name() + "'. Skipping metadata line " + QString::number(line_number) + ".");
				found = true;
				break;
			}
		}
		if(found) return;

		annotationDescriptions().append(new_annotation_description);

		//make sure the "GT" format field is always the first format field
		if (new_annotation_description.name()=="GT" && new_annotation_description.sampleSpecific())
		{
			int first_format_index = -1;
			for(int i=0; i<annotationDescriptions().count(); ++i)
			{
				if (!annotationDescriptions()[i].sampleSpecific()) continue; //skip INFO description

				first_format_index = i;
				break;
			}

			if (first_format_index<annotationDescriptions().count()-1)
			{
				annotationDescriptions().move(annotationDescriptions().count()-1, first_format_index);
			}
		}

		return;
	}
	//filter lines
	else if (line.startsWith("##FILTER=<ID="))
	{
		QStringList parts = QString(line.mid(13, line.length()-15)).split(",Description=\"");
		if(parts.count()!=2) THROW(FileParseException, "Malformed FILTER line: conains more/less than two parts: " + line);
		filters_[parts[0]] = parts[1];
		return;
	}
	//other meta-information lines
	else
	{
		addCommentLine(line);
		return;
	}*/
}

void VcfFileHandler::processVcfLine(int& line_number, QByteArray line)
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

/*
	//header line
	if (line.startsWith("#CHROM"))
	{
		header_fields = line.mid(1).split('\t');

		if (header_fields.count()<VcfFile::MIN_COLS)//8 are mandatory
		{
			THROW(FileParseException, "VCF file header line with less than 8 fields found: '" + line.trimmed() + "'");
		}
		if ((header_fields[0]!="CHROM")||(header_fields[1]!="POS")||(header_fields[2]!="ID")||(header_fields[3]!="REF")||(header_fields[4]!="ALT")||(header_fields[5]!="QUAL")||(header_fields[6]!="FILTER")||(header_fields[7]!="INFO"))
		{
			THROW(FileParseException, "VCF file header line with at least one illegal named mandatory column: '" + line.trimmed() + "'");
		}

		// set annotation headers
		annotations().append(VariantAnnotationHeader("ID"));
		annotations().append(VariantAnnotationHeader("QUAL"));
		annotations().append(VariantAnnotationHeader("FILTER"));
		// (1) for all INFO fields (sample independent annotations)
		for(int i=0; i<annotationDescriptions().count(); ++i)
		{
			if(annotationDescriptions()[i].name()=="ID" || annotationDescriptions()[i].name()=="QUAL" || annotationDescriptions()[i].name()=="FILTER")	continue;	//skip annotations that are already there
			if(annotationDescriptions()[i].sampleSpecific())	continue;
			annotations().append(VariantAnnotationHeader(annotationDescriptions()[i].name()));
		}
		// (2) for all samples and their FORMAT fields (sample dependent annotations)
		for(int i=9; i<header_fields.count(); ++i)
		{
			QString sample_id = QString(header_fields[i]);
			int sample_specific_count = 0;

			for(int ii=0; ii<annotationDescriptions().count(); ++ii)
			{
				if(!annotationDescriptions()[ii].sampleSpecific()) continue;
				++sample_specific_count;
				annotations().append(VariantAnnotationHeader(annotationDescriptions()[ii].name(),sample_id));
			}

			if(sample_specific_count==0)
			{
				annotations().append(VariantAnnotationHeader(".",sample_id));
				annotationDescriptions().append(VariantAnnotationDescription(".", "Default column description since no FORMAT fields were defined.", VariantAnnotationDescription::STRING, true, "1", false));//add dummy description
			}
		}

		// (3) FORMAT column available
		if(header_fields.count()<=9)
		{
			QString sample_id = "Sample";
			int sample_specific_count = 0;
			for(int i=0; i<annotationDescriptions().count(); ++i)
			{
				if(!annotationDescriptions()[i].sampleSpecific()) continue;
				annotations().append(VariantAnnotationHeader(annotationDescriptions()[i].name(),sample_id));
				++sample_specific_count;
			}

			if(sample_specific_count==0)
			{
				annotations().append(VariantAnnotationHeader(".", sample_id));
				annotationDescriptions().append(VariantAnnotationDescription(".", "Default column description since no FORMAT fields were defined.", VariantAnnotationDescription::STRING, true, "1", false));//add dummy description
			}
		}
		return;
	}

	//variant line
	QList<QByteArray> line_parts = line.split('\t');
	if (line_parts.count()<VcfFile::MIN_COLS)
	{
		THROW(FileParseException, "VCF data line needs at least 7 tab-separated columns! Found " + QString::number(line_parts.count()) + " column(s) in line number " + QString::number(line_number) + ": " + line);
	}

	//Skip variants that are not in the target region (if given)
	Chromosome chr = line_parts[0];
	int start = atoi(line_parts[1]);
	Sequence ref_bases = line_parts[3].toUpper();
	int end = start + ref_bases.length()-1;
	if (roi_idx!=nullptr)
	{
		bool in_roi = roi_idx->matchingIndex(chr, start, end)!=-1;
		if ((!in_roi && !invert) || (in_roi && invert))
		{
			return;
		}
	}

	//extract sample-independent annotations
	QList <QByteArray> annos;
	annos << line_parts[2] << line_parts[5] << line_parts[6]; //id, quality, filter
	for(int i=3; i<annotations().count();++i)
	{
		annos.append(QByteArray());
	}

	if ((line_parts.count()>=8)&&(line_parts[7]!="."))
	{
		QList<QByteArray> anno_parts = line_parts[7].split(';');
		for (int i=0; i<anno_parts.count(); ++i)
		{
			QList<QByteArray> key_value = anno_parts[i].split('=');

			QByteArray value;
			if (key_value.count()==1) //no value (flag)
			{
				value = "TRUE";
			}
			else
			{
				value = key_value[1];
				value = (value=="." ? "" : value);
			}

			int index = annotations().indexOf(VariantAnnotationHeader(key_value[0]));
			if(index==-1)
			{
				annotations().append(VariantAnnotationHeader(key_value[0]));
				annotationDescriptions().append(VariantAnnotationDescription(key_value[0], "no description available"));
				//Log::info("No metadata information for INFO field " + key_value[0] + " was found.");

				for(int ii=0;ii<variants_.count();++ii)
				{
					variants_[ii].annotations().append(QByteArray());
				}

				index = annos.count();
				annos.append(value);
			}
			annos[index] = value;
		}
	}

	//extract sample-dependent annotations
	if (line_parts.count()>=10)//if present: extract sample dependent annotations
	{
		QList<QByteArray> names = line_parts[8].split(':');
		for(int i=9; i<header_fields.count(); ++i)
		{
			QString sample_id = QString(header_fields[i]);
			if(sample_id.isEmpty() && header_fields.count()==10)	sample_id = "Sample";

			QList<QByteArray> values = line_parts[i].split(':');
			for (int ii=0; ii<names.count(); ++ii)
			{
				QByteArray value = "";
				if (values[ii]!=".") value = values[ii];

				int index = annotations().indexOf(VariantAnnotationHeader(names[ii],sample_id));
				if(index==-1 && names[ii]==".")
				{
					THROW(FileParseException, "Invalid empty FORMAT field of sample '"+sample_id+"'!");
				}
				if(index==-1)
				{
					//Log::info("No metadata information for FORMAT field " + names[ii] + ".");
					annotations().append(VariantAnnotationHeader(names[ii],sample_id));
					annotationDescriptions().append(VariantAnnotationDescription(names[ii],"no description available",VariantAnnotationDescription::STRING,true));
					for(int iii=0;iii<variants_.count();++iii)
					{
						variants_[iii].annotations().append(QByteArray());
					}

					index = annos.count();
					annos.append(value);
				}
				annos[index] = value;
			}
		}
	}

	append(Variant(chr, start, end, ref_bases, line_parts[4].toUpper(), annos, 2));*/
}

void VcfFileHandler::loadFromVCF(const QString& filename)
{
	//parse from stream
	int line_number = 0;
	QSharedPointer<QFile> file = Helper::openFileForReading(filename, true);
	while(!file->atEnd())
	{
		processVcfLine(line_number, file->readLine());
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

		processVcfLine(line_number, QByteArray(read_line));
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
	//TODO: make a container that keeps information in order
	VcfHeader_.storeHeaderInformation(stream);
/*
	//write annotations information (##INFO and ##FORMAT lines)
	for (int j=3; j<annotationDescriptions().count(); ++j) //why 3: skip ID Quality Filter
	{
		const VariantAnnotationDescription& anno_description = annotationDescriptions()[j];
		if(!anno_description.print())
		{
			continue;
		}

		stream << "##" << (anno_description.sampleSpecific() ? "FORMAT" : "INFO") << "=";
		stream << "<ID=" << anno_description.name();
		stream << ",Number=" << anno_description.number();
		stream << ",Type=" << annotationTypeToString(anno_description.type());
		QString desc = anno_description.description();
		stream << ",Description=\"" << (desc!="" ? desc : "no description available") << "\"";
		stream << ">\n";
	}

	//write filter headers
	auto it = filters().cbegin();
	while(it != filters().cend())
	{
		stream << "##FILTER=<ID=" << it.key() << ",Description=\"" << it.value() << "\">\n";
		++it;
	}

	//write header line
	stream << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT";
	QStringList samples = sampleNames();
	foreach(const QString& sample, samples)
	{
		stream << "\t" << sample;
	}
	stream << "\n";

	//write variants
	foreach(const Variant& v, variants_)
	{
		QString ID = v.annotations()[0];//will only work correctly if source was a  VCF file
		QString quality = v.annotations()[1];//will only work correctly if source was a VCF file
		QString filter = v.annotations()[2];//will only work correctly if source was a VCF file
		QStringList info_entries;
		QStringList format_entries;
		QHash <QString, QStringList> sample_entries_by_sample;
		QString sample;

		for (int i=3; i<v.annotations().count(); ++i) //why 3: skip ID Quality Filter
		{
			const VariantAnnotationHeader& anno_header = annotations()[i];
			const VariantAnnotationDescription& anno_desc = annotationDescriptionByName(anno_header.name(), !anno_header.sampleID().isEmpty());
			QByteArray anno_val = v.annotations()[i];

			if (anno_desc.sampleSpecific())
			{
				if (anno_val!="" || samples.count()>1)
				{
					if (sample.isEmpty()) sample = anno_header.sampleID();
					if (sample==anno_header.sampleID())
					{
						format_entries << anno_desc.name();
					}

					if (anno_val=="") anno_val = ".";
					sample_entries_by_sample[anno_header.sampleID()].append(anno_val);
				}
			}
			else
			{
				if (anno_val!="")
				{
					if (anno_desc.type()==VariantAnnotationDescription::FLAG) //Flags should not have values in VCF
					{
						info_entries << anno_desc.name();
					}
					else
					{
						info_entries << anno_desc.name() + "=" + anno_val;
					}
				}
			}
		}

		stream << v.chr().str() << "\t" << v.start() << "\t" << ID << "\t" << v.ref() << "\t"  << v.obs() << "\t" << quality << "\t" << filter;
		stream << "\t" << (info_entries.isEmpty() ? "." : info_entries.join(";"));
		stream << "\t" << (format_entries.isEmpty() ? "." : format_entries.join(":"));
		foreach(const QString& sample, samples)
		{
			const QStringList& sample_entries = sample_entries_by_sample[sample];
			stream << "\t" << (sample_entries.isEmpty() ? "." : sample_entries.join(":"));
		}
		stream << "\n";
	}
*/
}

