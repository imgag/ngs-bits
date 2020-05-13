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
#include <QThreadPool>
#include "ChunkProcessor.h"
#include <iostream>
#include <fstream>
#include <QTime>


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
        addInt("threads", "The number of threads used to read, process and write files.", true, 1);
		addInt("block_size", "Number of lines processed in one chunk.", true, 5000);

        changeLog(2020, 4, 11, "Added multithread support by Julian Fratte.");
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

        int block_size = getInt("block_size");
        int threads = getInt("threads");

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
        out << "Threads: \t" << threads << "\n";
        out << "Block (Chunk) size: \t" << block_size << "\n";

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

        QThreadPool analysis_pool;
        analysis_pool.setMaxThreadCount(getInt("threads"));

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

        //QByteArrayList data;
        int current_chunk = 0;
        int vcf_line_idx = 0;


		// iterate over the vcf file line by line and create job pool
		QList<AnalysisJob> job_pool;
        while(!eof)
        {
			AnalysisJob job = AnalysisJob();

            job.chunk_id = current_chunk;
            job.status = TO_BE_PROCESSED;

            while(vcf_line_idx < block_size && !eof)
            {
                // get next line
                QByteArray line;
                if (format == VariantListFormat::VCF)
                {
                    line = input_vcf -> readLine();
                    job.current_chunk.append(line);

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
                    job.current_chunk.append(line);

                    // check for eof:
                    eof = gzeof(input_vcf_gz);
                }

                vcf_line_idx++;

            }


            vcf_line_idx=0;
            ++current_chunk;
			job_pool << job;
        }

		// start all created jobs
		for (int i = 0; i < job_pool.size(); ++i)
		{
			analysis_pool.start(new ChunkProcessor(job_pool[i],
												   prefix_list,
												   unique_output_ids,
												   info_id_list,
												   out_info_id_list,
												   id_column_name_list,
												   out_id_column_name_list,
												   allow_missing_header_list,
												   annotation_file_list,
												   output_path,
												   input_path));
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

        QSharedPointer<QFile> output_vcf = Helper::openFileForWriting(output_path, true);

        int write_chunk = 0;
        int done = 0;

        while(done < job_pool.count())
        {
            done=0;
            for (int j=0; j<job_pool.count(); ++j)
            {
                AnalysisJob& job = job_pool[j];

                switch(job.status)
                {
                case DONE:
                    ++done;
                    break;

                case TO_BE_WRITTEN:
                    if (job.chunk_id == write_chunk)
                    {

                        for(QByteArray line : job.current_chunk_processed)
                        {
                            output_vcf -> write(line);
                        }

                        ++write_chunk;
                        job.status = DONE;
                    }

                default:
                    break;
                }
            }
        }

        out << "Programm finished!" << endl;

    }

private:

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

    //	/*
    //	 *  returns the value of a given INFO key from a given INFO header line
    //	 */
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

};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

