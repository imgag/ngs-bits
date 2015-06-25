#include "Exceptions.h"
#include "ToolBase.h"
#include "Helper.h"
#include <QFile>
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
		setDescription("Basic info on a FASTA file.");
		//optional
		addInfile("in", "Input FASTA file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
	}

	virtual void main()
	{
		//init
		QMap<QString, int> counts;

		//parse from stream
		QString last_head = "";
		QScopedPointer<QFile> file(Helper::openFileForReading(getInfile("in"), true));
		QTextStream stream(file.data());
		while(!stream.atEnd())
		{
			QString line = stream.readLine().trimmed();

			//skip empty lines
			if(line.count()==0) continue;

			if (line.startsWith(">"))
			{
				last_head = line.mid(1);
				counts[last_head] = 0;
			}
			else
			{
				counts[last_head] += line.count();
			}
		}

		//output summary
		QScopedPointer<QFile> outfile(Helper::openFileForWriting(getOutfile("out"), true));
		QTextStream stream2(outfile.data());
		stream2 << "== general info ==" << endl;
		stream2 << "sequences : " << counts.count() << endl;
		long sum = 0;
		foreach(int c, counts)
		{
			sum += c;
		}
		stream2 << "characters: " << sum << endl;
		stream2 << endl;

		//output details
		stream2 << "== characters per sequence ==" << endl;
		QMapIterator<QString,int> i(counts);
		while (i.hasNext())
		{
			i.next();
			stream2 << i.key() << ": " << i.value() << endl;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
