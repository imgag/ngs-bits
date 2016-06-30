#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "VariantList.h"
#include "Settings.h"
#include <QFile>
#include <QTextStream>
#include <QList>

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
		setDescription("Sort entries of a VCF file according to genomic position using a stream. Variants must be grouped by chromosome!");
		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);
		addInt("n", "Number of variants to cache for sorting.", true, 10000);

		changeLog(2016, 06, 27, "Initial implementation.");
	}

	virtual void main()
	{
		//init
		QMap<int, QByteArray> lines;
		int n = getInt("n");

		QByteArray last_chr;
		int last_pos_written = -1;
		QSet<QByteArray> chr_done;

		//open input/output streams
		QString in = getInfile("in");
		QString out = getOutfile("out");
		if(in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);

		while(!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();
			if (!line.endsWith('\n')) line += '\n';

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//write out headers/comments unchanged
			if (line.startsWith('#'))
			{
				out_p->write(line);
				continue;
			}

			int tab1 = line.indexOf('\t');
			int tab2 = line.indexOf('\t', tab1+1);

			QByteArray chr = line.left(tab1);
			int pos = Helper::toInt(line.mid(tab1, tab2-tab1), "chromosomal position");

			//handle new chromosome
			if (chr!=last_chr)
			{
				//flush lines
				while (!lines.isEmpty())
				{
					out_p->write(lines.take(lines.firstKey()));
				}
				last_pos_written = -1;

				//check chromosome done
				if (chr_done.contains(chr))
				{
					THROW(FileParseException, "Variants in input are not grouped according to chromosome. Found chromosome '" + chr + "' twice!");
				}

				//mark chromosome done
				chr_done.insert(chr);

				//set last chromosome
				last_chr = chr;
			}

			//check position
			if (pos<last_pos_written)
			{
				THROW(FileParseException, "Variants in input could not be sorted with given 'n' parameter! Positions " + QString::number(pos) + " and " + QString::number(last_pos_written) + " are too far apart on chromosome '" + chr + "'!");
			}

			//insert line
			lines.insertMulti(pos, line);

			//write overflow lines
			if (lines.count()>n)
			{
				int key = lines.firstKey();
				out_p->write(lines.take(key));
				last_pos_written = key;
			}
		}

		//flush lines
		while (!lines.isEmpty())
		{
			out_p->write(lines.take(lines.firstKey()));
		}

    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

