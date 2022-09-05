#include "ToolBase.h"
#include "TSVFileStream.h"
#include "Helper.h"
#include "BasicStatistics.h"
#include <QFileInfo>
#include <QBitArray>

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
		setDescription("Filters the rows of a TSV file according to the value of a specific column.");
		addString("filter", "Filter string with column name, operation and value,e.g. 'depth > 17'.\nValid operations are '" + ops.join("','") + "'.", false);
		//optional
		addInfile("in", "Input TSV file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addFlag("numeric", "If set, column name is interpreted as a 1-based column number.");
		addFlag("v", "Invert filter.");
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
		TSVFileStream instream(in);
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(out, true);
		bool v = getFlag("v");

		//split filter
		QString filter = getString("filter");
		QStringList parts = filter.split(" ");
		if (parts.count()<3)
		{
			THROW(CommandLineParsingException, "Could not split filter '" + filter + "' in three or more parts (by space)!");
		}
		//re-join string values with spaces
		while(parts.count()>3)
		{
			int count = parts.count();
			parts[count-2] += " " +parts[count-1];
			parts.removeLast();
		}

		//check column
		QVector<int> cols = instream.checkColumns(parts[0].toUtf8().split(','), getFlag("numeric"));
		if (cols.count()!=1)
		{
			THROW(CommandLineParsingException, "Could not determine column name/index '" + parts[0] + "'!");
		}
		int col = cols[0];

		//check operation
		QByteArray op = parts[1].toUtf8();
		int op_index = ops.indexOf(op);
		if(op_index==-1)
		{
			THROW(CommandLineParsingException, "Invalid operation '" + op + "'!");
		}

		//check value
		QByteArray value = parts[2].toUtf8();
		double value_num = 0;
		if (op_index<5)
		{
			if(!BasicStatistics::isValidFloat(value))
			{
				THROW(CommandLineParsingException, "Non-numeric filter value '" + value + "' for numeric filter operation '" + op + " given!");
			}
			value_num = value.toDouble();
		}

		//write comments
		foreach (QByteArray comment, instream.comments())
		{
			outstream->write(comment);
			outstream->write("\n");
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

