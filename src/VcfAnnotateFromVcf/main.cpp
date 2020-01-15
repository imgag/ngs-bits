#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "TabixIndexedFile.h"
#include "VcfFile.h"
#include "VariantList.h"
#include <QFile>
#include <QSharedPointer>
#include <zlib.h>
#include <QElapsedTimer>
#include <QFileInfo>

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Annotates the INFO column of a VCF with data from another VCF file (or multiple VCF files if config file is provided).");

		//optional
		addInfile("config_file", "TSV file containing the annotation file path, the prefix, the INFO ids and the id column for multiple annotations.", true);
		addInfile("annotation_file", "Tabix indexed VCF.GZ file used for annotation.", true, true);
		addString("info_ids", "INFO id(s) in annotation VCF file (Multiple ids can be separated by ',', optional new id names in output file can be added by '=': original_id=new_id).", true, "");
		addString("id_column", "Name of the ID column in annotation file. (If "" it will be ignored in output file, alternative output name can be specified by old_id_column_name=new_name", true, "");
		addString("id_prefix", "Prefix for INFO id(s) in output VCF file.", true, "");
		addFlag("allow_missing_header", "If set the execution is not aborted if a INFO header is missing in annotation file");
		addInfile("in", "Input VCF(.GZ) file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);

		changeLog(2019, 8, 19, "Added support for multiple annotations files through config file.");
		changeLog(2019, 8, 14, "Added VCF.GZ support.");
		changeLog(2019, 8, 13, "Initial implementation.");


	}

	virtual void main()
	{
		//init
		QTextStream out(stdout);
		QElapsedTimer timer;
		timer.start();

		// parse parameter
		QString input_path = getInfile("in");
		QString output_path = getOutfile("out");
		QString config_file_path = getInfile("config_file");
		QString annotation_file_path = getInfile("annotation_file");
		QByteArray info_id_string = getString("info_ids").toLatin1().trimmed();
		QByteArray id_column = getString("id_column").toLatin1().trimmed();
		QByteArray id_prefix = getString("id_prefix").toLatin1().trimmed();
		bool allow_missing_header = getFlag("allow_missing_header");

		QByteArrayList annotation_file_list;
		QByteArrayList prefix_list;
		QVector<QByteArrayList> info_id_list;
		QVector<QByteArrayList> out_info_id_list;
		QByteArrayList id_column_name_list;
		QByteArrayList out_id_column_name_list;
		QVector<bool> allow_missing_header_list;

		// check for config file
		if (config_file_path != "")
		{
			// parse config file line by line
			QSharedPointer<QFile> config_file = Helper::openFileForReading(config_file_path, false);
			while (!config_file -> atEnd())
			{
				QByteArray line = config_file -> readLine();
				// skip empty or comment lines
				if ((line.trimmed() == "") || (line.startsWith('#'))) continue;
				QByteArrayList columns = line.split('\t');
				if (columns.size() < 4)
				{
					THROW(FileParseException, QByteArray() + "Invalid number of columns! "
						  + "File name, prefix, INFO ids and id column name are required:\n"
						  + line.replace("\t", " -> ").trimmed());
				}

				annotation_file_list.append(columns[0].trimmed());

				QByteArray prefix = columns[1];
				QByteArrayList info_ids;
				QByteArrayList out_info_ids;
				parseInfoIds(info_ids, out_info_ids, columns[2], prefix);

				prefix_list.append(prefix);
				info_id_list.append(info_ids);
				out_info_id_list.append(out_info_ids);

				QByteArray id_column_name;
				QByteArray out_id_column_name;
				parseIdColumn(id_column_name, out_id_column_name, columns[3], prefix);

				id_column_name_list.append(id_column_name);
				out_id_column_name_list.append(out_id_column_name);

				if (columns.size() > 4)
				{
					allow_missing_header_list.append((columns[4].trimmed().toLower() == "true")
							|| (columns[4].trimmed() == "1"));
				}
				else
				{
					allow_missing_header_list.append(false);
				}
			}

			if (annotation_file_list.size() < 1)
			{
				THROW(FileParseException,
					  "The config file has to contain at least 1 valid annotation configuration!");
			}
		}
		else
		{
			// use parameter

			annotation_file_list.append(annotation_file_path.toLatin1().trimmed());

			if (info_id_string.trimmed() == "")
			{
				THROW(ArgumentException,
					  "The \"info_id\" parameter is required if no config file is provided");
			}
			QByteArrayList info_ids;
			QByteArrayList out_info_ids;
			parseInfoIds(info_ids, out_info_ids, info_id_string, id_prefix);

			prefix_list.append(id_prefix);
			info_id_list.append(info_ids);
			out_info_id_list.append(out_info_ids);

			QByteArray id_column_name;
			QByteArray out_id_column_name;
			parseIdColumn(id_column_name, out_id_column_name, id_column, id_prefix);

			id_column_name_list.append(id_column_name);
			out_id_column_name_list.append(out_id_column_name);

			allow_missing_header_list.append(allow_missing_header);
		}



		// write arguments:
		out << "Input file: \t" << input_path << "\n";
		out << "Output file: \t" << output_path << "\n";
		for(int i = 0; i < annotation_file_list.size(); i++)
		{
			out << "Annotation file: " << annotation_file_list[i] << "\n";

			if (id_column_name_list[i] != "")
			{
				out << "Id column:\n\t " << id_column_name_list[i].leftJustified(12) << "\t -> \t"
					<< out_id_column_name_list[i] << "\n";
			}
			out << "INFO ids:\n";
			for (int j = 0; j < info_id_list[i].size(); j++)
			{
				out << "\t " << info_id_list[i][j].leftJustified(12) << "->   "
					<< out_info_id_list[i][j] << "\n";
			}
		}

		out.flush();

		// open all annotation files
		QVector<TabixIndexedFile> annotation_files(annotation_file_list.size());
		QVector<int> id_column_indices(annotation_file_list.size(), -1);
		QByteArrayList annotation_header_lines;
		for (int i = 0; i < annotation_file_list.size(); i++)
		{
			out << "loading annotation file \"" << annotation_file_list[i] << "\"..." << endl;
			// get annotation header lines:
			QByteArrayList header_lines = getVcfHeaderLines(annotation_file_list[i],
															info_id_list[i], id_column_name_list[i],
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

		// check info ids for duplicates:
		QByteArrayList tmp;
		foreach (QByteArrayList ids, out_info_id_list)
		{
			tmp.append(ids);
		}
		QSet<QByteArray> unique_output_ids = QSet<QByteArray>::fromList(tmp);
		if (unique_output_ids.size() < tmp.size())
		{
			//Duplicates found!
			THROW(FileParseException, "The given output INFO ids contain duplicates!")
		}


		//open input/output streams
		if (input_path!="" && input_path==output_path)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		QSharedPointer<QFile> output_vcf = Helper::openFileForWriting(output_path, true);

		// determine file type:
		VariantListFormat format;
		if (input_path.toLower().endsWith(".vcf") || input_path == "")
		{
			// plain vcf
			format = VariantListFormat::VCF;
		}
		else if (input_path.toLower().endsWith(".vcf.gz"))
		{
			//zipped vcf
			format = VariantListFormat::VCF_GZ;
		}
		else
		{
			// invalid/unknown file type
			THROW(FileParseException, "File type of file \"" + input_path
				  + "\" is invalid/unknown!");
		}

		// parsing input file
		out << "parsing input vcf file..." << endl;
		int vcf_line_idx = 0;

		// open input file:
		QSharedPointer<QFile> input_vcf;
		gzFile input_vcf_gz = gzFile();
		char* buffer = new char[1048576];//1MB buffer
		bool eof = true;
		if (format == VariantListFormat::VCF)
		{
			input_vcf = Helper::openFileForReading(input_path, true);
			// check for eof:
			eof = input_vcf->atEnd();
		}
		else
		{
			//read binary: always open in binary mode because windows and mac open in text mode
			input_vcf_gz = gzopen(input_path.toUtf8(), "rb");
			if (input_vcf_gz==NULL)
			{
				THROW(FileAccessException, "Could not open file '" + input_path + "' for reading!");
			}
			// check for eof:
			eof = gzeof(input_vcf_gz);

		}

		// iterate over the vcf file line by line
		while(!eof)
		{
			// get next line
			QByteArray line;
			if (format == VariantListFormat::VCF)
			{
				line = input_vcf -> readLine();

				// check for eof:
				eof = input_vcf->atEnd();
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
						THROW(FileParseException, "Error while reading file '" + input_path
							  + "': " + error_message);
					}
				}
				line = QByteArray(char_array);

				// check for eof:
				eof = gzeof(input_vcf_gz);
			}
			vcf_line_idx++;

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
						if (unique_output_ids.contains(id_value))
						{
							THROW(FileParseException, "Annotation \"" + id_value
								  + "\" already exists in input file");
						}

					}

					//append header line for new annotation
					if (line.startsWith("#CHROM"))
					{
						output_vcf -> write(annotation_header_lines.join());
					}
					output_vcf -> write(line);
				}
				else
				{
					// parse vcf data line
					output_vcf -> write(extendVcfDataLine(line, info_id_list, out_info_id_list,
														  out_id_column_name_list,
														  id_column_indices,
														  annotation_files));
				}
			}

			// print stats
			if(vcf_line_idx%10000 == 0)
			{
				out << "\t" << vcf_line_idx << " vcf lines parsed" << endl;
			}
		}

		// close files
		if (format == VariantListFormat::VCF)
		{
			input_vcf -> close();
		}
		else
		{
			gzclose(input_vcf_gz);
			delete[] buffer;
		}
		// close output file
		output_vcf -> close();

		out << "\nExecution finished\n" << vcf_line_idx << " vcf lines parsed, " << extended_lines_
			<< " lines annotated. (runtime: " << getTimeString(timer.elapsed()) << ")\n" << endl;
    }

private:
	int extended_lines_ = 0;

	/*
	 *  returns a formatted time string (QByteArray) from a given time in milliseconds
	 */
	QByteArray getTimeString(qint64 milliseconds)
	{
		QTime time(0,0,0);
		time = time.addMSecs(milliseconds);
		return time.toString("hh:mm:ss.zzz").toUtf8();
	}


	/*
	 *  parses the INFO id parameter and extracts the INFO ids for the annotation file and the
	 *	 corresponding output and modifies the given QByteArrayLists inplace
	 */
	void parseInfoIds(QByteArrayList &info_ids, QByteArrayList &out_info_ids,
					  const QByteArray &input_string, const QByteArray &prefix)
	{
		// iterate over all info ids
		foreach(QByteArray raw_string, input_string.split(','))
		{
			QByteArray out_info_id;
			QByteArrayList in_out_info_id = raw_string.split('=');
			info_ids.append(in_out_info_id[0].trimmed());
			if (in_out_info_id.size() == 1)
			{
				// id in annotation file and output file are identical
				out_info_id = in_out_info_id[0].trimmed();
			}
			else if (in_out_info_id.size() == 2)
			{
				// id in annotation file and output file differ
				out_info_id = in_out_info_id[1].trimmed();
			}
			else
			{
				THROW(ArgumentException, "Error while parsing \"info_ids\" entry \"" + raw_string
					  + "\"!")
			}

			// extend output ids by the given prefix
			if (prefix != "")
			{
				out_info_id = prefix + "_" + out_info_id;
			}
			out_info_ids.append(out_info_id);
		}
	}

	/*
	 *  parses the column id parameter and returns the annotation id column name and the id column
	 *	 name in the output file by modifying the given QByteArrays inplace
	 */
	void parseIdColumn(QByteArray &id_column_name, QByteArray &out_id_column_name,
					   const QByteArray &input_string, const QByteArray &prefix)
	{
		// skip empty column id
		if (input_string.trimmed() == "")
		{
			id_column_name = "";
			out_id_column_name = "";
			return;
		}

		QByteArrayList id_column_parameter = input_string.split('=');
		id_column_name = id_column_parameter[0].trimmed();
		if (id_column_parameter.size() == 1)
		{
			// no optional name given
			out_id_column_name = "ID";
		}
		else if (id_column_parameter.size() == 2)
		{
			// alternative id name given
			out_id_column_name = id_column_parameter[1].trimmed();
		}
		else
		{
			THROW(ArgumentException, "Error while parsing \"id_column\" parameter!")
		}

		// extend output ids by the given prefix
		if (prefix != "")
		{
			out_id_column_name = prefix + "_" + out_id_column_name;
		}
	}

	/*
	 *		returns the concatinated INFO ids for the annotation
	 */
	QByteArrayList getOutputNames(QByteArray output_id_string, const QByteArrayList &input_info_ids,
								  const QByteArray &prefix)
	{
		QByteArrayList output_info_ids;
		if(output_id_string == "")
		{
			// if no alternative output ids are provided simply use the original id
			output_info_ids = input_info_ids;
		}
		else
		{
			// parse output ids:
			output_info_ids = output_id_string.replace(" ", "").split(',');

			// check if input and output ids match
			if (input_info_ids.size() != output_info_ids.size())
			{
				QByteArray error_string = "Number of given input and output info ids do not match! "
						+ QByteArray::number(input_info_ids.size()) + " input info ids but "
						+ QByteArray::number(output_info_ids.size()) + " output info ids are given.";
				THROW(ArgumentException, error_string)
			}
		}

		// extend output ids by prefix
		if (prefix != "")
		{
			for (int i = 0; i < output_info_ids.size(); i++)
			{
				output_info_ids[i] = prefix + "_" + output_info_ids[i];
			}
		}
		return output_info_ids;
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
				THROW(FileAccessException, "Could not open file '" + vcf_file_path
					  + "' for reading!");
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
			THROW(FileParseException, "File type of file \"" + vcf_file_path
				  + "\" is invalid/unknown!");
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
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

