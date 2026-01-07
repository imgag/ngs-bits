#include "ToolBase.h"
#include "Helper.h"
#include "VcfFile.h"

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
		setDescription("Annotates source variants to info field for a given VCF.");

		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);

		changeLog(2025, 12,  5, "Initial implementation.");
	}

	virtual void main()
	{
		//open input/output streams
		QString in = getInfile("in");
		QString out = getOutfile("out");
		if (in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
		QByteArrayList header_buffer;
		bool in_header = true;
		bool info_header_set = false;

		while(!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//add SOURCE_VAR info header line if not present
			if (line.startsWith("#") && in_header)
			{
				if (line.startsWith("#CHROM"))
				{
					for (auto &hline : header_buffer)
					{
						if (hline.startsWith("##INFO") && !info_header_set)
						{
							out_p->write("##INFO=<ID=SOURCE_VAR,Number=A,Type=String,Description=\"Source variant\">\n");
							info_header_set = true;
						}
						out_p->write(hline);
					}

					out_p->write(line);

					in_header = false;
					continue;
				}

				if (line.startsWith("##INFO=<ID=SOURCE_VAR"))
				{
					info_header_set = true;
				}

				header_buffer.append(line);
				continue;
			}

			//split line and extract variant infos
			QByteArrayList parts = line.trimmed().split('\t');
			QByteArray info = parts[VcfFile::INFO];
			QByteArray new_info = "SOURCE_VAR=" + parts[VcfFile::CHROM] + "&" + parts[VcfFile::POS] + "&" + parts[VcfFile::REF] + "&" + parts[VcfFile::ALT];

			//remove old annotation if present
			if (info.contains("SOURCE_VAR"))
			{
				QByteArrayList info_parts = info.trimmed().split(';');
				QByteArrayList cleaned;

				for (const QByteArray &p : info_parts)
				{
					if (!p.startsWith("SOURCE_VAR")) cleaned.append(p);
				}
				info = cleaned.join(";");
			}

			//add source variant annotation
			if (info.isEmpty()) info.append(new_info);
			else info.append(";" + new_info);

			parts[VcfFile::INFO] = info;

			//output line
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
