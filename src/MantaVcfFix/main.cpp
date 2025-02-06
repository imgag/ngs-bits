#include "Exceptions.h"
#include "ToolBase.h"
#include "BedFile.h"
#include "VcfFile.h"
#include "ChromosomalIndex.h"
#include "Helper.h"
#include <QFile>



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
		setDescription("Fixes issues in VCF of Manta SV calls.");
		setExtendedDescription(QStringList() << "Removes invalid VCF lines containing empty REF entries."
											 << "Removes duplicate SV calls from Manta VCFs."
							   );
		addInfile("in", "Input VCF file.", false, true);
		addOutfile("out", "Output VCF file.", false, true);

		//optional:
		addFlag("debug", "Print verbose output to STDERR.");


		changeLog(2024,  10, 31, "initial commit.");
		changeLog(2025,   2,  4, "added filtering.");
	}

	virtual void main()
	{
		//static variables
		static const int buffer_size = 1048576; //1MB buffer
		static char* buffer = new char[buffer_size];

		QString in = getInfile("in");
		QString out = getOutfile("out");
		bool debug = getFlag("debug");

		//open output stream
		QSharedPointer<QFile> out_stream = Helper::openFileForWriting(out, true);

		//open input steam
		FILE* file = in.isEmpty() ? stdin : fopen(in.toUtf8().data(), "rb");
		if (file==nullptr) THROW(FileAccessException, "Could not open file '" + in + "' for reading!");
		gzFile in_stream = gzdopen(fileno(file), "rb"); //always open in binary mode because windows and mac open in text mode
		if (in_stream==nullptr) THROW(FileAccessException, "Could not open file '" + in + "' for reading!");

		//cache to store read SVs
		QMap<QByteArray,QByteArrayList> cache;

		//read lines
		while(!gzeof(in_stream))
		{
			char* char_array = gzgets(in_stream, buffer, buffer_size);

			//handle errors like truncated GZ file
			if (char_array==nullptr)
			{
				int error_no = Z_OK;
				QByteArray error_message = gzerror(in_stream, &error_no);
				if (error_no!=Z_OK && error_no!=Z_STREAM_END)
				{
					THROW(FileParseException, "Error while reading input: " + error_message);
				}
			}

			QByteArray line = QByteArray(char_array);

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//keep header unchanged
			if(line.startsWith("#"))
			{
				out_stream->write(line);
				continue;
			}

			QByteArrayList parts = line.split('\t');
			Helper::trim(parts);

			//remove variants with empty REF entry
			if (parts.at(VcfFile::REF).isEmpty())
			{
				if (debug) qDebug() << "Removed SV with empty REF column at " + parts.at(VcfFile::CHROM) + "_" + parts.at(VcfFile::POS);
				continue;
			}

			//remove duplicate SVs (INS) from the manta calls

			//get prefix of MantaID
			QList<QByteArray> manta_id = parts.at(VcfFile::ID).split(':');
			if (manta_id.at(0).startsWith("Manta"))
			{
				manta_id[4] = "X";
			}
			else //DRAGEN VCF
			{
				manta_id[5] = "X";
			}

			QByteArray manta_id_prefix = parts.at(VcfFile::CHROM) + "_" + parts.at(VcfFile::POS) + "_" + manta_id.join(':');

			if(cache.contains(manta_id_prefix))
			{
				if (debug) qDebug() << "Skip duplicate variant at " + parts.at(VcfFile::CHROM) + "_" + parts.at(VcfFile::POS);
				//skip SV
				continue;
			}
			else
			{
				cache.insert(manta_id_prefix, parts);
			}

			//write line
			out_stream->write(parts.join("\t") + "\n");

		}

		out_stream->flush();
		out_stream->close();

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
