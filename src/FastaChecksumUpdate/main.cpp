#include "ToolBase.h"
#include "Helper.h"
#include <QCryptographicHash>

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv), crypt(QCryptographicHash::Md5), debug_stream(stdout)
	{
	}

	virtual void setup()
	{
		setDescription("Fix Checksums in a FASTA file.");
		addInfile("in", "Input FASTA file.", false);
		addOutfile("out", "Output file.", false);
		addFlag("debug", "Write debug statements");

		//changelog
		changeLog(2026, 2, 27, "Initial version.");
	}

	virtual void main()
	{
		//input checks
		if (getInfile("in")==getOutfile("out")) THROW(Exception, "'in' and 'out' cannot be the same file!");

		in = Helper::openFileForReading(getInfile("in"));
		out = Helper::openFileForWriting(getOutfile("out"));
		debug = getFlag("debug");

		QTextStream debug_stream(stdout);

		QList<QByteArray> current_header;

		while(!in->atEnd())
		{
			QByteArray line = in->readLine();
			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

			//skip empty lines
			if(line.isEmpty()) continue;

			if (line.startsWith(">")) // init line
			{
				fixChecksum();

				QByteArray header = line.mid(1).trimmed();
				out->write(">");
				auto header_list = header.split(' ');
				for (int i =0; i < header_list.count(); ++i){
					if (header_list[i].size() > 3 && header_list[i].left(3) == "M5:") //checksum column
					{
						md5_checksum_pos = out->pos();
						stored_checksum = header_list[i].mid(3);
					}
					out->write(header_list[i]);
					if (i == header_list.count() - 1) out->write("\n");
					else out->write(" ");
				}

				crypt.reset();
			}
			else
			{
				crypt.addData(line);
				out->write(line);
				out->write("\n");
			}
		}
		fixChecksum();
	}
private:
	QCryptographicHash crypt;
	qint64 md5_checksum_pos = -1;
	QByteArray stored_checksum;
	bool debug;
	QTextStream debug_stream;
	QSharedPointer<QFile> in;
	QSharedPointer<QFile> out;

	void fixChecksum()
	{
		if (md5_checksum_pos != -1)
		{
			QByteArray checksum = crypt.result().toHex();
			if (debug)
			{
				debug_stream << "original checksum: " << stored_checksum << "\n";
				debug_stream << "new checksum: " << checksum << "\n";
			}
			if (checksum != stored_checksum)
			{
				if (debug) debug_stream << "checksum mismatch! Rewriting.\n";

				out->seek(md5_checksum_pos); //seek back to write the correct value
				out->write("M5:");
				out->write(checksum);
				out->seek(out->size()); //seek to the end
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
