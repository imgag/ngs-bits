#include "ToolBase.h"
#include "NGSD.h"
#include "QCCollection.h"

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
		setDescription("Imports QC metrics of a sample into NGSD.");
		addString("ps", "Processed sample name.", false);
		addInfileList("files", "qcML files to import.", false);
		addFlag("force", "Overwrites already existing QC metrics instead of throwing an error.");
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		QString ps = getString("ps");
		NGSD db(getFlag("test"));
		QString ps_id = db.processedSampleId(ps);
		bool force = getFlag("force");
		QTextStream stream(stdout);

		// check if QC parameters were already imported for this pid
		int count_old =  db.getValue("SELECT count(id) FROM processed_sample_qc WHERE processed_sample_id='" + ps_id + "'").toInt();
		stream << "Found " << count_old << " QC metrics of processed sample '" << ps <<"' already in NGSD." << endl;

		if(count_old!=0)
		{
			if (!force)
			{
				THROW(Exception, "QC metrics of processed sample '" + ps + "' are already imported. Use '-force' to overwrite them.");
			}
			else
			{
				db.getQuery().exec("DELETE FROM processed_sample_qc WHERE processed_sample_id='" + ps_id + "'");
				stream << "Deleted existing QC metrics of '" << ps <<"' because the flag '-force' was used." << endl;
			}
		}

		//read QC terms from qcML files
		QCCollection metrics;
		QSet<QString> accessions_done;
		foreach(QString file, getInfileList("files"))
		{
			QStringList errors;
			QCCollection tmp = QCCollection::fromQCML(file, "://Resources/qcML.obo", errors);
			for (int i=0; i<tmp.count(); ++i)
			{
				const QCValue& metric =	tmp[i];

				//skip plots
				if (metric.type()==QCValueType::IMAGE) continue;

				//check that each metric is contained only once
				const QString& accession = metric.accession();
				if (accessions_done.contains(accession)) THROW(ArgumentException, "Metric " + accession + " contained more than once in input files!");
				accessions_done << accession;

				metrics.insert(metric);
			}

			//output
			foreach(const QString& error, errors)
			{
				stream << "File " + file + " contains error: " + error << endl;
			}
		}

		//import metrics into NGSD
		try
		{
			SqlQuery query = db.getQuery();
			query.prepare("INSERT INTO processed_sample_qc (processed_sample_id, qc_terms_id, value) VALUES (:0, :1, :2)");
			db.transaction();
			for (int i=0; i<metrics.count(); ++i)
			{
				query.bindValue(0, ps_id);
				QString term_id = db.getValue("SELECT id FROM qc_terms WHERE qcml_id=:0", false, metrics[i].accession()).toString();
				query.bindValue(1, term_id);
				query.bindValue(2, metrics[i].toString(6));
				query.exec();
			}
			db.commit();
			stream << "Imported " << metrics.count() << " QC metrics for processed sample " + ps << endl;
		}
		catch (Exception& e)
		{
			db.rollback();
			throw;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

