#include "VcfFile.h"
#include "OntologyTermCollection.h"
#include "Chromosome.h"
#include "Sequence.h"
#include "FastaFileIndex.h"
#include "Helper.h"
#include "NGSHelper.h"
#include <QList>
#include <zlib.h>

bool VcfFile::isValid(QString vcf_file_path, QString ref_file, QTextStream& out_stream, bool print_general_information, int max_lines)
{
	// determine file type:
	VariantListFormat format;
	if (vcf_file_path.toLower().endsWith(".vcf") || vcf_file_path == "")
	{
		// plain vcf
		format = VariantListFormat::VCF;
	}
	else if (vcf_file_path.toLower().endsWith(".vcf.gz") || vcf_file_path.toLower().endsWith(".bgz"))
	{
		//zipped vcf
		format = VariantListFormat::VCF_GZ;
	}
	else
	{
		// invalid/unknown file type
		THROW(FileParseException, "File type of file \"" + vcf_file_path  + "\" is invalid/unknown!");
	}


	// open input file:
	QSharedPointer<QFile> in_p;
	gzFile input_vcf_gz = gzFile();
	char* buffer = new char[1048576];//1MB buffer
	bool eof = true;
	if (format == VariantListFormat::VCF)
	{
		in_p = Helper::openFileForReading(vcf_file_path, true);
		// check for eof:
		eof = in_p->atEnd();
	}
	else
	{
		//read binary: always open in binary mode because windows and mac open in text mode
		input_vcf_gz = gzopen(vcf_file_path.toUtf8(), "rb");
		if (input_vcf_gz==NULL)
		{
			THROW(FileAccessException, "Could not open file '" + vcf_file_path + "' for reading!");
		}
		// check for eof:
		eof = gzeof(input_vcf_gz);
	}

	//open reference genome
	FastaFileIndex reference(ref_file);

	//load MISO terms
	OntologyTermCollection obo_terms("://Resources/so-xp_3_0_0.obo", true);

	//ALT allele regexp
	QRegExp alt_regexp("[ACGTN]+");

	//create list of all invalid chars in INFO column values
	QList<char> invalid_chars;
	foreach(KeyValuePair kvp, VcfFile::INFO_URL_MAPPING)
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
	int expected_parts = 8;
	bool in_header = true;
	int c_data = 0;
	int l = 1;
	while(!eof && l<max_lines)
	{
		// get next line
		QByteArray line;
		if (format == VariantListFormat::VCF)
		{
			line = in_p -> readLine().trimmed();

			// check for eof:
			eof = in_p->atEnd();
		}
		else
		{
			char* char_array = gzgets(input_vcf_gz, buffer, 1048576);

			//handle errors like truncated GZ file
			if (char_array==nullptr)
			{
				int error_no = Z_OK;
				QByteArray error_message = gzerror(input_vcf_gz, &error_no);
				if (error_no!=Z_OK && error_no!=Z_STREAM_END)
				{
					THROW(FileParseException, "Error while reading file '" + vcf_file_path + "': " + error_message);
				}
			}
			line = QByteArray(char_array).trimmed();

			// check for eof:
			eof = gzeof(input_vcf_gz);
		}



		//skip empty lines
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

				if (parts.count()<VcfFile::MIN_COLS)
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
            Chromosome chr(parts[VcfFile::CHROM]);
			if (chr.str().contains(":"))
			{
                printError(out_stream, "Chromosome '" + parts[VcfFile::CHROM] + "' is not valid!", l, line);
				return false;
			}

			//position
			bool pos_is_valid;
            int pos = parts[VcfFile::POS].toInt(&pos_is_valid);
			if(!pos_is_valid)
			{
                printError(out_stream, "Chromosomal position '" + parts[VcfFile::POS] + "' is not a number!", l, line);
				return false;
			}

			//reference base
            QByteArray ref = parts[VcfFile::REF].toUpper();
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
			QByteArrayList alts = parts[VcfFile::ALT].split(',');
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
            const QByteArray& qual = parts[VcfFile::QUAL];
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
            const QByteArray& filter = parts[VcfFile::FILTER];
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
            QByteArrayList info = parts[VcfFile::INFO].split(';');
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
							printError(out_stream, "VEP-based CSQ annoation has " + QByteArray::number(csq_parts.count()) + " entries, expected " + QByteArray::number(csq_defs.count()) + " according to definition in header!", l, line);
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

			//format
			if (parts.count()==8) continue;
            QByteArrayList format_names = parts[VcfFile::FORMAT].split(':');
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

		++l;
	}


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

void VcfFile::checkValues(const VcfFile::DefinitionLine& def, const QByteArrayList& values, int alt_count, const QByteArray& sample, QTextStream& out, int l, const QByteArray& line)
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
	foreach(KeyValuePair replacement, VcfFile::INFO_URL_MAPPING)
	{
		info_value.replace(replacement.key, replacement.value);
	}
	return info_value;
}

//Returns string where all URL encoded chars of an info column value are decoded
QString VcfFile::decodeInfoValue(QString encoded_info_value)
{
	// iterate over the mapping list in reverse order and replace each encoded character
	for (int i=VcfFile::INFO_URL_MAPPING.size() - 1; i >= 0; i--)
	{
		encoded_info_value.replace(VcfFile::INFO_URL_MAPPING[i].value, VcfFile::INFO_URL_MAPPING[i].key);
	}
	return encoded_info_value;
}

