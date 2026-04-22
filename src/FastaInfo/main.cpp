#include "ToolBase.h"
#include "Helper.h"
#include "BedFile.h"
#include <QTextStream>

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
		setDescription("Basic info on a FASTA file containing DNA sequences.");

		//optional
		addInfile("in", "Input FASTA file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addOutfile("write_n", "Write BED file with N base coordinates", true);
		addOutfile("write_other", "Write BED file with other base coordinates", true);

		//changelog
		changeLog(2024,  8, 16, "Added optional parameters 'write_n' and 'write_other'.");
		changeLog(2015,  6, 25, "initial version");
	}

	virtual void main()
	{
		//init
		struct Counts
		{
			int acgt = 0;
			int n = 0;
			int other = 0;

			int all() const { return acgt+n+other; }
		};
		QHash<QByteArray, Counts> counts;
		QList<QByteArray> order;
		BedFile bed_n;
		bool write_n = !getOutfile("write_n").isEmpty();
		BedFile bed_other;
		bool write_other = !getOutfile("write_other").isEmpty();

		//parse from file
		QByteArray current_chr = "";
		QSharedPointer<QFile> file = Helper::openFileForReading(getInfile("in"), true);
		int pos = 0;
		while(!file->atEnd())
		{
			QByteArray line = file->readLine().trimmed();

			//skip empty lines
			if(line.isEmpty()) continue;

			if (line.startsWith(">"))
			{
				current_chr = line.mid(1);
				int space_idx = current_chr.indexOf(' ');
				if (space_idx!=-1) current_chr = current_chr.left(space_idx);

				order << current_chr;
				counts[current_chr] = Counts();
				pos = 0;
			}
			else
			{
				Counts& count_data = counts[current_chr];
				line = line.toUpper();
				foreach(const char& c, line)
				{
					++pos;
					switch(c)
					{
						case 'A':
						case 'C':
						case 'G':
						case 'T':
							++count_data.acgt;
							break;
						case 'N':
							++count_data.n;
							if (write_n)
							{
								if (bed_n.count()>0)
								{
									const BedLine& last = bed_n.last();
									if  (last.chr().str()==current_chr && last.end()==pos-1)
									{
										bed_n[bed_n.count()-1].setEnd(pos);
									}
									else
									{
										bed_n.append(BedLine(current_chr, pos, pos));
									}
								}
								else
								{
									bed_n.append(BedLine(current_chr, pos, pos));
								}
							}
							break;
						default:
							++count_data.other;
							if (write_other) bed_other.append(BedLine(current_chr, pos, pos));
					}
				}
			}
		}

		//output summary
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream stream(outfile.data());
        stream << "== general info ==" << Qt::endl;
        stream << "sequences : " << counts.count() << Qt::endl;
		long long sum = 0;
		foreach(const Counts& counts, counts)
		{
			sum += counts.all();
		}
        stream << "characters: " << sum << Qt::endl;
        stream << Qt::endl;

		//output details
        stream << "== characters per sequence ==" << Qt::endl;
		foreach(const QByteArray& sequence, order)
		{
			const Counts& c = counts[sequence];
            stream << sequence << ": " << c.all() << " (ACGT:" << c.acgt << " N:" << c.n << " other:" << c.other << ")" << Qt::endl;
		}

		//write BED files
		if (write_n)
		{
			bed_n.merge();
			bed_n.store(getOutfile("write_n"));
		}
		if (write_other)
		{
			bed_other.merge();
			bed_other.store(getOutfile("write_other"));
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
