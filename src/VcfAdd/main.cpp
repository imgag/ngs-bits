#include "ToolBase.h"
#include "Settings.h"
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
		setDescription("Appends variants from a VCF file to another VCF file.");
		setExtendedDescription(QStringList() << "VCF header lines are taken from 'in' only.");
		addInfile("in2", "Input VCF file that is added to 'in'.", false);

		//optional
		addInfile("in", "Input VCF file to add 'in2' to.", true);
		addOutfile("out", "Output VCF file with variants from 'in' and 'in2'.", true);
		addString("filter", "Tag variants from 'in2' with this filter entry.", true);
		addString("filter_desc", "Description used in the filter header - use underscore instead of spaces.", true);
		addFlag("skip_duplicates", "Skip variants from  'in2' which are also contained in 'in'.");

		changeLog(2022, 12,  8, "Initial implementation.");
	}

	virtual void main()
	{
		// init
		QString in = getInfile("in");
		QString in2 = getInfile("in2");
		QString out = getOutfile("out");
		QByteArray filter = getString("filter").toUtf8();
		QByteArray filter_desc = getString("filter_desc").toUtf8();
		bool filter_used = !filter.isEmpty();
		bool skip_duplicates = getFlag("skip_duplicates");

		//variables to store infos from 'in'
		int column_count = -1;
		QSet<QByteArray> filters_defined;
		QSet<QByteArray> vars;

		//copy in to out
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
				//store defined filters
				if (line.startsWith("##FILTER=<ID="))
				{
					QByteArray tmp = line.mid(13);
					filters_defined << tmp.left(tmp.indexOf(','));
					QTextStream(stdout) << tmp.left(tmp.indexOf(',')) << endl;
				}

				if (!line.startsWith("##"))
				{
					//store column count
					column_count = line.split('\t').count();

					//add filter header if missing
					if (filter_used && !filters_defined.contains(filter))
					{
						out_p->write("##FILTER=<ID="+filter+",Description=\""+filter_desc.replace("_", " ")+"\">\n");
					}
				}
			}
			else if (skip_duplicates)
			{
				QByteArrayList parts = line.split('\t');
				QByteArray tag = parts[VcfFile::CHROM] + '\t' + parts[VcfFile::POS] + '\t' + parts[VcfFile::REF] + '\t' + parts[VcfFile::ALT];
				vars << tag;
			}

			out_p->write(line);
			out_p->write("\n");
		}
		in_p->close();

		//copy in2 to out
		in_p = Helper::openFileForReading(in2, false);
		while (!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();
			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

			//skip empty lines
			if (line.isEmpty()) continue;

			//skip header lines
			if (line[0]=='#')
			{
				//add filter header if missing
				if (!line.startsWith("##"))
				{
					QByteArrayList parts = line.split('\t');
					if (parts.count()!=column_count) THROW(ArgumentException, "VCF files with differing column count cannot be combined! First file has " + QString::number(column_count) + " columns, but second as " + QString::number(parts.count()) + " columns!");
				}

				continue;
			}

			//skip duplicates
			QByteArrayList parts;
			if (skip_duplicates)
			{
				parts = line.split('\t');
				QByteArray tag = parts[VcfFile::CHROM] + '\t' + parts[VcfFile::POS] + '\t' + parts[VcfFile::REF] + '\t' + parts[VcfFile::ALT];
				if (vars.contains(tag)) continue;
			}

			//add filter entries
			if (filter_used)
			{
				if (parts.isEmpty()) parts = line.split('\t');
				QByteArray filter_str = parts[VcfFile::FILTER];
				if (filter_str=="PASS" || filter_str==".")
				{
					parts[VcfFile::FILTER] = filter;
				}
				else
				{
					parts[VcfFile::FILTER] = filter_str + ";" + filter;
				}
				line = parts.join('\t');
			}

			out_p->write(line);
			out_p->write("\n");
		}
		in_p->close();
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
