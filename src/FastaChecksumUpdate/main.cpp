#include "ToolBase.h"
#include "Helper.h"
#include <QCryptographicHash>

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv), debug_stream(stdout), crypt(QCryptographicHash::Md5)
	{
	}

	virtual void setup()
	{
		setDescription("Fix Checksums in a FASTA file.");
		addInfile("in", "Input FASTA file.", false);
		addOutfile("out", "Output file.", false);
		addFlag("debug", "Write debug statements");

		//changelog
		changeLog(2026, 3, 2, "Initial version.");
	}

	virtual void main()
	{
		//input checks
		if (getInfile("in")==getOutfile("out")) THROW(Exception, "'in' and 'out' cannot be the same file!");

		in = Helper::openFileForReading(getInfile("in"));
		out = Helper::openFileForWriting(getOutfile("out"));
		debug = getFlag("debug");

		while(!in->atEnd())
		{
			QByteArray line = in->readLine().trimmed();

			//skip empty lines
			if(line.isEmpty()) continue;

			if (line.startsWith(">")) // init line
			{
				updatePreviousHeader();
				crypt.reset();
				md5_checksum_pos = -1;

				QByteArray header = line.mid(1).trimmed();
				writeHeader(header);
			}
			else
			{
				crypt.addData(line);
				out->write(line);
				out->write("\n");
			}
		}
		updatePreviousHeader();
	}
private:
	QSharedPointer<QFile> in;
	QSharedPointer<QFile> out;
	QTextStream debug_stream;

	QCryptographicHash crypt;

	qint64 md5_checksum_pos = -1;
	QByteArray stored_checksum;
	bool debug;

	void updatePreviousHeader()
	{
		if (md5_checksum_pos == -1) return;

		QByteArray checksum = crypt.result().toHex();

		if (debug)
		{
			debug_stream << "original checksum: " << stored_checksum << Qt::endl;
			debug_stream << "calculated checksum: " << checksum << Qt::endl;
		}
		if (checksum != stored_checksum)
		{
			if (debug) debug_stream << "checksum mismatch! Rewriting." << Qt::endl;

			out->seek(md5_checksum_pos); //seek back to write the correct value
			out->write(checksum);
			out->seek(out->size()); //seek to the end
		}
	}

	void writeHeader(const QByteArray& header)
	{
		out->write(">");
		auto header_list = header.split(' ');
		for (int i =0; i < header_list.count(); ++i)
		{
			if (header_list[i].size() > 3 && header_list[i].left(3) == "M5:") //checksum column
			{
				md5_checksum_pos = out->pos() + 3; //store checksum write pos
				stored_checksum = header_list[i].mid(3);
			}
			out->write(header_list[i]);
			out->write((i == (header_list.count() - 1)) ? "\n" : " ");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
