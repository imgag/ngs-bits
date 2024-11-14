#include "Exceptions.h"
#include "ToolBase.h"
#include "VcfFile.h"
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
		setDescription("Extract one or several samples from a VCF file. Can also be used to re-order sample columns.");

		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);
		addString("samples", "Comma-separated list of samples to extract (in the given order).", false);

		changeLog(2018, 11, 27, "Initial implementation.");
    }

    virtual void main()
	{
		//init
        QString in = getInfile("in");
        QString out = getOutfile("out");
		QByteArrayList samples = getString("samples").toUtf8().split(',');
        if(in!="" && in==out)
        {
            THROW(ArgumentException, "Input and output files must be different when streaming!");
        }
        QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
        QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);

		//always extract up to the FORMAT column
		QList<int> column_indices;
		for (int i=0; i<=VcfFile::FORMAT; ++i)
		{
			column_indices << i;
		}

		//process
        while (!in_p->atEnd())
        {
			QByteArray line = in_p->readLine();
			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//header and comment lines
            if (line.startsWith('#'))
			{
				if (line.startsWith("#CHROM"))
				{
					QByteArrayList parts = line.trimmed().split('\t');

					//check given sample names and extract column indices
					foreach(const QByteArray& sample, samples)
					{
						int index = parts.indexOf(sample, VcfFile::FORMAT+1);
						if (index==-1)
						{
							THROW(ArgumentException, "Cannot find sample '" + sample + "' in VCF header. Valid sample names are: '" + parts.mid(VcfFile::FORMAT+1).join("', '") + "'");
						}
						column_indices << index;
					}

					//write header out
					foreach(int col, column_indices)
					{
						if (col!=0)
						{
							out_p->write("\t");
						}
						out_p->write(parts[col]);
					}
					out_p->write("\n");
				}
				else //comment lines
				{
					out_p->write(line);
					out_p->write("\n");
				}
                continue;
            }

			//content lines
			QByteArrayList parts = line.trimmed().split('\t');
			foreach(int col, column_indices)
			{
				if (col!=0)
				{
					out_p->write("\t");
				}
				out_p->write(parts[col]);
			}
			out_p->write("\n");
        }

        // Close streams
        in_p->close();
        out_p->close();
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
