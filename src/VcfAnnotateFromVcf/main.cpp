#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QFile>
#include <QSharedPointer>
#include <QThreadPool>
#include "ChunkProcessor.h"
#include "OutputWorker.h"
#include "ThreadCoordinator.h"


class ConcreteTool
        : public ToolBase
{
    Q_OBJECT

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
		setExitEventLoopAfterMain(false);
    }


    virtual void setup()
    {
        setDescription("Annotates the INFO column of a VCF with data from another VCF file (or multiple VCF files if config file is provided).");

        //optional
		addInfile("in", "Input VCF(.GZ) file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);
		addInfile("config_file", "TSV file containing the annotation file path, the prefix, the INFO ids and the id column for multiple annotations.", true);
        addInfile("annotation_file", "Tabix indexed VCF.GZ file used for annotation.", true, true);
        addString("info_ids", "INFO id(s) in annotation VCF file (Multiple ids can be separated by ',', optional new id names in output file can be added by '=': original_id=new_id).", true, "");
        addString("id_column", "Name of the ID column in annotation file. (If "" it will be ignored in output file, alternative output name can be specified by old_id_column_name=new_name", true, "");
        addString("id_prefix", "Prefix for INFO id(s) in output VCF file.", true, "");
		addFlag("allow_missing_header", "If set the execution is not aborted if a INFO header is missing in annotation file.");
		addInt("threads", "The number of threads used to process VCF lines.", true, 1);
		addInt("block_size", "Number of lines processed in one chunk.", true, 10000);
		addInt("prefetch", "Maximum number of chunks that may be pre-fetched into memory.", true, 64);
		addFlag("debug", "Enables debug output (use only with one thread).");

		changeLog(2022, 2, 24, "Refactoring and change to event-driven implementation (improved scaling with many threads)");
		changeLog(2021, 9, 20, "Prefetch only part of input file (to save memory).");
		changeLog(2020, 4, 11, "Added multithread support.");
        changeLog(2019, 8, 19, "Added support for multiple annotations files through config file.");
        changeLog(2019, 8, 14, "Added VCF.GZ support.");
        changeLog(2019, 8, 13, "Initial implementation.");
    }

    virtual void main()
    {
        //init
		QTextStream out(stdout);

		//parse parameters
		Parameters params;
		params.in = getInfile("in");
		params.out = getOutfile("out");
        QString config_file_path = getInfile("config_file");
        QString annotation_file_path = getInfile("annotation_file");
        QByteArray info_id_string = getString("info_ids").toLatin1().trimmed();
        QByteArray id_column = getString("id_column").toLatin1().trimmed();
        QByteArray id_prefix = getString("id_prefix").toLatin1().trimmed();
		bool allow_missing_header = getFlag("allow_missing_header");
		params.threads = getInt("threads");
		params.prefetch = getInt("prefetch");
		params.block_size = getInt("block_size");
		params.debug = getFlag("debug");

		//check parameters
		if (params.block_size < 1) THROW(ArgumentException, "Parameter 'block_size' has to be greater than zero!");
		if (params.threads < 1) THROW(ArgumentException, "Parameter 'threads' has to be greater than zero!");
		if (params.prefetch < params.threads) THROW(ArgumentException, "Parameter 'prefetch' has to be at least number of used threads!");
		if (params.in!="" && params.in==params.out) THROW(ArgumentException, "Input and output files must be different when streaming!");

		//get meta data
		MetaData meta;
		if (config_file_path != "") //from file
        {
            // parse config file line by line
            QSharedPointer<QFile> config_file = Helper::openFileForReading(config_file_path, false);
			while (!config_file->atEnd())
            {
				QByteArray line = config_file->readLine();
                // skip empty or comment lines
                if ((line.trimmed() == "") || (line.startsWith('#'))) continue;
                QByteArrayList columns = line.split('\t');
                if (columns.size() < 4)
                {
					THROW(FileParseException, QByteArray() + "Invalid number of columns! "  + "File name, prefix, INFO ids and id column name are required:\n"  + line.replace("\t", " -> ").trimmed());
                }

				meta.annotation_file_list.append(columns[0].trimmed());

                QByteArray prefix = columns[1];
                QByteArrayList info_ids;
                QByteArrayList out_info_ids;
                parseInfoIds(info_ids, out_info_ids, columns[2], prefix);

				meta.prefix_list.append(prefix);
				meta.info_id_list.append(info_ids);
				meta.out_info_id_list.append(out_info_ids);

                QByteArray id_column_name;
                QByteArray out_id_column_name;
                parseIdColumn(id_column_name, out_id_column_name, columns[3], prefix);

				meta.id_column_name_list.append(id_column_name);
				meta.out_id_column_name_list.append(out_id_column_name);

                if (columns.size() > 4)
                {
					meta.allow_missing_header_list.append((columns[4].trimmed().toLower() == "true") || (columns[4].trimmed() == "1"));
                }
                else
                {
					meta.allow_missing_header_list.append(false);
                }
            }

			if (meta.annotation_file_list.size() < 1)
            {
				THROW(FileParseException, "The config file has to contain at least 1 valid annotation configuration!");
            }
        }
		else //from CLI parameters
        {
			meta.annotation_file_list.append(annotation_file_path.toLatin1().trimmed());

            if (info_id_string.trimmed() == "")
            {
                THROW(ArgumentException,
                      "The \"info_id\" parameter is required if no config file is provided");
            }
            QByteArrayList info_ids;
            QByteArrayList out_info_ids;
            parseInfoIds(info_ids, out_info_ids, info_id_string, id_prefix);

			meta.prefix_list.append(id_prefix);
			meta.info_id_list.append(info_ids);
			meta.out_info_id_list.append(out_info_ids);

            QByteArray id_column_name;
            QByteArray out_id_column_name;
            parseIdColumn(id_column_name, out_id_column_name, id_column, id_prefix);

			meta.id_column_name_list.append(id_column_name);
			meta.out_id_column_name_list.append(out_id_column_name);

			meta.allow_missing_header_list.append(allow_missing_header);
        }

		//check meta data
		QByteArrayList tmp;
		foreach (QByteArrayList ids, meta.out_info_id_list)
		{
			tmp.append(ids);
		}
		QSet<QByteArray> unique_output_ids = QSet<QByteArray>::fromList(tmp);
		if (unique_output_ids.size() < tmp.size())
		{
			THROW(FileParseException, "The given output INFO ids contain duplicates!")
		}

		//write meta data to stdout
		out << "Input file: \t" << params.in << "\n";
		out << "Output file: \t" << params.out << "\n";
		out << "Threads: \t" << params.threads << "\n";
		out << "Block (Chunk) size: \t" << params.block_size << "\n";

		for(int i = 0; i < meta.annotation_file_list.size(); i++)
        {
			out << "Annotation file: " << meta.annotation_file_list[i] << "\n";

			if (meta.id_column_name_list[i] != "")
            {
				out << "Id column:\n\t " << meta.id_column_name_list[i].leftJustified(12) << "\t -> \t" << meta.out_id_column_name_list[i] << "\n";
            }
            out << "INFO ids:\n";
			for (int j = 0; j < meta.info_id_list[i].size(); j++)
            {
				out << "\t " << meta.info_id_list[i][j].leftJustified(12) << "->   " << meta.out_info_id_list[i][j] << endl;
            }
		}

		//create coordinator instance
		out << "Performing annotation" << endl;
		ThreadCoordinator* coordinator = new ThreadCoordinator(this, params, meta);
		connect(coordinator, SIGNAL(finished()), QCoreApplication::instance(), SLOT(quit()));
    }

private:

	//parses the INFO id parameter and extracts the INFO ids for the annotation file and the corresponding output and modifies the given QByteArrayLists inplace
	void parseInfoIds(QByteArrayList &info_ids, QByteArrayList &out_info_ids, const QByteArray &input_string, const QByteArray &prefix)
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

	//parses the column id parameter and returns the annotation id column name and the id column name in the output file by modifying the given QByteArrays inplace
	void parseIdColumn(QByteArray &id_column_name, QByteArray &out_id_column_name,  const QByteArray &input_string, const QByteArray &prefix)
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

};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

