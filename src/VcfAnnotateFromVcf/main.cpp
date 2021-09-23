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
#include "OutputWorker.h"
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
		addInt("prefetch", "Maximum number of chunks that may be pre-fetched into memory.", true, 64);

		changeLog(2021, 9, 20, "Prefetch only part of input file (to save memory).");
        changeLog(2020, 4, 11, "Added multithread support by Julian Fratte.");
        changeLog(2019, 8, 19, "Added support for multiple annotations files through config file.");
        changeLog(2019, 8, 14, "Added VCF.GZ support.");
        changeLog(2019, 8, 13, "Initial implementation.");
    }

    virtual void main()
    {
        //init
		QTextStream out(stdout);

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
		int prefetch = getInt("prefetch");

		// check parameter:
		if (block_size < 1) THROW(ArgumentException, "Parameter 'block_size' has to be greater than zero!");
		if (threads < 1) THROW(ArgumentException, "Parameter 'threads' has to be greater than zero!");
		if (prefetch < threads) THROW(ArgumentException, "Parameter 'prefetch' has to be at least number of used threads!");

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

		// create job pool
		QList<AnalysisJob> job_pool;
		while(job_pool.count() < prefetch)
		{
			job_pool << AnalysisJob();
		}

		//create thread pool
        QThreadPool analysis_pool;
		analysis_pool.setMaxThreadCount(threads + 1); // +1 for output writer

		// start writer thread
		OutputWorker* output_worker = new OutputWorker(job_pool, output_path);
		analysis_pool.start(output_worker);

		//open input file
		FILE* instream = input_path.isEmpty() ? stdin : fopen(input_path.toLatin1().data(), "rb");
		gzFile file = gzdopen(fileno(instream), "rb"); //always open in binary mode because windows and mac open in text mode
		if (file==NULL)
		{
			THROW(FileAccessException, "Could not open file '" + input_path + "' for reading!");
		}

		const int buffer_size = 1048576; //1MB buffer
		char* buffer = new char[buffer_size];
		int current_chunk = 0;
		int vcf_line_idx = 0;

		try
		{
			while(!gzeof(file))
			{
				int to_be_processed = 0;
				int to_be_written = 0;
				int done = 0;
				for (int j=0; j<job_pool.count(); ++j)
				{
					AnalysisJob& job = job_pool[j];
					switch(job.status)
					{
						case DONE:
							++done;

							// create new job with the next chunk of lines
							job.clear();
							job.chunk_id = current_chunk;
							job.status = TO_BE_PROCESSED;

							while(vcf_line_idx < block_size && !gzeof(file))
							{
								// get next line
								char* char_array = gzgets(file, buffer, buffer_size);

								//handle errors like truncated GZ file
								if (char_array==nullptr)
								{
									int error_no = Z_OK;
									QByteArray error_message = gzerror(file, &error_no);
									if (error_no!=Z_OK && error_no!=Z_STREAM_END)
									{
										THROW(FileParseException, "Error while reading file '" + input_path + "': " + error_message);
									}
								}

								job.current_chunk.append(QByteArray(char_array));

								vcf_line_idx++;
							}
							vcf_line_idx = 0;
							analysis_pool.start(new ChunkProcessor(job,
																   prefix_list,
																   unique_output_ids,
																   info_id_list,
																   out_info_id_list,
																   id_column_name_list,
																   out_id_column_name_list,
																   allow_missing_header_list,
																   annotation_file_list));
							++current_chunk;
							break;

						case TO_BE_WRITTEN:
							++to_be_written;
							//sleep
							QThread::msleep(100);
							break;

						case TO_BE_PROCESSED:
							++to_be_processed;
							//sleep
							QThread::msleep(100);
							break;

						case ERROR:
							if (job.error_message.startsWith("FileParseException\t"))
							{
								THROW(FileParseException, job.error_message.split("\t").at(1));
							}
							else if (job.error_message.startsWith("FileAccessException\t"))
							{
								THROW(FileAccessException, job.error_message.split("\t").at(1));
							}
							else
							{
								THROW(Exception, job.error_message);
							}
							break;
						default:
							break;
					}

					//break if file is read
					if(gzeof(file)) break;
				}
			}

			//close file
			gzclose(file);
			delete[] buffer;

			//wait for all jobs to finish
			int done =0;
			int to_be_written, to_be_processed;
			while (done < job_pool.count())
			{
				done = 0;
				to_be_written = 0;
				to_be_processed = 0;

				for (int j=0; j<job_pool.count(); ++j)
				{
					AnalysisJob& job = job_pool[j];

					switch(job.status)
					{
						case DONE:
							++done;
							break;

						case TO_BE_WRITTEN:
							to_be_written++;
							//sleep
							QThread::msleep(100);
							break;

						case TO_BE_PROCESSED:
							++to_be_processed;
							//sleep
							QThread::msleep(100);
							break;

						case ERROR:
							if (job.error_message.startsWith("FileParseException\t"))
							{
								THROW(FileParseException, job.error_message.split("\t").at(1));
							}
							else if (job.error_message.startsWith("FileAccessException\t"))
							{
								THROW(FileAccessException, job.error_message.split("\t").at(1));
							}
							else
							{
								THROW(Exception, job.error_message);
							}
							break;

						default:
							break;
					}
				}
			}

			//terminater output worker
			output_worker->terminate();
			out << "Programm finished!" << endl;

		}
		catch (...)
		{
			delete[] buffer;
			//terminate output worker if an exeption is thrown
			output_worker->terminate();
			throw;
		}
    }

private:

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

};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

