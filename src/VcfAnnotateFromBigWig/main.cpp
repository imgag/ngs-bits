#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "VcfFile.h"
#include "Settings.h"
#include <QFile>
#include <QList>
#include <BigWigReader.h>

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
		setDescription("Annotates the INFO column of a VCF with data from a bigWig file.");

		addInfile("in", "Input VCF file. If unset, reads from STDIN.", false, true);
		addOutfile("out", "Output VCF or VCF or VCF.GZ file. If unset, writes to STDOUT.", true, true);
		addInfile("bw", "BigWig file containen the data to be used in the annotation.", true, true);
		addString("name", "Name of the new INFO column.", false);
		addString("desc", "Optional description of the new INFO column.", false);
		//optional


		changeLog(2022, 01, 14, "Initial implementation.");
	}

	virtual void main()
	{

		//open input/output streams
		QString in = getInfile("in");
		QString out = getOutfile("out");
		QString bw_path = getInfile("bw");
		QString name = getString("name");

		QString desc = getString("desc");

		if(in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}

		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
		BigWigReader bw = BigWigReader(bw_path);
		bool writtenInfo = false;
		while(!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//write out headers:
			if (line.startsWith('#'))
			{
				 // add the new INFO header line at the top of the other ones.
				if (! writtenInfo && line.startsWith("##INFO"))
				{
					writtenInfo = true;
					out_p->write(QString("##INFO=<ID=" + name + ",Number=1,Type=Float,Description=\"" + desc + "\">\n").toStdString().data());
				}
				out_p->write(line);
				continue;
			}

			//split line and extract variant infos
			QList<QByteArray> parts = line.split('\t');
			if (parts.count()<8) THROW(FileParseException, "VCF with too few columns: " + line);

			Chromosome chr = parts[0];
			int pos_start = atoi(parts[1]);

			Sequence ref = parts[3].toUpper();
			Sequence alt = parts[4].toUpper();

			float value = bw.reproduceVepPhylopAnnotation(chr.str(), pos_start, pos_start + ref.length() - 1, QString(ref.data()), QString(alt.data()));
			parts[7].append(QString(";%1=%2").arg(name, QString::number(value))); // append key value pair to INFO column
			char tab = '\t';
			// write the first 7 columns unchanged
			for (int i=0; i<parts.length(); i++)
			{
				out_p->write(parts[i]);

				if (i!=parts.length()-1)
				{
					out_p->write(&tab, 1);
				}
			}
		}

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

