#include "ToolBase.h"
#include "Helper.h"
#include "FastqFileStream.h"
#include "Log.h"
#include <QRegularExpression>

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
		setDescription("Returns the UMI info of a FastQ file on STDOUT.");
		addInfile("in", "Input FASTQ file.", false);

		//optional
		addOutfile("out", "Output file containing the result string. If unset, writes to STDOUT.", true);
		addInt("lines", "Number of lines which should be checked. (default: 10)", true, 10);

		changeLog(2023, 10, 5, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		int max_lines = getInt("lines");
		if(max_lines < 1) THROW(ArgumentException, "Number of lines has to be greater than zero!");
		FastqFileStream input_stream(in, false);
		int n = 0;
		QSet<QByteArray> barcode_info;

		while (!input_stream.atEnd() && (n < max_lines))
		{
			FastqEntry read;
			input_stream.readEntry(read);
			n++;

			//if barcode is present it is the last header entry
			QByteArrayList barcodes = read.header.split(' ').at(0).split(':').last().split(',');
			QList<int> barcode_lengths;
			bool valid_barcode = true;
			foreach(const QByteArray& barcode, barcodes)
			{
				int length;
				if(!isValidSequence(barcode, length))
				{
					valid_barcode = false;
					break;
				}
				else
				{
					barcode_lengths << length;
				}
			}

			if(valid_barcode)
			{
				QByteArrayList lengths;
				foreach(int i, barcode_lengths) lengths.append(QByteArray::number(i));
				barcode_info.insert("UMI: true\tlength: " + lengths.join(','));
			}
			else
			{
				barcode_info.insert("UMI: false\tlength: n/a");
			}

		}

        if(barcode_info.size() > 1) THROW(FileParseException, "ERROR: FastQ reads contain multiple UMI types!\n\t" + barcode_info.values().join("\n\t"));

		//output
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out(outfile.data());
        out << barcode_info.values().first() << QT_ENDL;
	}

	bool isValidSequence(QByteArray barcode, int& length)
	{
        QRegularExpression seq(QRegularExpression::anchoredPattern("[ATCGN]*"));
        barcode = barcode.trimmed().toUpper();
        length = barcode.length();
        if(seq.match(barcode).hasMatch()) return true;
        return false;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
