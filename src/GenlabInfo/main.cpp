#include "ToolBase.h"
#include "Helper.h"
#include "GenLabDB.h"

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
		setDescription("Provide sample information from GenLAB.");
		addString("ps", "Processed sample or TSV file with processed sample ids in the first column", false);
		//optional
		addString("info", "Infos that will be collected from Genlab.Comma seperated list of values. Supported: SAPID,PATID", true, "SAPID");
		addString("out", "TSV file where the Genlab infos will be written to. stdout if emtpy.", true);
	}

	virtual void main()
	{
		//init
		if (!GenLabDB::isAvailable()) THROW(DatabaseException, "Genlab database is not available. Can't import data.");
		GenLabDB genlab;

		QString ps = getString("ps");
		QStringList infos = getString("info").split(",");
		QSharedPointer<QFile> out = Helper::openFileForWriting(getString("out"), true);

		QStringList out_header;
		out_header << "#sample" << infos;
		out->write(out_header.join("\t").toUtf8() +  "\n");

		QStringList ps_names;
		if (! QFileInfo(ps).isFile())
		{
			ps_names << ps;
		}
		else
		{
			TsvFile in_file;
			in_file.load(ps);
			ps_names = in_file.extractColumn(0);
		}

		foreach(const QString& ps_name, ps_names)
		{
			out->write(ps_name.toUtf8() + "\t" + getInfos(ps_name.trimmed(), genlab, infos).join("\t").toUtf8() + "\n");
		}

		out->close();
	}

	QStringList getInfos(const QString& ps, GenLabDB& genlab, const QStringList& infos)
	{
		QStringList ps_infos;
		foreach(const QString& info, infos)
		{
			if (info == "SAPID")
			{
				ps_infos << genlab.sapID(ps);
			}
			else if (info == "PATID")
			{
				ps_infos << genlab.patientIdentifier(ps);
			}
			else THROW(ArgumentException, "Unknown info: '" + info + "' cannot provide it from GenLab!");
		}
		return ps_infos;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
