#include "ToolBase.h"
#include "Exceptions.h"
#include "FastqFileStream.h"
#include "Helper.h"
#include <QMap>
#include <QTextStream>

struct SampleSheetEntry
{
	QString name;
	QString mid;
};

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
		setDescription("Counts the number of occurances of each MID in a FASTQ file.");
		addInfile("in", "Input gzipped FASTQ file.", false);
		//operional
		addOutfile("out", "Output TXT file. If unset, writes to STDOUT.", true);
		addInt("lines", "The number of FASTQ entries in the input file to parse. 0 is unlimited.", true, 1000);
		addInt("mids", "The number of top-ranking MIDs to print. 0 is unlimited.", true, 20);
		addInfile("sheet", "Optional sample sheet CSV file as provided to CASAVA. If given, the closest match in the sample sheet is printed after each MID.", true);
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		int max_lines = getInt("lines");
		int max_mids = getInt("mids");

		//load sample sheet if given
		QVector<SampleSheetEntry> sheet;
		if (getInfile("sheet")!="")
		{
			QStringList lines = Helper::loadTextFile(getInfile("sheet"), true, QChar::Null, true);
			foreach(QString line, lines)
			{
				QStringList parts = line.split(",");
				SampleSheetEntry entry;
				entry.name = parts[2];
				entry.mid = parts[4];
				if (entry.name!="SampleID")
				{
					sheet.append(entry);
				}
			}
		}

		//parse
		QMap<QString, int> counts;
		int i=0;
		FastqFileStream stream(in, false);
		FastqEntry entry;
		while (!stream.atEnd())
		{
			stream.readEntry(entry);

			//split header line
			QList<QByteArray> parts = entry.header.split(':');
			if (parts.count()<10)
			{
				THROW(ArgumentException, "Line " + QString::number(i) + " of file " + in + " does not contain 10 :-separated parts!");
			}

			// count MID
			QString mid = parts[9];
			if (counts.contains(mid))
			{
				counts[mid] += 1;
			}
			else
			{
				counts[mid] = 1;
			}

			//abort if the requested number of lines is reached
			++i;
			if (max_lines!=0 && i>=max_lines) break;
		}

		//create a sorted and unique list of values in the map
		QList<int> values = Helper::setToList(Helper::listToSet(counts.values()));
		std::sort(values.begin(), values.end(), std::greater<int>());

		//print list ordered by counts
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out(outfile.data());
		int j =0;
		foreach(int value, values)
		{
			QList<QString> keys = counts.keys(value);
			foreach(QString key, keys)
			{
				if (sheet.count()==0)
				{
                    out << key << "\t" << value << Qt::endl;
				}
				else
				{
					QStringList min_dist_diff;
                    for (int j=0; j<key.size(); ++j) min_dist_diff.append("x");
					SampleSheetEntry min_dist_entry;
					foreach(const SampleSheetEntry& entry, sheet)
					{
						QStringList different;
                        for(int j=0; j<entry.mid.size(); ++j)
						{
							if (entry.mid[j]!=key[j]) different.append(QString::number(j));
						}
						if (different.count()<min_dist_diff.count())
						{
							min_dist_entry = entry;
							min_dist_diff  = different;
						}
					}

                    out << key << "\t" << value << "\t(nearest=" << min_dist_entry.mid << " name=" << min_dist_entry.name << " dist=" << min_dist_diff.count() << " diff_indices=" << min_dist_diff.join(',') << ")" << Qt::endl;
				}

				//abort when the maximum number of MIDs is reached
				++j;
				if (max_mids!=0 && j>=max_mids) break;
			}

			if (max_mids!=0 && j>=max_mids) break;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
