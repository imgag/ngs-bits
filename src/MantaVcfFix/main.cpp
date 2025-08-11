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
		QString in = getInfile("in");
		QString out = getOutfile("out");
		bool debug = getFlag("debug");

		//open output stream
		QSharedPointer<QFile> out_stream = Helper::openFileForWriting(out, true);

		//open input steam
		QSharedPointer<VersatileFile> file = Helper::openVersatileFileForReading(in, true);

		//cache to store read SVs
		QMap<QByteArray,int> id_buffer_mapping;
		QByteArrayList output_buffer;
		int buffer_idx = 0;

		//read lines
		while(!file->atEnd())
		{
			QByteArray line = file->readLine();

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

			//get SV length (except for INS)
			QByteArray sv_length;
			if (!parts.at(VcfFile::INFO).contains("SVTYPE=INS"))
			{
				foreach (const QByteArray& info_kv, parts.at(VcfFile::INFO).split(';'))
				{
					if (info_kv.startsWith("SVLEN="))
					{
						sv_length = info_kv.split('=').at(1).trimmed();
						break;
					}

				}
			}

			QByteArray manta_id_prefix = parts.at(VcfFile::CHROM) + "_" + parts.at(VcfFile::POS) + "_" + manta_id.join(':') + ((sv_length.isEmpty())?"":"_SVLEN=" + sv_length);

			if(id_buffer_mapping.contains(manta_id_prefix))
			{
				//get cached line
				int buffer_pos = id_buffer_mapping.value(manta_id_prefix);
				QByteArrayList cached_parts = output_buffer.at(buffer_pos).split('\t');
				Helper::trim(cached_parts);
				// Keep variant with higher quality
				int qual_current = -1;
				int qual_cache = -1;
				if (parts.at(VcfFile::QUAL) != ".") qual_current = Helper::toInt(parts.at(VcfFile::QUAL), "VCF quality value (current varinat)", line);
				if (cached_parts.at(VcfFile::QUAL) != ".") qual_cache =  Helper::toInt(cached_parts.at(VcfFile::QUAL), "VCF quality value (cached variant)", line);

				if (qual_current > qual_cache)
				{
					//replace variant
					output_buffer[buffer_pos] = parts.join("\t") + "\n";
					if (debug) qDebug() << "Replaced duplicate variant at " + parts.at(VcfFile::CHROM) + "_" + parts.at(VcfFile::POS);
				}
				else
				{
					//skip variant
					if (debug) qDebug() << "Skip duplicate variant at " + parts.at(VcfFile::CHROM) + "_" + parts.at(VcfFile::POS);
				}

			}
			else //write variant to file
			{
				//Don't modify BNDs

				if (!parts.at(VcfFile::INFO).contains("SVTYPE=BND")) id_buffer_mapping.insert(manta_id_prefix, buffer_idx);
				//write line to buffer
				output_buffer.append(parts.join("\t") + "\n");
				buffer_idx++;
			}
		}

		//write variants from buffer to file
		foreach (const QByteArray& line, output_buffer)
		{
			out_stream->write(line);
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
