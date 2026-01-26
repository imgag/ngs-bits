#include "ToolBase.h"
#include "NGSD.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include "ArribaFile.h"


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
		setDescription("Imports variants of a RNA processed sample into the NGSD.");
		addString("ps", "Rna processed sample name", false);
		//optional
		addInfile("fusion", "fusion file from arriba", true, true);
		addFlag("fusion_force", "Force import of detected fusions, even if already imported.");
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("no_time", "Disable timing output.");
	}

	//import SNVs/INDELs from tumor-normal GSVar file
	void importFusions(NGSD& db, QTextStream& out, QString ps_name, bool no_time)
	{
		bool fusion_force = getFlag("fusion_force");
		QString filename = getInfile("fusion");
		if(filename=="") return;

        out << Qt::endl;
        out << "### importing fusion variants for " << ps_name << " ###" << Qt::endl;
        out << "filename: " << filename << Qt::endl;

		QString ps_id = db.processedSampleId(ps_name);

		int rna_report_conf_id = db.rnaReportConfigId(ps_id);

		//DO NOT IMPORT Anything if a report config exists and contains small variants
		if(rna_report_conf_id != -1)
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT * FROM rna_report_configuration_fusion WHERE rna_report_configuration_id=" + QString::number(rna_report_conf_id));
			if(query.size()>0)
			{
                out << "Skipped import of fusion variants for sample " << ps_name << ": a rna report configuration with fusion variants exists for this sample!" << Qt::endl;
				return;
			}
		}

		QElapsedTimer timer;
		timer.start();

		QString last_callset_id = db.getValue("SELECT id FROM rna_fusion_callset WHERE processed_sample_id=" + ps_id).toString();

		out << "last callset id:" << last_callset_id << "\n";

		if(last_callset_id != "")
		{
			if (!fusion_force)
			{
                out << "Skipped import of fusion variants for sample " << ps_name << ": fusion callset already exists for this sample!" << Qt::endl;
				return;
			}
			else
			{
				//Delete old fusions if forced
				if(last_callset_id != "" && fusion_force)
				{
					db.getQuery().exec("DELETE FROM rna_fusion WHERE rna_fusion_callset_id ='" + last_callset_id + "'");
					db.getQuery().exec("DELETE FROM rna_fusion_callset WHERE id='" + last_callset_id + "'");

                    out << "Deleted previous rna fusion callset" << Qt::endl;
				}
			}
		}

		ArribaFile fusions;
		fusions.load(filename);
		QString caller = "Arriba";
		if(fusions.count() == 0)
		{
            out << "No fusion variants imported (empty arriba file)." << Qt::endl;
			return;
		}

        out << "caller: " << caller << Qt::endl;
        out << "caller version: " << fusions.getCallerVersion() << Qt::endl;
        out << "call date: " << fusions.getCallDate() << Qt::endl;

		//Import rna fusion callset
		SqlQuery q_set = db.getQuery();
		q_set.prepare("INSERT INTO `rna_fusion_callset` (`processed_sample_id`, `caller`, `caller_version`, `call_date`) VALUES (:0, :1, :2, :3)");
		q_set.bindValue(0, ps_id);
		q_set.bindValue(1, caller);
		q_set.bindValue(2, fusions.getCallerVersion());
		q_set.bindValue(3, fusions.getCallDate());
		q_set.exec();

		int callset_id = q_set.lastInsertId().toInt();

		//Import fusions
		for(int i=0; i<fusions.count(); ++i)
		{
			db.addRnaFusion(callset_id, fusions.getFusion(i));
		}

        out << "Imported rna fusions: " << fusions.count() << Qt::endl;

		if(!no_time)
		{
            out << "Import took: " << Helper::elapsedTime(timer) << Qt::endl;
		}
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream stream(out.data());

		QString ps_name = getString("ps");

		bool no_time = getFlag("no_time");

		//prevent tumor samples from being imported into the germline variant tables
		SampleData sample_data = db.getSampleData(db.sampleId(ps_name));
		if (sample_data.type != "RNA")
		{
			THROW(ArgumentException, "Cannot import variant data for sample " + ps_name + " the sample is not an RNA sample according to NGSD!");
		}

		importFusions(db, stream, ps_name, no_time);
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
