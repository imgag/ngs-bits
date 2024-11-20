#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "VcfFile.h"
#include <QRegularExpression>
#include <QFile>

class ConcreteTool: public ToolBase
{
    Q_OBJECT

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
    }

    virtual void setup()
    {
		setDescription("Removes unwanted format- and info-fields from VCF file only keeping specified fields.");
		addString("format", "Comma-separated list of formats to keep", false);

		//optional
        addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF file. If unset, writes to STDOUT.", true, true);
		addString("info", "Comma-separated list of infos to keep. When not providing an info list, no infos are kept.", true, "none");



		changeLog(2018, 10, 18, "Initial implementation.");
    }

	///Return ID from FORMAT/INFO line
	QByteArray getId(const QByteArray& header)
	{
		int start = header.indexOf("ID=") + 3;
		int end = header.indexOf(',', start);
		return header.mid(start, end-start);
	}

    virtual void main()
    {
        //open input/output streams
        QString in = getInfile("in");
        QString out = getOutfile("out");
		QTextStream out_stream(stdout);

        if(in!="" && in==out)
        {
            THROW(ArgumentException, "Input and output files must be different when streaming!");
        }

        QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);

		//get list of formats and infos to keep
		QStringList formats_keep = getString("format").split(',');
		QStringList infos_keep = getString("info").split(',');

		//create regex from format and info list
		QRegularExpression formats_regex("ID=(" + formats_keep.join('|') + "),");
		QRegularExpression infos_regex("ID=(" + infos_keep.join('|') + "),");


        while(!in_p->atEnd())
        {
            QByteArray line = in_p->readLine();

            //skip empty lines
            if (line.trimmed().isEmpty()) continue;

			//convert line to QString for regex matching
			QString line_str = QString::fromUtf8(line);

			//remove unwanted header
            if (line.startsWith('#'))
			{
				if (line.startsWith("##INFO") && (!line_str.contains(infos_regex)))
				{
					continue;
                }
				else if (line.startsWith("##FORMAT") && (!line_str.contains(formats_regex)))
				{
					continue;
                }
				out_p->write(line);
                continue;
            }


			//split line and extract info, format field and sample count
			QByteArrayList parts = line.trimmed().split('\t');
			if (parts.length() < VcfFile::MIN_COLS) THROW(FileParseException, "VCF with too few columns: " + line);

			QByteArrayList info = parts[VcfFile::INFO].split(';');
			QByteArrayList format;
			int samples_count = 0;

			if (parts.length() > VcfFile::MIN_COLS)
			{
				format = parts[VcfFile::FORMAT].split(':');
				samples_count = parts.length() - VcfFile::FORMAT - 1;
			}

			//construct a new info block
			QByteArray new_infos;
			if (infos_keep[0]!="none")
			{
				for (int i = 0; i < info.length(); ++i)
				{
					for (int j = 0; j < infos_keep.length(); ++j)
					{
						if (info[i].startsWith(infos_keep[j].toUtf8() + "="))
						{
							if (!new_infos.isEmpty()) new_infos += ";";
							new_infos += info[i];
						}
					}
				}
			}
			else new_infos = ".";
			if (new_infos.isEmpty()) new_infos = ".";

			//exchange info field
			parts[VcfFile::INFO] = new_infos;

			//construct new format block and save indices of entries to keep (to construct sample blocks later)
			QByteArray new_formats;
			QList<int> format_indices;
			if (samples_count!=0)
			{
				for (int i = 0; i < format.length(); ++i)
				{
					for (int j = 0; j < formats_keep.length(); ++j)
					{
						if (format[i] == formats_keep[j])
						{
							if (!new_formats.isEmpty()) new_formats += ":";
							new_formats += format[i];
							format_indices.append(i);
							break;
						}
					}
				}

				if (new_formats.isEmpty()) new_formats = ".";

				//exchange format field
				parts[VcfFile::FORMAT] = new_formats;

				//construct new sample blocks
				for (int i = 0; i < samples_count; ++i)
				{
					QByteArray new_samples;
					int sample_column = VcfFile::FORMAT + i + 1;
					if (parts[sample_column] == ".") continue; //skip MISSING sample

					QByteArrayList sample_values = parts[sample_column].split(':');

					for (int j = 0; j < format_indices.length(); j++)
					{
						if (!new_samples.isEmpty()) new_samples += ":";
						new_samples += sample_values[format_indices[j]];
					}

					//exchange sample field
					if (new_samples.isEmpty()) new_samples = ".";
					parts[sample_column] = new_samples;
				}

			}

			//write output line
			out_p->write(parts.join('\t').append('\n'));
		}
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
