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
		setDescription("Merges several VCF files into one VCF by appending one to the other.");
		setExtendedDescription(QStringList() << "Variant lines from all other input files are appended to the first input file." << "VCF header lines are taken from the first input file only.");
		addInfileList("in", "Input VCF files to merge.", false);

		//optional
		addOutfile("out", "Output VCF file with all variants.", true);
		addString("filter", "Tag variants from all but the first input file with this filter entry.", true);
		addString("filter_desc", "Description used in the filter header - use underscore instead of spaces.", true);
		addFlag("skip_duplicates", "Skip variants if they occur more than once.");

		//TODO Marc: add gzip support > remove VcfMerge in ngs-bits and megSAP
		changeLog(2022, 12,  8, "Initial implementation.");
	}

	virtual void main()
	{
		// init
		QStringList in_files = getInfileList("in");
		QString out = getOutfile("out");
		foreach(QString in, in_files)
		{
			if(in==out)
			{
				THROW(ArgumentException, "Input and output files must be different!");
			}
		}
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
		QByteArray filter = getString("filter").toUtf8();
		QByteArray filter_desc = getString("filter_desc").toUtf8();
		bool filter_used = !filter.isEmpty();
		bool skip_duplicates = getFlag("skip_duplicates");

		//variables to store infos from 'in'
		int column_count = -1;
		QSet<QByteArray> filters_defined;
		QSet<QByteArray> vars;
		
		//counts
		int c_written = 0;
		int c_dup = 0;
		int c_filter = 0;

		//copy in to out
		for (int i=0; i<in_files.count();++i)
		{
			QSharedPointer<QFile> in_p = Helper::openFileForReading(in_files[i], true);
			while (!in_p->atEnd())
			{
				bool is_first = (i==0);
				QByteArray line = in_p->readLine();
				while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

				//skip empty lines
				if (line.isEmpty()) continue;

				QByteArrayList parts = line.split('\t');

				//header lines
				if (line[0]=='#')
				{
					if (is_first)
					{
						//store defined filters
						if (line.startsWith("##FILTER=<ID="))
						{
							QByteArray tmp = line.mid(13);
							filters_defined << tmp.left(tmp.indexOf(','));
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
						out_p->write(line);
						out_p->write("\n");
					}
					else
					{
						if (!line.startsWith("##"))
						{
							if (parts.count()!=column_count) THROW(ArgumentException, "VCF files with differing column count cannot be combined! First file has " + QString::number(column_count) + " columns, but second as " + QString::number(parts.count()) + " columns!");
						}
						continue;
					}

					continue;
				}

				//content lines
				if (skip_duplicates)
				{
					QByteArray tag = parts[VcfFile::CHROM] + '\t' + parts[VcfFile::POS] + '\t' + parts[VcfFile::REF] + '\t' + parts[VcfFile::ALT];
					if (vars.contains(tag))
					{
						++c_dup;
						continue;
					}
					vars << tag;
				}


				//add filter entries
				if (!is_first && filter_used)
				{
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
					++c_filter;
				}

				++c_written;
				out_p->write(line);
				out_p->write("\n");
			}
			in_p->close();
		}

		//Statistics output
		QTextStream stream(stdout);
		stream << "Variants written: " << c_written << endl;
		if (filter_used)
		{
			stream << "Filter entries added to variants: " << c_filter << endl;
		}
		if (skip_duplicates)
		{
			stream << "Duplicate variants skipped: " << c_dup  << endl;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
