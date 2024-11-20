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

		//optional
        addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF file. If unset, writes to STDOUT.", true, true);
		addString("info", "Comma-separated list of infos to keep. When not providing an info list, all infos are kept.", true);
		addString("format", "Comma-separated list of formats to keep. When not providing a format list, all formats are kept", true);
		addFlag("clear_info", "remove all info fields");

		changeLog(2024, 11, 20, "Initial implementation.");
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
		bool clear_info = getFlag("clear_info");

        if(in!="" && in==out)
        {
            THROW(ArgumentException, "Input and output files must be different when streaming!");
        }

        QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);

		//get list of formats and infos to keep
		QSet<QByteArray> formats_keep = getString("format").toUtf8().split(',').toSet().subtract(QSet<QByteArray>{""});
		QSet<QByteArray> infos_keep = getString("info").toUtf8().split(',').toSet().subtract(QSet<QByteArray>{""});

        while(!in_p->atEnd())
        {
            QByteArray line = in_p->readLine();
			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

            //skip empty lines
			if (line.isEmpty()) continue;

			//remove unwanted header
            if (line.startsWith('#'))
			{
				QByteArray line_id = getId(line);
				if (line.startsWith("##INFO"))
				{
					if (!infos_keep.isEmpty())
					{
						if (!infos_keep.contains(line_id)) continue;
					}
					else if (clear_info) continue;
					else
					{
						out_p->write(line.append(('\n')));
						continue;
					}
                }
				else if (line.startsWith("##FORMAT") && (!formats_keep.contains(line_id) && !formats_keep.isEmpty()))
				{
					continue;
                }
				out_p->write(line.append('\n'));
                continue;
            }


			//split line and extract format field and sample count
			QByteArrayList parts = line.split('\t');
			if (parts.length() < VcfFile::MIN_COLS) THROW(FileParseException, "VCF with too few columns: " + line);

			QByteArrayList format;
			int samples_count = 0;

			if (parts.length() > VcfFile::MIN_COLS)
			{
				format = parts[VcfFile::FORMAT].split(':');
				samples_count = parts.length() - VcfFile::FORMAT - 1;
			}

			//construct a new info block
			if (!infos_keep.isEmpty())
			{
				QByteArray new_infos;
				QByteArrayList info = parts[VcfFile::INFO].split(';');
				for (int i = 0; i < info.length(); ++i)
				{
					//determine info ID
					QByteArray info_id;
					if (info[i].contains("="))
					{
						int id_len = info[i].indexOf("=");
						info_id = info[i].mid(0, id_len);
					}
					else info_id = info[i];

					//check whether to keep or discard the info
					if (infos_keep.contains(info_id))
					{
						if (!new_infos.isEmpty()) new_infos += ";";
						new_infos += info[i];
					}
				}
				if (new_infos.isEmpty()) new_infos = ".";
				//exchange info field
				parts[VcfFile::INFO] = new_infos;
			}
			else if (clear_info) parts[VcfFile::INFO] = ".";

			//construct new format block and save indices of entries to keep (to construct sample blocks later)
			QByteArray new_formats;
			QList<int> format_indices;
			if (samples_count!=0 && (!formats_keep.isEmpty()))
			{
				for (int i = 0; i < format.length(); ++i)
				{
					if (formats_keep.contains(format[i]))
					{
						if (!new_formats.isEmpty()) new_formats += ":";
						new_formats += format[i];
						format_indices.append(i);
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
