#include "ToolBase.h"
#include "Helper.h"
#include <zlib.h>

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
		changeLog(2023, 12, 14, "Added support for gzipped input.");
	}

	virtual void main()
	{
		//open input/output streams
		QStringList ins = getInfileList("in");
		QString out = getOutfile("out");
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);

		//init
		const int buffer_size = 1048576; //1MB buffer
		char* buffer = new char[buffer_size];

		bool is_first_vcf = true;
		foreach(QString in, ins)
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
			gzclose(file);

			is_first_vcf = false;
		}
		out_p->close();
		delete[] buffer;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
