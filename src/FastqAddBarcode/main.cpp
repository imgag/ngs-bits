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
		setDescription("Adds barcodes from separate FASTQ file to read headers.");
		addInfileList("in1", "Input FASTQ file 1.", false);
		addInfileList("in2", "Input FASTQ file 2.", false);
		addInfileList("in_barcode", "Input barcode file.", false);
		addOutfile("out1", "Output filename for read 1 FASTQ.", false);
		addOutfile("out2", "Output filename for read 2 FASTQ.", false);
		//optional
		addInt("compression_level", "Output FASTQ compression level from 1 (fastest) to 9 (best compression).", true, 1);

		changeLog(2020, 7, 15, "Added 'compression_level' parameter.");
	}

	virtual void main()
	{
		//output files
		QString out1 = getOutfile("out1");
		QString out2 = getOutfile("out2");
		int compression_level = getInt("compression_level");
		FastqOutfileStream outstream1(out1, compression_level);
		FastqOutfileStream outstream2(out2, compression_level);

		//input files
		QStringList in1 = getInfileList("in1");
		QStringList in2 = getInfileList("in2");
		QStringList in_barcode = getInfileList("in_barcode");

		for (int i=0; i<in1.count(); ++i)
		{
			FastqFileStream input_stream1(in1[i], false);
			FastqFileStream input_stream2(in2[i], false);
			FastqFileStream input_stream3(in_barcode[i], false);

			while (!input_stream1.atEnd() && !input_stream2.atEnd() && !input_stream3.atEnd())
			{
				FastqEntry read1;
				FastqEntry read2;
				FastqEntry read3;
				input_stream1.readEntry(read1);
				input_stream2.readEntry(read2);
				input_stream3.readEntry(read3);

				QByteArray barcode_header = ":" + QByteArray::number(read3.bases.length()) + ",0:" + read3.bases + ",";
				QList<QByteArray> parts;

				parts = read1.header.split(' ');
				parts[0] += barcode_header;
				read1.header = parts.join(' ');

				parts = read2.header.split(' ');
				parts[0] += barcode_header;
				read2.header = parts.join(' ');

				outstream1.write(read1);
				outstream2.write(read2);
			}
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
