#include "ToolBase.h"
#include "BamReader.h"
#include "Helper.h"
#include "Settings.h"

#include <HttpRequestHandler.h>
#include <ProxyDataService.h>

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
		setDescription("Download the reference genome FASTA file for a BAM/CRAM file.");
		addInfile("in", "Input BAM/CRAM file.", false);
		addOutfile("out", "Output reference genome FASTA file.", false);
		//optional
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);

		//changelog
		changeLog(2024,  8, 18, "Initial version.");
	}

	virtual void main()
	{
		//init
		QTextStream out_stream(stdout);
		HttpRequestHandler request_handler(ProxyDataService::getProxy());
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), false);

		//open refererence genome file (we don't need it, but the BamReader needs it for CRAM files)
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");

		QString in = getInfile("in");
		BamReader reader(in, ref_file);
		foreach(QString line, reader.headerLines())
		{
			line = line.trimmed();
			if (!line.startsWith("@SQ")) continue;
            out_stream << line << Qt::endl;

			QString name = "";
			QString md5 = "";
			QStringList parts = line.split("\t");
			foreach(QString part, parts)
			{
				part = part.trimmed();
				if (part.startsWith("SN:"))
				{
					name = part.mid(3);
				}
				if (part.startsWith("M5:"))
				{
					md5 = part.mid(3);
				}
			}

			//checks
			if (name.isEmpty()) THROW(FileParseException, "Invalid @SQ line without name found: " + line)
			if (md5.isEmpty())
			{
                out_stream << "Skipped chromosome '" << name << "': @SQ line contains no M5 entry" << Qt::endl;
				continue;
			}

			//download
			QString url = "https://www.ebi.ac.uk/ena/cram/md5/" + md5;
			try
			{
				ServerReply reply = request_handler.get(url);
				out->write(">" + name.toLatin1() + "\n");
				out->write(reply.body);
				out->write("\n");
				out->flush();
			}
			catch (Exception& e)
			{
                out_stream << "Skipped chromosome '" << name << "': could not download " << url << Qt::endl;
			}
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
