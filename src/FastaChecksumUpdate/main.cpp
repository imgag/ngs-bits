#include "ToolBase.h"
#include "Helper.h"
#include <QCryptographicHash>

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv), debug_stream_(stdout), crypt_(QCryptographicHash::Md5)
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

		in_ = Helper::openFileForReading(getInfile("in"));
		out_ = Helper::openFileForWriting(getOutfile("out"));
		debug_ = getFlag("debug");

		while(!in_->atEnd())
		{
			QByteArray line = in_->readLine().trimmed();

			//skip empty lines
			if(line.isEmpty()) continue;

			if (line.startsWith(">")) // init line
			{
				updatePreviousHeader();
				crypt_.reset();
				md5_checksum_pos_ = -1;

				QByteArray header = line.mid(1).trimmed();
				writeHeader(header);
			}
			else
			{
				crypt_.addData(line);
				out_->write(line);
				out_->write("\n");
			}
		}
		updatePreviousHeader();
	}
private:
	QSharedPointer<QFile> in_;
	QSharedPointer<QFile> out_;
	QTextStream debug_stream_;

	QCryptographicHash crypt_;

	qint64 md5_checksum_pos_ = -1;
	QByteArray stored_checksum_;
	bool debug_;

	void updatePreviousHeader()
	{
		if (md5_checksum_pos_ == -1) return;

		QByteArray checksum = crypt_.result().toHex();

		if (debug_)
		{
			debug_stream_ << "original checksum: " << stored_checksum_ << Qt::endl;
			debug_stream_ << "calculated checksum: " << checksum << Qt::endl;
		}
		if (checksum != stored_checksum_)
		{
			if (debug_) debug_stream_ << "checksum mismatch! Rewriting." << Qt::endl;

			out_->seek(md5_checksum_pos_); //seek back to write the correct value
			out_->write(checksum);
			out_->seek(out_->size()); //seek to the end
		}
	}

	void writeHeader(const QByteArray& header)
	{
		out_->write(">");
		auto header_list = header.split(' ');
		for (int i =0; i < header_list.count(); ++i)
		{
			if (header_list[i].size() > 3 && header_list[i].left(3) == "M5:") //checksum column
			{
				md5_checksum_pos_ = out_->pos() + 3; //store checksum write pos
				stored_checksum_ = header_list[i].mid(3);
			}
			out_->write(header_list[i]);
			out_->write((i == (header_list.count() - 1)) ? "\n" : " ");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	qputenv("QT_QPA_PLATFORM", "offscreen");
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
