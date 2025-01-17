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
		addInfileList("in", "Input VCF ro VCG.GZ files to merge.", false);

		//optional
		addOutfile("out", "Output VCF file with all variants.", true);
		addString("filter", "Tag variants from all but the first input file with this filter entry.", true);
		addString("filter_desc", "Description used in the filter header - use underscore instead of spaces.", true);
		addFlag("skip_duplicates", "Skip variants if they occur more than once.");

		changeLog(2025,  1, 17, "Added support for gzipped VCFs and removing duplicates if there is only one input file.");
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

		//variables to store infos
		int column_count = -1;
		QSet<QByteArray> filters_defined;
		QSet<QByteArray> vars;
		bool is_first = true;
		const int buffer_size = 1048576; //1MB buffer
		char* buffer = new char[buffer_size];
		
		//counts
		int c_written = 0;
		int c_dup = 0;
		int c_filter = 0;

		//copy in to out
		foreach(QString in, in_files)
		{
			FILE* instream = fopen(in.toUtf8().data(), "rb");
			if (instream==nullptr) THROW(FileAccessException, "Could not open file '" + in + "' for reading!");
			gzFile file = gzdopen(fileno(instream), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
			if (file==nullptr) THROW(FileAccessException, "Could not open file '" + in + "' for reading!");

			while(!gzeof(file))
			{
				char* char_array = gzgets(file, buffer, buffer_size);
				//handle errors like truncated GZ file
				if (char_array==nullptr)
				{
					int error_no = Z_OK;
					QByteArray error_message = gzerror(file, &error_no);
					if (error_no!=Z_OK && error_no!=Z_STREAM_END)
					{
						THROW(FileParseException, "Error while reading file '" + in + "': " + error_message);
					}

					continue;
				}

				//determine end of read line
				int i=0;
				while(i<buffer_size && char_array[i]!='\0' && char_array[i]!='\n' && char_array[i]!='\r')
				{
					++i;
				}

				QByteArray line = QByteArray::fromRawData(char_array, i);
				while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

				//skip empty lines
				if (line.isEmpty()) continue;

				//split line to tab-separated parts if we need it
				QByteArrayList parts;
				if(skip_duplicates || filter_used || (line[0]=='#' && !line.startsWith("##"))) parts = line.split('\t');

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
							column_count = parts.count();

							//add filter header if missing
							if (filter_used && !filters_defined.contains(filter))
							{
								out_p->write("##FILTER=<ID="+filter+",Description=\""+filter_desc.replace("_", " ")+"\">\n");
							}
						}
						out_p->write(line);
						out_p->write("\n");
					}
					else if (!line.startsWith("##")) //check number of columns matches in all other files
					{
						if (parts.count()!=column_count) THROW(ArgumentException, "VCF files with differing column count cannot be combined! First file has " + QString::number(column_count) + " columns, but second as " + QString::number(parts.count()) + " columns!");
					}

					continue;
				}

				//skip duplicate variants
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
			gzclose(file);

			is_first = false;
		}

		//clean up
		out_p->close();
		delete[] buffer;

		//Statistics output
		QTextStream stream(stdout);
		stream << "Variants written: " << c_written << endl;
		if (skip_duplicates)
		{
			stream << "Duplicate variants skipped: " << c_dup  << endl;
		}
		if (filter_used)
		{
			stream << "Filter entries added to variants: " << c_filter << endl;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
