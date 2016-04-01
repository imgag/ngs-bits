#include "ToolBase.h"
#include "Helper.h"
#include "FastqFileStream.h"
#include "NGSHelper.h"
#include "Log.h"
#include <QFile>
#include <QDebug>
#include <QList>
#include <QDir>

//TODO: Find more suitable name for current tool

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
		addInfile("mip_file","input file for moleculare inversion probes (reads are trimmed to minimum MIP size to avoid readthrough).", true, "");
	}

	int get_min_mip(QString mip_file)
	{
		QFile input_file(mip_file);
		input_file.open(QIODevice::ReadOnly);
		QTextStream in(&input_file);
		QRegExp delimiters("(\\-|\\:|\\/)");
		int min_mip=std::numeric_limits<int>::max();
		while (!in.atEnd())
		{
			QString line = in.readLine();
			if (line.startsWith("<")) continue;
			QStringList splitted_mip_entry=line.split(delimiters);
			if (splitted_mip_entry.size()<3) continue;
			int mip_start=splitted_mip_entry[1].toInt();
			int mip_end=splitted_mip_entry[2].toInt();
			min_mip=qMin(min_mip,qAbs(mip_start-mip_end));
		}
		input_file.close();
		return min_mip;
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

		QString mip_file=getInfile("mip_file");
		int min_mip=std::numeric_limits<int>::max();
		if (mip_file!="") min_mip=get_min_mip(mip_file);

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
