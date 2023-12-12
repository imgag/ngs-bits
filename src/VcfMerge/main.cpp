#include "ToolBase.h"
#include "Helper.h"

class ConcreteTool: public ToolBase
{
	Q_OBJECT
public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)

	{
	}

	virtual void setup()
	{
		setDescription("Merges several VCF files into one VCF");
		addInfileList("in", "Input VCF files that are merged. The VCF header is taken from the first file.", false, true);
		//optional
		addOutfile("out", "Output VCF. If unset, writes to STDOUT.", true, true);

		changeLog(2023, 12, 12, "Initial implementation.");
	}

	virtual void main()
	{
		//open input/output streams
		QStringList ins = getInfileList("in");
		QString out = getOutfile("out");
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);

		bool is_first_vcf = true;
		foreach(QString in, ins)
		{
			QSharedPointer<QFile> in_p = Helper::openFileForReading(in, false);
			while (!in_p->atEnd())
			{
				QByteArray line = in_p->readLine();
				while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

				//skip empty lines
				if (line.trimmed().isEmpty()) continue;

				//header rows
				if (line.startsWith('#'))
				{
					if (is_first_vcf) out_p->write(line + '\n');
					continue;
				}

				//variant rows
				out_p->write(line + '\n');
			}
			in_p->close();

			is_first_vcf = false;
		}
		out_p->close();
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
