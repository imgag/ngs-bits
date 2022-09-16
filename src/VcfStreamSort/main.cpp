#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "VariantList.h"
#include "Settings.h"
#include "VcfFile.h"

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

		changeLog(2019,  1,  8, "Added REF, ALT and INFO fields to sorting for a defined output order.");
		changeLog(2016,  6, 27, "Initial implementation.");
	}

	//Datastructure for VCF lines
	struct VcfCoords
	{
		int pos;
		QByteArray ref;
		QByteArray alt;
		QByteArray info;

		bool operator<(const VcfCoords& rhs) const
		{
			if (pos<rhs.pos) return true;
			if (pos>rhs.pos) return false;
			if (ref<rhs.ref) return true;
			if (ref>rhs.ref) return false;
			if (alt<rhs.alt) return true;
			if (alt>rhs.alt) return false;
			return info<rhs.info;
		}
	};

	//Write all lines and clear datastructure.
	static void flush(QMap<VcfCoords, QByteArray>& lines, QSharedPointer<QFile>& out_p)
	{
		auto it = lines.begin();
		while (it!=lines.end())
		{
			out_p->write(it.value());
			++it;
		}

		lines.clear();
	}

	virtual void main()
	{
		//init
		QMap<VcfCoords, QByteArray> lines;
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

			QByteArrayList parts = line.split('\t');
			if (parts.count()<VcfFile::MIN_COLS)
			{
				THROW(FileParseException, "VCF line with less than 8 fields found: '" + line.trimmed() + "'");
			}

			QByteArray chr = parts[0];
			int pos = Helper::toInt(parts[1], "chromosomal position");

			//handle new chromosome
			if (chr!=last_chr)
			{
				//flush lines
				flush(lines, out_p);

				//reset pos
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
			lines.insertMulti(VcfCoords{pos, parts[3], parts[4], parts[7]}, line);

			//write overflow lines
			if (lines.count()>n)
			{
				auto it = lines.begin();
				out_p->write(it.value());
				last_pos_written = it.key().pos;
				lines.erase(it);
			}
		}

		//flush lines
		flush(lines, out_p);
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

