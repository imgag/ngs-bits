#include "Exceptions.h"
#include "ToolBase.h"
#include "BedFile.h"
#include "VcfFile.h"
#include "ChromosomalIndex.h"
#include "Helper.h"
#include <QFile>
#include <QRegularExpression>

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
		setDescription("Fixes VCF file from Sniffles SV Caller.");
		setExtendedDescription(QStringList() << "Converts lowEvidence variants into het variants." );
		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);

		changeLog(2025, 2, 14, "Initial implementation.");
		changeLog(2025, 4, 8, "Added sorting of read names.");
	}

	virtual void main()
	{
		//open input/output streams
		QString in = getInfile("in");
		QString out = getOutfile("out");
		if(in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);


		// Read input
		QTextStream std_err(stderr);
		int column_count = 0;
		while (!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//split and trim
			QByteArrayList parts = line.split('\t');
			Helper::trim(parts);

			//handle header columns
			if (line.startsWith('#'))
			{
				if (!line.startsWith("##"))
				{
					column_count = parts.count();

					//add new filter
					out_p->write("##FILTER=<ID=LOW_EVIDENCE,Description=\"Low evidence variants.\">\n");
				}

				out_p->write(line);
				continue;
			}
			if (column_count > 10) THROW(ArgumentException, "Multi sample VCF not supported!");

			//get genotype
			QByteArrayList format_headers = parts.at(VcfFile::FORMAT).split(':');
			QByteArrayList format_values = parts.at(VcfFile::FORMAT + 1).split(':');
			if (format_headers.contains("GT"))
			{
				QString gt = format_values.at(format_headers.indexOf("GT"));
				if (gt == "0/0")
				{
					format_values[format_headers.indexOf("GT")] = "0/1";
					parts[VcfFile::FORMAT + 1] = format_values.join(":");

					// set filter column
					if (parts[VcfFile::FILTER] == "PASS") parts[VcfFile::FILTER] = "LOW_EVIDENCE";
					else	parts[VcfFile::FILTER].append(";LOW_EVIDENCE");
				}
			}


			if (parts.at(VcfFile::INFO).contains("RNAMES="))
			{
				//sort read names
				QByteArrayList info_values = parts.at(VcfFile::INFO).split(';');
				for (int i = 0; i < info_values.size(); ++i)
				{
					if (info_values[i].startsWith("RNAMES="))
					{
						QByteArrayList read_names = info_values[i].split('=').at(1).split(',');
						std::sort(read_names.begin(), read_names.end());
						info_values[i] = "RNAMES=" + read_names.join(',');
						break;
					}
				}
				parts[VcfFile::INFO] = info_values.join(";");
			}



			out_p->write(parts.join("\t")+"\n");

		}

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
