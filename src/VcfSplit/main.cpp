#include "ToolBase.h"
#include "Helper.h"
#include <QTextStream>

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
		setDescription("Splits a VCF into several chunks");
		addInt("lines", "Number of variant lines per chunk.", false);
		addString("out", "Output VCF base name. Suffixed with chunk number and extension, e.g. '0001.vcf'", false);
		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);

		changeLog(2023, 12, 10, "Initial implementation.");
	}

	void store(const QByteArrayList& header_lines, const QByteArrayList& variant_lines, QString out, int chunk_index)
	{
		QString filename = out + QString::number(chunk_index).rightJustified(4, '0') + ".vcf";
		QSharedPointer<QFile>  out_p = Helper::openFileForWriting(filename);
		QTextStream stream(out_p.data());
        #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        stream.setEncoding(QStringConverter::Utf8);
        #else
        stream.setCodec("UTF-8");
        #endif
		foreach(const QByteArray& line, header_lines)
		{
			stream << line;
		}
		foreach(const QByteArray& line, variant_lines)
		{
			stream << line;
		}
	}

	virtual void main()
	{
		//open input/output streams
		QString in = getInfile("in");
		QString out = getString("out");
		int lines = getInt("lines");
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);

		//process
		bool header = true;
		QByteArrayList header_lines;
		QByteArrayList variant_lines;
		int chunk_index = 1;
		while (!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//handle header columns
			if (header && line.startsWith('#'))
			{
				header_lines << line;
				continue;
			}

			header = false;
			variant_lines << line;
			if (variant_lines.count()>=lines)
			{
				store(header_lines, variant_lines, out, chunk_index);
				variant_lines.clear();
				++chunk_index;
			}
		}
		store(header_lines, variant_lines, out, chunk_index);

		//close streams
		in_p->close();
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
