#include "ToolBase.h"
#include "TSVFileStream.h"
#include "Helper.h"
#include "BasicStatistics.h"
#include <QFileInfo>
#include <QBitArray>
#include <iomanip>
#include <ctime>


class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
		ops << ">" << ">=" << "=" << "<=" << "<" << "is" << "contains";
	}

	virtual void setup()
	{
		setDescription("Transforms a TSV file (col1: transcript ID; col 2: HGVS change ) into a VCF file.");
		QStringList extDescription;
		extDescription << "Transforms a given TSV file with the transcript ID in the first column and the HGVS change in the second column into a vcf file.";
		extDescription << "Any further columns are added as info headers with the column header as info ID.";
		setExtendedDescription(extDescription);
		//optional
		addInfile("in", "Input TSV file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addString("sep", "Separator in the input TSV file, default: \\t", true, "\t");
	}

	void writeVcfHeaders(QSharedPointer<QFile> outstream, QByteArrayList info_headers)
	{
		outstream << "##fileformat=VCFv4.2\n";
		auto t = std::time(nullptr);
		outstream << "##fileDate=" + std::put_time(std::localtime(&t), "%Y%m%d");  //TODO does this work
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");
		if(in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}

		//TODO parse in case the separator is actually \t instead of the tabulator.
		QString sep = getString("sep");


		TSVFileStream instream(in, sep);
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(out, true);

		QByteArrayList headers = instream.header();

		if (headers.size() < 2)
		{
			THROW(ArgumentException, "Input TSV file needs at least two columns: transcript ids, HGVS-change.")
		}


		//write header
		const int col_count = instream.header().count();
		outstream->write("#");
		for(int i=0; i<col_count; ++i)
		{
			outstream->write(instream.header()[i]);
			outstream->write(i==col_count-1 ? "\n" : "\t");
		}

		//write content
		while(!instream.atEnd())
		{
			QList<QByteArray> parts = instream.readLine();
			if (parts.count()==0) continue;

			QByteArray value2 = parts[col];
			double value2_num = 0;
			if (op_index<5)
			{
				bool ok = true;
				value2_num = value2.toDouble(&ok);
				if (!ok)
				{
					THROW(CommandLineParsingException, "Non-numeric value '" + value2 + "' for numeric filter operation '" + op + " in line " + QString::number(instream.lineIndex()+1) + "!");
				}
			}

			bool pass_filter = true;
			switch(op_index)
			{
				case 0: //">"
					pass_filter = value2_num>value_num;
					break;
				case 1: //">="
					pass_filter = value2_num>=value_num;
					break;
				case 2: //"="
					pass_filter = value2_num==value_num;
					break;
				case 3: //"<="
					pass_filter = value2_num<=value_num;
					break;
				case 4: //"<"
					pass_filter = value2_num<value_num;
					break;
				case 5: //"is"
					pass_filter = value2 == value;
					break;
				case 6: //"contains"
					pass_filter = value2.contains(value);
					break;
				default:
					THROW(ProgrammingException, "Invalid filter operation index " + QString::number(op_index) + "!");
			};

			if ((!v && !pass_filter) ||  (v && pass_filter))
			{
				continue;
			}

			for(int i=0; i<col_count; ++i)
			{
				outstream->write(parts[i]);
				outstream->write(i==col_count-1 ? "\n" : "\t");
			}
		}
    }

	//valid operations list
	QStringList ops;
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

