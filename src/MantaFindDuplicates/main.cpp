#include "Exceptions.h"
#include "ToolBase.h"
#include "NGSD.h"
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
		setDescription("Reports duplicate SV calls");
		//optional
		addInfile("in", "Input VCF file.", true, true);
		addString("ps", "Processed sample name (path to SV file will be determined by NGSD).", true, "");
		addFlag("test", "Uses the test database instead of on the production database.");

	}

	virtual void main()
	{
		QString in = getInfile("in");
		QString in_fallback =  getInfile("in");
		QString ps_name = getString("ps");
		QString ps_id;
		NGSD db(getFlag("test"));


		if (in.isEmpty() && ps_name.isEmpty()) THROW(ArgumentException, "At least one of the parameters 'in' and 'ps' is required!");

		if (ps_name.isEmpty())
		{
			//get ps_name from file
			ps_id = db.processedSampleId(in, true);
			ps_name = db.processedSampleName(ps_id, true);
		}
		else
		{
			//get ps_id from ps_name
			ps_id = db.processedSampleId(ps_name);
		}

		if (in.isEmpty())
		{
			in = db.processedSamplePath(ps_id, PathType::GSVAR).chopped(6) + "_var_structural_variants.vcf.gz";
			in_fallback = db.processedSamplePath(ps_id, PathType::GSVAR).chopped(6) + "_manta_var_structural.vcf.gz";
		}

		QTextStream out(stdout);
		if (!QFile::exists(in))
		{
			//check fallback file path
			if (!QFile::exists(in_fallback))
			{
				out << ps_name + "\tError: file '" + in +"' not found!\n";
				return;
			}
			else
			{
				in = in_fallback;
			}

		}

		QMap<QByteArray,QByteArrayList> cache;

		//open input steam
		QSharedPointer<VersatileFile> file = Helper::openVersatileFileForReading(in, true);
		while(!file->atEnd())
		{
			QByteArray line = file->readLine(true);

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//keep header unchanged
			if(line.startsWith("#")) continue;

			QByteArrayList parts = line.split('\t');
			Helper::trim(parts);

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

			if(cache.contains(manta_id_prefix))
			{
				out << ps_name + "\t" + manta_id_prefix + "\t" +  cache.value(manta_id_prefix).join('\t') + "\n";
				out << ps_name + "\t" + manta_id_prefix + "\t" +  parts.join('\t') + "\n";
			}

			cache.insert(manta_id_prefix, parts);
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
