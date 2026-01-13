#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QFile>
#include <QSharedPointer>
#include "ThreadCoordinator.h"
#include <htslib/tbx.h>

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
		setDescription("Annotates a VCF file with data from one or more source VCF files.");
		setExtendedDescription(QStringList() << "NOTICE: the parameter '-existence_only' cannot be used together with '-config_file', '-info_keys' or '-id_column'.");

        //optional
		addInfile("in", "Input VCF(.GZ) file that is annotated. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF file. If unset, writes to STDOUT.", true, true);
		addInfile("config_file", "TSV file for annotation from multiple source files. For each source file, these tab-separated columns have to be given: source file name, prefix, INFO keys, ID column.", true);
		addInfile("source", "Tabix indexed VCF.GZ file that is the source of the annotated data.", true, true);
		addString("info_keys", "INFO key(s) in 'source' that should be annotated (Multiple keys are be separated by ',', optional keys can be renamed using this syntax: 'original_key=new_key').", true, "");
		addString("id_column", "ID column in 'source' (must be 'ID'). If unset, the ID column is not annotated. Alternative output name can be specified by using 'ID=new_name'.", true, "");
		addString("prefix", "Prefix added to all annotations in the output VCF file.", true, "");
		addFlag("allow_missing_header", "If set the execution is not aborted if a INFO header is missing in the source file.");
		addFlag("existence_only", "Only annotate if variant exists in source.");
		addString("existence_key_name", "Defines the INFO key name.", true, "EXISTS_IN_SOURCE");
		addInt("threads", "The number of threads used to process VCF lines.", true, 1);
		addInt("block_size", "Number of lines processed in one chunk.", true, 10000);
		addInt("prefetch", "Maximum number of chunks that may be pre-fetched into memory.", true, 64);
		addFlag("debug", "Enables debug output (use only with one thread).");
		addFlag("hts_version", "Prints used htlib version and exits.");

		changeLog(2024, 5,  6, "Added option to annotate the existence of variants in the source file");
		changeLog(2022, 7,  8, "Usability: changed parameter names and updated documentation.");
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

		//version output
		if (getFlag("hts_version"))
		{
			out << "htslib version: " << hts_version() << "\n";
			quit();
			return;
		}

		//parse parameters
		Parameters params;
		params.in = getInfile("in");
		params.out = getOutfile("out");
		QString file_path = getInfile("config_file").trimmed();
		QString source = getInfile("source").trimmed();
		QByteArray info_keys = getString("info_keys").toUtf8().trimmed();
        QByteArray id_column = getString("id_column").toUtf8().trimmed();
		QByteArray prefix = getString("prefix").toUtf8().trimmed();
		bool allow_missing_header = getFlag("allow_missing_header");
		bool existence_only = getFlag("existence_only");
		QByteArray existence_key_name = getString("existence_key_name").toUtf8().trimmed();
		params.threads = getInt("threads");
		params.prefetch = getInt("prefetch");
		params.block_size = getInt("block_size");
		params.debug = getFlag("debug");

		//check parameters
		if (params.block_size < 1) THROW(ArgumentException, "Parameter 'block_size' has to be greater than zero!");
		if (params.threads < 1) THROW(ArgumentException, "Parameter 'threads' has to be greater than zero!");
		if (params.prefetch < params.threads) THROW(ArgumentException, "Parameter 'prefetch' has to be at least number of used threads!");
		if (params.in!="" && params.in==params.out) THROW(ArgumentException, "Input and output files must be different when streaming!");
		if (existence_only && (!file_path.isEmpty() || !info_keys.isEmpty() || !id_column.isEmpty())) THROW(ArgumentException, "Parameter 'existence_only' cannot be used together with '-config_file', '-info_keys' or '-id_column'!");
		if (existence_only && existence_key_name.isEmpty()) THROW(ArgumentException, "Parameter 'existence_key_name' cannot be empty!");

		//get meta data
		MetaData meta;
		if (file_path != "") //from file
        {
            // parse config file line by line
			QSharedPointer<QFile> config_file = Helper::openFileForReading(file_path, false);
			while (!config_file->atEnd())
            {
				QByteArray line = config_file->readLine();
                // skip empty or comment lines
                if ((line.trimmed() == "") || (line.startsWith('#'))) continue;
                QByteArrayList columns = line.split('\t');
                if (columns.size() < 4)
                {
					THROW(FileParseException, QByteArray() + "Config file line does not contain 4 tab-separated columns (source file name, prefix, INFO keys, ID column). Found:\n"  + line.replace("\t", " -> ").trimmed());
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

				// read columns for existence annotation
				if (columns.size() > 5)
				{
					meta.annotate_only_existence.append((columns[5].trimmed().toLower() == "true") || (columns[5].trimmed() == "1"));
				}
				else
				{
					meta.annotate_only_existence.append(false);
				}

				if ((columns.size() > 6) && !columns[6].trimmed().isEmpty())
				{
					meta.existence_name_list.append(columns[6].trimmed());
				}
				else
				{
					meta.existence_name_list.append("EXISTS_IN_SOURCE");
				}

				// check parsing result
				if (meta.annotate_only_existence.last() && (!out_info_ids.isEmpty() || !out_id_column_name.isEmpty())) THROW(ArgumentException, "'existence_only' annotation cannot be used together with INFO/ID annotation!")
            }

			if (meta.annotation_file_list.size() < 1)
            {
				THROW(FileParseException, "The config file has to contain at least 1 valid annotation configuration!");
            }
        }
		else //from CLI parameters
        {
			meta.annotation_file_list.append(source.toUtf8());

			if (info_keys.isEmpty() && id_column.isEmpty() && !existence_only) THROW(ArgumentException, "One of the parameters 'info_keys', 'id_column' or 'existence_only' is required if no config file is provided!");
			if (source.isEmpty()) THROW(ArgumentException, "The 'source' parameter is required if no config file is provided!");

            QByteArrayList info_ids;
            QByteArrayList out_info_ids;
			parseInfoIds(info_ids, out_info_ids, info_keys, prefix);

			meta.prefix_list.append(prefix);
			meta.info_id_list.append(info_ids);
			meta.out_info_id_list.append(out_info_ids);

            QByteArray id_column_name;
            QByteArray out_id_column_name;
			parseIdColumn(id_column_name, out_id_column_name, id_column, prefix);

			meta.id_column_name_list.append(id_column_name);
			meta.out_id_column_name_list.append(out_id_column_name);

			meta.allow_missing_header_list.append(allow_missing_header);

			meta.annotate_only_existence.append(existence_only);
			meta.existence_name_list.append(existence_key_name);
        }

		//check meta data
		QByteArrayList tmp;
		foreach (QByteArrayList ids, meta.out_info_id_list)
		{
			tmp.append(ids);
		}
        QSet<QByteArray> unique_output_ids = LIST_TO_SET(tmp);
		if (unique_output_ids.size() < tmp.size())
		{
			THROW(FileParseException, "The given output INFO ids contain duplicates!")
		}

		//write meta data to stdout
		if (params.debug)
		{
			out << "Input file: \t" << params.in << "\n";
			out << "Output file: \t" << params.out << "\n";
			out << "Threads: \t" << params.threads << "\n";
			out << "Block (Chunk) size: \t" << params.block_size << "\n";

			for (int i=0; i<meta.annotation_file_list.size(); ++i)
			{
				out << "Annotation file: " << meta.annotation_file_list[i] << "\n";
				if (meta.annotate_only_existence[i])
				{
					out << "Existence-only annotation, INFO key name:\t" + meta.existence_name_list[i] + "\n";
				}
				else
				{
					if (meta.id_column_name_list[i] != "")
					{
						out << "Id column:\n\t " << meta.id_column_name_list[i].leftJustified(12) << "\t -> \t" << meta.out_id_column_name_list[i] << "\n";
					}
					out << "INFO ids:\n";
					for (int j = 0; j < meta.info_id_list[i].size(); j++)
					{
						out << "\t " << meta.info_id_list[i][j].leftJustified(12) << "->   " << meta.out_info_id_list[i][j] << QT_ENDL;
					}
				}
			}
		}

		//create coordinator instance
		if (params.debug) out << "Performing annotation" << QT_ENDL;
		ThreadCoordinator* coordinator = new ThreadCoordinator(this, params, meta);
		connect(coordinator, SIGNAL(finished()), QCoreApplication::instance(), SLOT(quit()));
    }

private:

	//parses the INFO id parameter and extracts the INFO ids for the annotation file and the corresponding output and modifies the given QByteArrayLists inplace
	void parseInfoIds(QByteArrayList &info_ids, QByteArrayList &out_info_ids, const QByteArray &input_string, const QByteArray &prefix)
    {
        // iterate over all info ids
		foreach(QByteArray entry, input_string.split(','))
        {
			entry = entry.trimmed();
			if (entry.isEmpty()) continue;

            QByteArray out_info_id;
			QByteArrayList parts = entry.split('=');
			info_ids.append(parts[0].trimmed());
			if (parts.size() == 1)
            {
                // id in annotation file and output file are identical
				out_info_id = parts[0].trimmed();
            }
			else if (parts.size() == 2)
            {
                // id in annotation file and output file differ
				out_info_id = parts[1].trimmed();
            }
            else
            {
				THROW(ArgumentException, "Error while parsing \"info_ids\" entry \"" + entry + "\"!")
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

		//check name
		QByteArrayList parts = input_string.trimmed().split('=');
		if (parts[0]!="ID")
		{
			THROW(ArgumentException, "Parameter \"id_column\" is '" + input_string + "', but column name must be 'ID'!")
		}

		id_column_name = "ID";
		out_id_column_name = "ID";
		if (parts.size()==2) // alternative id name given
        {

			out_id_column_name = parts[1].trimmed();
        }
		else if (parts.count()>2)
        {
			THROW(ArgumentException, "Parameter \"id_column\" contains more than one '='!")
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

