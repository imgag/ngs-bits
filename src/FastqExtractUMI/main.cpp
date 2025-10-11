#include "ToolBase.h"
#include "FastqFileStream.h"
#include "Log.h"

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
		setDescription("Cuts UMI bases from the beginning of reads and adds them to read headers.");
		addInfile("in1", "Input FASTQ file 1.", false);
		addInfile("in2", "Input FASTQ file 2.", false);
		addOutfile("out1", "Output filename for read 1 FASTQ.", false);
		addOutfile("out2", "Output filename for read 2 FASTQ.", false);
		//optional
		addInt("cut1", "Number of bases from the head of read 1 to use as UMI.", true, 0);
		addInt("cut2", "Number of bases from the head of read 2 to use as UMI.", true, 0);
		addInt("compression_level", "Output FASTQ compression level from 1 (fastest) to 9 (best compression).", true, Z_BEST_SPEED);

		changeLog(2020, 7, 15, "Added 'compression_level' parameter.");
	}

	virtual void main()
	{
		//init
		QString in1 = getInfile("in1");
		QString in2 = getInfile("in2");
		QString out1 = getOutfile("out1");
		QString out2 = getOutfile("out2");
		int cut1 = getInt("cut1");
		int cut2 = getInt("cut2");

		FastqFileStream input_stream1(in1, false);
		FastqFileStream input_stream2(in2, false);

		int compression_level = getInt("compression_level");
		FastqOutfileStream outstream1(out1, compression_level);
		FastqOutfileStream outstream2(out2, compression_level);

		while (!input_stream1.atEnd() && !input_stream2.atEnd())
		{
			FastqEntry read1;
			FastqEntry read2;
			input_stream1.readEntry(read1);
			input_stream2.readEntry(read2);

			QByteArray barcode1(read1.bases, cut1);
			QByteArray barcode2(read2.bases, cut2);
			QByteArray barcode_header = ":" + QByteArray::number(cut1) + "," + QByteArray::number(cut2) + ":" + barcode1 + "," + barcode2;
			QList<QByteArray> parts;

			parts = read1.header.split(' ');
			parts[0] += barcode_header;
			read1.header = parts.join(' ');
			read1.bases.remove(0, cut1);
			read1.qualities.remove(0, cut1);

			parts = read2.header.split(' ');
			parts[0] += barcode_header;
			read2.header = parts.join(' ');
			read2.bases.remove(0, cut2);
			read2.qualities.remove(0, cut2);

			outstream1.write(read1);
			outstream2.write(read2);
		}
		outstream1.close();
		outstream2.close();
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
