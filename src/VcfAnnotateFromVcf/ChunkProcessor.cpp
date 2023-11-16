#include "ChunkProcessor.h"
#include "VcfFile.h"
#include "TabixIndexedFile.h"
#include <zlib.h>
#include <QFileInfo>

ChunkProcessor::ChunkProcessor(AnalysisJob& job, const MetaData& meta, Parameters& params)
	: QObject()
	, QRunnable()
	, job_(job)
	, meta_(meta)
	, params_(params)
{
	if (params_.debug) QTextStream(stdout) << "ChunkProcessor(): " << job_.index << endl;
}

ChunkProcessor::~ChunkProcessor()
{
	if (params_.debug) QTextStream(stdout) << "~ChunkProcessor(): " << job_.index << endl;
}


//returns the value of a given INFO key from a given INFO header line
QByteArray getInfoHeaderValue(const QByteArray &header_line, QByteArray key)
{
	if (!header_line.contains('<')) THROW(ArgumentException, "VCF INFO header contains no '<': " + header_line);

	key = key.toLower();

	QByteArrayList key_value_pairs = header_line.split('<')[1].split('>')[0].split(',');
	foreach (const QByteArray& key_value, key_value_pairs)
	{
		if (key_value.toLower().startsWith(key+'='))
		{
			return key_value.split('=')[1].trimmed();
		}
	}

	THROW(ArgumentException, "VCF INFO header contains no key '"+key+"': " + header_line);
}

//modifies the value of a given INFO key from a given INFO header line and returns the complete INFO header line
//if extend == true the value will not be replaced but extended
QByteArray modifyInfoHeaderValue(const QByteArray &header_line, const QByteArray &key, const QByteArray &new_value, bool case_sensitive = false, bool extend=false)
{
	// parse info id:
	QByteArrayList info_line_key_values = header_line.mid(header_line.indexOf('<') + 1, header_line.lastIndexOf('>') - header_line.indexOf('<')).split(',');

	// rejoin segments which are inside double quotes:
	QByteArrayList joint_key_values;
	QByteArray tmp;
	foreach (const QByteArray& key_value, info_line_key_values)
	{
		// append to previously opened string
		if (tmp.size() > 0) tmp.append(",");
		tmp.append(key_value);

		// check if quotes are closed:
		int n_quotes = tmp.count("\"");
		// remove escaped quotes:
		n_quotes -= tmp.count("\\\"");

		if(n_quotes % 2 == 0)
		{
			// --> quotes closed -> add complete key-value pair to list and clear string
			joint_key_values.append(tmp);
			tmp.clear();
		}
		// else --> quotes not closed -> continue in next iteration
	}
	if(!tmp.isEmpty()) THROW(ArgumentException, "Error parsing Info header: Quoted string was not closed!");

	// overwrite uncorrected key-value list
	info_line_key_values = joint_key_values;

	// modify header line
	for (int i = 0; i < info_line_key_values.size(); i++)
	{
		bool key_match = info_line_key_values[i].startsWith(key)
				|| (!case_sensitive &&
					info_line_key_values[i].toLower().startsWith(key.toLower()));
		if (key_match)
		{
			// match found:
			QByteArrayList key_value = info_line_key_values[i].split('=');
			// extend quotes of values to new value
			bool in_quotes = key_value[1].startsWith("\"");
			if (in_quotes)
			{
				// remove quotes
				key_value[1] = key_value[1].mid(1, key_value[1].lastIndexOf('\"') - 1 ).trimmed();
			}
			if (extend)
			{
				key_value[1] = key_value[1] + new_value;
			}
			else
			{
				key_value[1] =  new_value;
			}

			// concat key_value
			if (in_quotes)
			{
				info_line_key_values[i] = key_value[0] + "=\"" + key_value[1] + "\"";
			}
			else
			{
				info_line_key_values[i] = key_value[0] + "=" + key_value[1];
			}

			// concat header line
			return "##INFO=<" + info_line_key_values.join(',') + ">\n";
		}
	}
	THROW(ArgumentException, "Key \"" + key + "\" not found in header line!");
}

//returns the header lines for the given info ids and stores the index of the given ID column
QByteArrayList getVcfHeaderLines(const QByteArray &vcf_file_path, QByteArrayList info_ids, const QByteArray &id_column_name, int &id_column_idx, bool allow_missing_header)
{
	// check file type
	if (!vcf_file_path.toLower().endsWith(".vcf.gz")) THROW(ArgumentException, "File extension of input file '" + vcf_file_path + "' is not in VCF.GZ!");

	QByteArrayList info_header_lines;
	id_column_idx = id_column_name.isEmpty() ? -1 : 2;

	//read binary: always open in binary mode because windows and mac open in text mode
	gzFile vcfgz_file = gzopen(vcf_file_path, "rb");
	if (vcfgz_file==NULL) THROW(FileAccessException, "Could not open file '" + vcf_file_path + "' for reading!");

	const int buffer_size = 1048576; //1MB buffer
	char* buffer = new char[buffer_size];
	while(!gzeof(vcfgz_file))
	{

		char* char_array = gzgets(vcfgz_file, buffer, buffer_size);

		//handle errors like truncated GZ file
		if (char_array==nullptr)
		{
			int error_no = Z_OK;
			QByteArray error_string = gzerror(vcfgz_file, &error_no);
			if (error_no!=Z_OK && error_no!=Z_STREAM_END) THROW(FileAccessException, "Error while reading file '" + vcf_file_path + "': " + error_string);
		}

		QByteArray line = QByteArray(char_array);

		//skip empty lines
		if (line.trimmed().isEmpty()) continue;

		// abort if header is parsed completely
		if (!line.startsWith('#')) break;

		// parse info header
		if (line.startsWith("##INFO=<ID="))
		{
			// get info id value:
			QByteArray id_value = getInfoHeaderValue(line, "ID");
			if (info_ids.contains(id_value))
			{
				info_header_lines.append(line);
				info_ids.removeAll(id_value);
			}
		}
	}
	gzclose(vcfgz_file);
	delete[] buffer;

	if (info_ids.size() > 0)
	{
		if (allow_missing_header)
		{
			// create default header
			foreach (QByteArray info_id, info_ids)
			{
				info_header_lines.append("##INFO=<ID=" + info_id + ",Number=.,Type=String,Description=\"\">");
			}
		}
		else
		{
			THROW(FileParseException, "INFO key(s) \"" + info_ids.join(", ") + "\" not found in source file \"" + vcf_file_path + "\"!");
		}
	}

	//append ID column header as last header line
	if (id_column_name != "" && id_column_idx != -1)
	{
		info_header_lines.append("##INFO=<ID=" + id_column_name  + ",Number=.,Type=String,Description=\"ID column\">");
	}

	// extend header line by annotation file name:
	for (int i = 0; i < info_header_lines.size(); i++)
	{
		QByteArray additional_info = " (from file " + QFileInfo(vcf_file_path).fileName().toUtf8() + ")";
		info_header_lines[i] = modifyInfoHeaderValue(info_header_lines[i], "Description", additional_info, false, true);
	}

	return info_header_lines;
}

//extends a given vcf line by a key-value-pair of the given annotation vcf
QByteArray extendVcfDataLine(const QByteArray& vcf_line, const MetaData& meta, const QVector<int>& id_column_indices, const QVector<TabixIndexedFile>& annotation_files)
{
	int extended_lines_ = 0;

    //split line and extract variant infos
    QList<QByteArray> vcf_column = vcf_line.trimmed().split('\t');
	if (vcf_column.count()<VcfFile::MIN_COLS) THROW(FileParseException, "VCF line with too few columns in input file: " + vcf_line);

    // parse position
    Chromosome chr = vcf_column[VcfFile::CHROM];
    bool ok = false;
    int start = vcf_column[VcfFile::POS].toInt(&ok);
	if (!ok) THROW(FileParseException, "Could not convert VCF variant position '" + vcf_column[VcfFile::POS] + "' to integer in line: " + vcf_line);
    int end = start + vcf_column[VcfFile::REF].length() - 1; //length of ref

    // parse sequences
    QByteArray ref = vcf_column[VcfFile::REF];
    QByteArray obs = vcf_column[VcfFile::ALT];

    QByteArrayList additional_annotation;
    // iterate over all annotation files
    for (int ann_file_idx = 0; ann_file_idx < annotation_files.size(); ann_file_idx++)
    {
        // get all matching variants for this annotaion file
		QByteArrayList matches = annotation_files[ann_file_idx].getMatchingLines(chr, start, end, true);

        // collect the key-value pairs for all matches to prevent key duplications
        QByteArrayList additional_keys;
        QByteArrayList additional_values;
        QByteArrayList additional_ids;
        foreach(const QByteArray& match, matches)
        {

            // parse vcf line
            QByteArrayList parts = match.trimmed().split('\t');
			if (parts.count()<VcfFile::MIN_COLS) THROW(FileParseException, "VCF line with too few columns in annotation file: " + match);

            // check if same variant
            if (parts[VcfFile::REF] != ref || parts[VcfFile::ALT] != obs) continue;
            bool ok;
            int pos = parts[VcfFile::POS].toInt(&ok);
			if (!ok) THROW(FileParseException, "Could not convert VCF variant position '" + parts[VcfFile::POS] + "' to integer in annotation file line: " + match);
            if (pos != start) continue;

            // add ID column from annotation file
            if (id_column_indices[ann_file_idx] > -1)
            {
                additional_ids.append(parts[id_column_indices[ann_file_idx]].trimmed());
            }


            // parse INFO column
            QByteArrayList info_column = parts[VcfFile::INFO].split(';');

            //get annotation
			for (int j = 0; j < meta.info_id_list[ann_file_idx].size(); j++)
            {
                foreach (QByteArray key_value_pair, info_column)
                {
                    QByteArrayList key_value_pair_split = key_value_pair.split('=');
					if (key_value_pair_split[0].trimmed() == meta.info_id_list[ann_file_idx][j])
                    {
                        // handle boolean INFO entries (contain only key):
                        if (key_value_pair_split.size() == 1)
                        {
							additional_annotation.append(meta.out_info_id_list[ann_file_idx][j]);
                        }
                        else
                        {
							QByteArray annotation_value = key_value_pair.split('=')[1].trimmed();

							// skip empty values
							if (annotation_value == "") continue;

							int key_idx = additional_keys.indexOf(meta.out_info_id_list[ann_file_idx][j]);
							if (key_idx == -1)
							{
								additional_keys.append(meta.out_info_id_list[ann_file_idx][j]);
								additional_values.append(annotation_value);
							}
							else
							{
								additional_values[key_idx] += "&" + annotation_value;
							}
                        }
                        break;
                    }
                }
            }
        }

        // transfer the collected values into the INFO column
        if (additional_ids.size() > 0)
        {
			additional_annotation.append(meta.out_id_column_name_list[ann_file_idx] + "=" + additional_ids.join("&"));
        }

        for (int kv_idx=0; kv_idx<additional_keys.size(); kv_idx++)
        {
			additional_annotation.append(additional_keys[kv_idx] + "=" + additional_values[kv_idx]);
        }
    }

    if (additional_annotation.size() > 0)
    {

        // extend info column
		if (vcf_column[VcfFile::INFO]==".")
		{
			vcf_column[VcfFile::INFO] = additional_annotation.join(';');
		}
		else
		{
			vcf_column[VcfFile::INFO] += ";" + additional_annotation.join(';');
		}
        extended_lines_++;

		// concat vcf line:
        return vcf_column.join('\t') + "\n";
    }
    else
    {
        // if no annotation found write line without changes
        return vcf_line;
    }
}


// single chunks are processed
void ChunkProcessor::run()
{
	try
	{
		//load annotation indices - they are not thread-safe, so we need to load them in each thread :(
		QVector<TabixIndexedFile> annotation_files(meta_.annotation_file_list.size());
		QVector<int> id_column_indices(meta_.annotation_file_list.size(), -1);
		QByteArrayList annotation_header_lines;
		for (int i = 0; i < meta_.annotation_file_list.size(); i++)
		{
			// get annotation header lines:
			QByteArrayList header_lines = getVcfHeaderLines(meta_.annotation_file_list[i], meta_.info_id_list[i], meta_.id_column_name_list[i], id_column_indices[i], meta_.allow_missing_header_list[i]);

			// replace input INFO ids with output INFO ids
			for (int j = 0; j < meta_.info_id_list[i].size(); j++)
			{
				if (meta_.info_id_list[i][j] != meta_.out_info_id_list[i][j])
				{
					for (int h=0; h<header_lines.count(); h++) {
						QByteArray line_start = "##INFO=<ID=" + meta_.info_id_list[i][j];
						if (header_lines[h].startsWith(line_start))
						{
							header_lines[h].replace(line_start, "##INFO=<ID=" + meta_.out_info_id_list[i][j]);
						}
					}

				}
			}

			// modify header line with id column
			if (header_lines.size() > meta_.info_id_list[i].size() && meta_.prefix_list[i] != "")
			{
				header_lines.back().replace("##INFO=<ID=" + meta_.id_column_name_list[i], "##INFO=<ID=" + meta_.prefix_list[i] + "_" + meta_.id_column_name_list[i]);
			}

			// append header lines to global list
			annotation_header_lines.append(header_lines);
			// load tab-indexed vcf file
			annotation_files[i].load(meta_.annotation_file_list[i]);
		}

		//process data
		QList<QByteArray> lines_new;
		lines_new.reserve(job_.lines.size());
		foreach(const QByteArray& line, job_.lines)
		{
			if (line.trimmed().isEmpty())  continue;

			if (line.startsWith('#')) //header line
			{
				// check if new annotation name already exists in input file
				if (line.startsWith("##INFO=<"))
				{
					QByteArray id_value = getInfoHeaderValue(line, "ID");
					if (meta_.unique_output_ids.contains(id_value)) THROW(Exception, "INFO name '" + id_value + "' already exists in input file: " + line);
				}

				//append header line for new annotation
				if (line.startsWith("#CHROM"))
				{
					lines_new << annotation_header_lines;
				}

				lines_new << line;
			}
			else //content line
			{
				lines_new << extendVcfDataLine(line, meta_, id_column_indices, annotation_files);
			}
		}
		job_.lines = lines_new;

		emit done(job_.index);
	}
	catch(Exception& e)
	{
		emit error(job_.index, e.message());
	}
}
