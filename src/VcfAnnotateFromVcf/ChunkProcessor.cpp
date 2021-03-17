#include "ChunkProcessor.h"
#include "VcfFile.h"
#include "ToolBase.h"
#include <zlib.h>
#include <QFileInfo>
#include "Helper.h"
#include "VcfFile.h"
#include "VariantList.h"
#include <QMutex>

ChunkProcessor::ChunkProcessor(AnalysisJob &job,
                               QByteArrayList &prefix_list,
                               QSet<QByteArray> &ids,
                               const QVector<QByteArrayList> &info_id_list,
                               const QVector<QByteArrayList> &out_info_id_list,
                               const QByteArrayList &id_column_name_list,
                               const QByteArrayList &out_id_column_name_list,
                               const QVector<bool> &allow_missing_header_list,
							   QByteArrayList &annotation_file_list)

    :QRunnable()
    , terminate_(false)
    , job(job)
    , prefix_list(prefix_list)
    , ids(ids)
    , info_id_list(info_id_list)
    , out_info_id_list(out_info_id_list)
    , out_id_column_name_list(out_id_column_name_list)
    , id_column_name_list(id_column_name_list)
    , allow_missing_header_list(allow_missing_header_list)
	, annotation_file_list(annotation_file_list)
{
}


/*
 *  returns the value of a given INFO key from a given INFO header line
 */
QByteArray getInfoHeaderValue(const QByteArray &header_line, const QByteArray &key,
                              bool case_sensitive = false)
{
    // parse info id:
    QByteArrayList info_line_key_values = header_line.split('<')[1].split('>')[0].split(',');
    foreach (QByteArray key_value, info_line_key_values)
    {
        bool key_match = key_value.startsWith(key)
                || (!case_sensitive && key_value.toLower().startsWith(key.toLower()));
        if (key_match)
        {
            return key_value.split('=')[1].trimmed();
        }
    }
    THROW(FileParseException, "Key \"" + key + "\" not found in header line!");
    return "";
}

/*
 *  modifies the value of a given INFO key from a given INFO header line
 *   and returns the complete INFO header line
 *
 *	if extend == true the value will not be replaced but extended
 */
QByteArray modifyInfoHeaderValue(const QByteArray &header_line, const QByteArray &key,
                                 const QByteArray &new_value, bool case_sensitive = false,
                                 bool extend=false)
{
    // parse info id:
    QByteArrayList info_line_key_values = header_line.split('<')[1].split('>')[0].split(',');

    // rejoin segments which are inside double quotes:
    QByteArrayList joint_key_values;
    QByteArray tmp;
    foreach (const QByteArray& key_value, info_line_key_values)
    {
        // append to previously opened string
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
        else
        {
            // --> quotes not closed -> continue in next iteration
        }
    }



    // overwrite uncorrected key-value list
    info_line_key_values = joint_key_values;

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
                key_value[1] = key_value[1].trimmed();
                key_value[1] = key_value[1].split('\"')[1].split('\"')[0];
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
    THROW(FileParseException, "Key \"" + key + "\" not found in header line!");
    return "";
}

/*
 *	returns the header lines for the given info ids and stores the index of the given ID column
 */
QByteArrayList getVcfHeaderLines(const QByteArray &vcf_file_path, QByteArrayList info_ids,
                                 const QByteArray &id_column_name, int &id_column_idx,
                                 bool allow_missing_header)
{
    QByteArrayList info_header_lines;
    bool id_column_found = id_column_name == "";
    id_column_idx = -1;
    // check file type:
    if (vcf_file_path.toLower().endsWith(".vcf.gz"))
    {
        //read binary: always open in binary mode because windows and mac open in text mode
        gzFile vcfgz_file = gzopen(vcf_file_path, "rb");
        if (vcfgz_file==NULL)
        {
            THROW(FileAccessException, "Could not open file '" + vcf_file_path + "' for reading!");
        }

        char* buffer = new char[1048576]; //1MB buffer
        while(!gzeof(vcfgz_file))
        {

            char* char_array = gzgets(vcfgz_file, buffer, 1048576);

            //handle errors like truncated GZ file
            if (char_array==nullptr)
            {
                int error_no = Z_OK;
                QByteArray error_message = gzerror(vcfgz_file, &error_no);
                if (error_no!=Z_OK && error_no!=Z_STREAM_END)
                {
                    THROW(FileParseException, "Error while reading file '" + vcf_file_path
                          + "': " + error_message);
                }
            }

            QByteArray line = QByteArray(char_array);

            //skip empty lines
            if (line.trimmed().isEmpty()) continue;

            // abort if header is parsed
            if (!line.startsWith('#')) break;

            // parse info header
            if (line.startsWith("##INFO=<ID="))
            {
                // get info id value:
                QByteArray id_value = getInfoHeaderValue(line, "ID");
                if (info_ids.contains(id_value))
                {
                    // header line found
                    info_header_lines.append(line);
                    info_ids.removeAll(id_value);
                }
            }

            // parse column header line
            if (!id_column_found && line.startsWith("#CHROM"))
            {
                QByteArrayList header = line.remove(0,1).split('\t');
                id_column_idx = header.indexOf(id_column_name);
                if (id_column_idx == -1)
                {
                    THROW(FileParseException, "Id column \"" + id_column_name
                          + "\" not found in annotation file!");
                }
                id_column_found = true;

            }

            // abort if all info ids and the ID column have been found:
            if (id_column_found && (info_ids.size() == 0)) break;
        }
        gzclose(vcfgz_file);
        delete[] buffer;
    }
    else
    {
        // invalid/unknown file type:
        THROW(FileParseException, "File type of file \"" + vcf_file_path + "\" is invalid/unknown!");
    }

    if (info_ids.size() > 0)
    {
        if (allow_missing_header)
        {
            // create default header
            foreach (QByteArray info_id, info_ids) {
                info_header_lines.append("##INFO=<ID=" + info_id
                                         + ",Number=.,Type=String,Description=\"\">");
            }
        }
        else
        {
            THROW(FileParseException, "INFO id(s) \"" + info_ids.join(", ")
                  + "\" not found in VCF.GZ file \"" + vcf_file_path + "\"!");
        }
    }

    //append ID column header as last header line
    if (id_column_name != "" && id_column_idx != -1)
    {
        info_header_lines.append("##INFO=<ID=" + id_column_name
                                 + ",Number=.,Type=String,Description=\"ID column\">");
    }

    // extend header line by annotation file name:
    for (int i = 0; i < info_header_lines.size(); i++)
    {
        QByteArray additional_info = " (from file "
                + QFileInfo(vcf_file_path).fileName().toUtf8() + ")";
        info_header_lines[i] = modifyInfoHeaderValue(info_header_lines[i], "Description",
                                                     additional_info, false, true);
    }
    return info_header_lines;

}

/*
 *  extends a given vcf line by a key-value-pair of the given annotation vcf
 */
QByteArray extendVcfDataLine(const QByteArray &vcf_line,
                             const QVector<QByteArrayList> &info_ids,
                             const QVector<QByteArrayList> &out_info_ids,
                             const QByteArrayList &out_id_column_name_list,
                             const QVector<int> &id_column_indices,
                             QVector<TabixIndexedFile> &annotation_files)
{
    QTextStream out(stdout);

    int extended_lines_ = 0;

    //split line and extract variant infos
    QList<QByteArray> vcf_column = vcf_line.trimmed().split('\t');
    if (vcf_column.count()<VcfFile::MIN_COLS)
    {
        THROW(FileParseException, "VCF line with too few columns in input file: \n" + vcf_line);
    }

    // parse position
    Chromosome chr = vcf_column[VcfFile::CHROM];
    bool ok = false;
    int start = vcf_column[VcfFile::POS].toInt(&ok);
    if (!ok)
    {
        THROW(FileParseException, "Could not convert VCF variant position '"
              + vcf_column[1] + "' to integer!");
    }
    int end = start + vcf_column[VcfFile::REF].length() - 1; //length of ref

    // parse sequences
    QByteArray ref = vcf_column[VcfFile::REF];
    QByteArray obs = vcf_column[VcfFile::ALT];

    QByteArrayList additional_annotation;
    // iterate over all annotation files
    for (int ann_file_idx = 0; ann_file_idx < annotation_files.size(); ann_file_idx++)
    {
        // get all matching variants for this annotaion file
        QByteArrayList matches =
                annotation_files[ann_file_idx].getMatchingLines(chr, start, end, true);

        // collect the key-value pairs for all matches to prevent key duplications
        QByteArrayList additional_keys;
        QByteArrayList additional_values;
        QByteArrayList additional_ids;
        foreach(const QByteArray& match, matches)
        {

            // parse vcf line
            QByteArrayList parts = match.trimmed().split('\t');
            if (parts.count()<VcfFile::MIN_COLS)
            {
                THROW(FileParseException,
                      "VCF line with too few columns in annotation file: \n" + match);
            }

            // check if same variant
            if (parts[VcfFile::REF] != ref || parts[VcfFile::ALT] != obs) continue;
            bool ok;
            int pos = parts[VcfFile::POS].toInt(&ok);
            if (!ok)
            {
                THROW(FileParseException,
                      "VCF annotation file has invalid position in VCF line: \n" + match);
            }
            if (pos != start) continue;

            // add ID column from annotation file
            if (id_column_indices[ann_file_idx] > -1)
            {
                additional_ids.append(parts[id_column_indices[ann_file_idx]].trimmed());
            }


            // parse INFO column
            QByteArrayList info_column = parts[VcfFile::INFO].split(';');

            //get annotation
            for (int j = 0; j < info_ids[ann_file_idx].size(); j++)
            {
                foreach (QByteArray key_value_pair, info_column)
                {
                    QByteArrayList key_value_pair_split = key_value_pair.split('=');
                    if (key_value_pair_split[0].trimmed() == info_ids[ann_file_idx][j])
                    {
                        // handle boolean INFO entries (contain only key):
                        if (key_value_pair_split.size() == 1)
                        {
                            additional_annotation.append(out_info_ids[ann_file_idx][j]);
                        }
                        else
                        {
                            QByteArray annotation_value =
                                    key_value_pair.split('=')[1].trimmed();

                            // skip empty values:
                            if (annotation_value == "") continue;

                            int key_idx = additional_keys.indexOf(out_info_ids[ann_file_idx][j]);
                            if (key_idx == -1)
                            {
                                additional_keys.append(out_info_ids[ann_file_idx][j]);
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
            additional_annotation.append(out_id_column_name_list[ann_file_idx] + "="
                                         + additional_ids.join("&"));
        }

        for (int kv_idx=0; kv_idx<additional_keys.size(); kv_idx++)
        {
            additional_annotation.append(additional_keys[kv_idx] + "="
                                         + additional_values[kv_idx]);
        }
    }

    if (additional_annotation.size() > 0)
    {

        // extend info column
        vcf_column[VcfFile::INFO] += ";" + additional_annotation.join(';');
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

    QTextStream out(stdout);

    //out << "Start processing " << job.chunk_id << endl;

    //out << "Started CHUNKPROCESSOR!" << endl;

    // open all annotation files
    QVector<TabixIndexedFile> annotation_files(annotation_file_list.size());
    QVector<int> id_column_indices(annotation_file_list.size(), -1);
    QByteArrayList annotation_header_lines;
    for (int i = 0; i < annotation_file_list.size(); i++)
    {
        // out << "loading annotation file \"" << annotation_file_list[i] << "\"..." << endl;
        // get annotation header lines:
        QByteArrayList header_lines = getVcfHeaderLines(annotation_file_list[i],
                                                        info_id_list[i],
                                                        id_column_name_list[i],
                                                        id_column_indices[i],
                                                        allow_missing_header_list[i]);

        // replace input INFO ids with output INFO ids
        for (int j = 0; j < info_id_list[i].size(); j++)
        {
            if (info_id_list[i][j] != out_info_id_list[i][j])
            {
                header_lines[j].replace("##INFO=<ID=" + info_id_list[i][j],
                                        "##INFO=<ID=" + out_info_id_list[i][j]);
            }
        }

        // modify header line with id column
        if (header_lines.size() > info_id_list[i].size() && prefix_list[i] != "")
        {
            header_lines.back().replace("##INFO=<ID=" + id_column_name_list[i],
                                        "##INFO=<ID=" + prefix_list[i] + "_"
                                        + id_column_name_list[i]);
        }

        // append header lines to global list
        annotation_header_lines.append(header_lines);
        // load tab-indexed vcf file
        annotation_files[i].load(annotation_file_list[i]);
    }

    // read file
    for(const QByteArray& line : job.current_chunk)
    {
        // parse line:
        if (line.trimmed().isEmpty())
        {
            //skip empty lines
            continue;
        }

        else
        {
            // parse line
            if (line.startsWith('#'))
            {
                // check if new annotation name already exists in input file
                if (line.startsWith("##INFO=<"))
                {

                    // get INFO id value:
                    QByteArray id_value = getInfoHeaderValue(line, "ID");
                    if (ids.contains(id_value))
                    {
                        THROW(FileParseException, "Annotation \"" + id_value
                              + "\" already exists in input file");
                    }

                }

                //append header line for new annotation
                if (line.startsWith("#CHROM"))
                {
                    job.current_chunk_processed.append(annotation_header_lines);
                }

                job.current_chunk_processed.append(line);
                //out << "non chrom line" << line << endl;

            }
            else
            {
                // parse vcf data line
                job.current_chunk_processed.append(extendVcfDataLine(line,
                                                                     info_id_list,
                                                                     out_info_id_list,
                                                                     out_id_column_name_list,
                                                                     id_column_indices,
                                                                     annotation_files));
            }
        }
    }

    job.status=TO_BE_WRITTEN;

}
