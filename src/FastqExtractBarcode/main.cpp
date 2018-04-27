#include "ToolBase.h"
#include "Helper.h"
#include "FastqFileStream.h"
#include "NGSHelper.h"
#include "Log.h"
#include <QFile>
#include <QList>
#include <QDir>

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
		setDescription("Cuts bases from the beginning of reads and stores them in an additional fastq.");
		addInfile("in", "input fastq file1.", false);
		addInt("cut", "number of bases from the beginning of reads to use as barcodes.", true, 0);
		addString("out_main","output filename for main fastq.", false);
		addString("out_index","output filename for index fastq.", true, "index.fastq.gz");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out_main = getString("out_main");
		QString out_index = getString("out_index");
		int cut = getInt("cut");
		FastqFileStream input_stream(in, false);

		FastqOutfileStream outstream_main(out_main, false);
		FastqOutfileStream outsrtream_index(out_index, false);

		while (!input_stream.atEnd())//foreach input read
		{
			FastqEntry read1;
			input_stream.readEntry(read1);
			FastqEntry read2=read1;

			QByteArray barcode(read1.bases,cut);
			QByteArray barcode_qual(read1.qualities,cut);

			read1.bases.remove(0,cut);
			read1.qualities.remove(0,cut);
			read2.bases=barcode;
			read2.qualities=barcode_qual;

			outstream_main.write(read1);
			outsrtream_index.write(read2);
		}
		outstream_main.close();
		outsrtream_index.close();
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
