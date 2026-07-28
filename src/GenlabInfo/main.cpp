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
		addString("ps", "Processed sample or TSV file with processed sample ids in the first column", true, "");
		addString("sap_id", "SAP id or TSV file with SAP ids in the first column. Will provide a table with patient ID and processed samples names.", true, "");
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
		QString sap_id = getString("sap_id");

		if (ps.isEmpty() && sap_id.isEmpty()) THROW(ArgumentException, "Either a processed sample or a SAP id has to be provided.");
		if (! ps.isEmpty() && ! sap_id.isEmpty()) THROW(ArgumentException, "A processed sample and a SAP id cannot be procesed at the same time. Please call the tool with only one of the two.");

		//Procesed Sample to SAP ID or patient ID
		if (!ps.isEmpty())
		{
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

		//SAP ID to patient id and processed samples
		if (!sap_id.isEmpty())
		{
			QSharedPointer<QFile> out = Helper::openFileForWriting(getString("out"), true);

			QStringList out_header;
			out_header << "#sap_id" << "patient_id" << "processed_samples";
			out->write(out_header.join("\t").toUtf8() +  "\n");

			QStringList sap_ids;
			if (! QFileInfo(sap_id).isFile())
			{
				sap_ids << sap_id;
			}
			else
			{
				TsvFile in_file;
				in_file.load(sap_id);
				sap_ids = in_file.extractColumn(0);
			}

			foreach(const QString& sap_id, sap_ids)
			{
				out->write(sap_id.toUtf8() + "\t" + getSapIdInfos(sap_id.trimmed(), genlab).join("\t").toUtf8() + "\n");
			}
			out->close();
		}
	}

	QStringList getSapIdInfos(const QString& sap_id, GenLabDB& genlab)
	{
		//pat ID:

		ProcessedSampleSearchParameters params;
		QStringList ps_samples = genlab.samplesWithSapID(sap_id, params);
		QSet<QString> pat_ids_set;
		foreach(const QString& ps, ps_samples)
		{
			pat_ids_set << genlab.patientIdentifier(ps);
		}

		QStringList infos;
		infos << QStringList(pat_ids_set.begin(), pat_ids_set.end()).join(",");
		infos << ps_samples.join(",");
		return infos;
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
