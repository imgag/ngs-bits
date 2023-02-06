#include "ToolBase.h"
#include "Helper.h"

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
		setDescription("Substracts the variants of 'in2' VCF from the 'in' VCF.");
		addInfile("in2", "Variants in VCF format that are remove from 'in'", false);

		//optional
		addInfile("in", "Input VCF file from which the variants of 'in2' are substracted.", true);
		addOutfile("out", "Output VCF file with variants from 'in2' removed from 'in'.", true);

		changeLog(2023, 2,  6, "Initial implementation.");
	}

	virtual void main()
	{
		// init
		QString in = getInfile("in");
		QString in2 = getInfile("in2");
		QString out = getOutfile("out");
		if(in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}

		//load blacklist:
		QSet<QByteArray> blacklist_hash;
		QSharedPointer<QFile> in2_p = Helper::openFileForReading(in2, true);
		while (!in2_p->atEnd())
		{
			QByteArray line = in2_p->readLine();
			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

			//skip empty lines and headers
			if (line.isEmpty() || line[0]=='#') continue;

			blacklist_hash.insert(variant_key(line));
		}

		int removed_count = 0;

		//remove blacklist from in:
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
		while (!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();
			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

			//skip empty lines
			if (line.isEmpty()) continue;

			//headers
			if (line[0]=='#')
			{
				out_p->write(line);
				out_p->write("\n");
				continue;
			}

			if (blacklist_hash.contains(variant_key(line)))
			{
				removed_count++;
				continue;
			}

			out_p->write(line);
			out_p->write("\n");
		}
		in_p->close();
		out_p->close();

		//Statistics output
		QTextStream stream(stdout);
		stream << "Variants from in removed: " << removed_count << endl;
	}

private:

	QByteArray variant_key(const QByteArray& vcf_line)
	{
		QByteArrayList parts = vcf_line.split('\t');
		return parts[0] + ":" + parts[1] + " " + parts[3] + ">" + parts[4];
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
