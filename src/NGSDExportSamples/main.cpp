#include "Exceptions.h"
#include "ToolBase.h"
#include "NGSD.h"
#include "Helper.h"
#include "Exceptions.h"
#include "Settings.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDir>

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
		setDescription("Lists processed samples from the NGSD.");
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addString("project", "Project name filter.", true, "");
		addString("sys", "Processing system short name filter.", true, "");
		addString("run", "Sequencing run name filter.", true, "");
		addString("species", "Species filter.", true, "");
		addEnum("quality", "Minimum processed sample/sample/run quality filter.", true, QStringList() << "bad" << "medium" << "good", "bad");
		addFlag("no_tumor", "If set, tumor samples are excluded.");
		addFlag("no_ffpe", "If set, FFPE samples are excluded.");
		addFlag("qc", "If set, QC columns are added to output.");
		addFlag("outcome", "If set, diagnostic outcome columns are added to output.");
		addFlag("disease_details", "If set, disease details columns are added to output.");
		addFlag("check_path", "Checks if the sample folder is present at the defaults location in the 'projects_folder' (as defined in the 'settings.ini' file).");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2019,  1, 10, "Added 'species' filter.");
		changeLog(2018, 10, 23, "Added 'outcome' flag.");
	}

	virtual void main()
	{
		//init
		bool check_path = getFlag("check_path");
		NGSD db(getFlag("test"));
		QStringList tables;
		tables << "processed_sample as ps";
		tables << "sample as s";
		tables << "sequencing_run as r";
		tables << "project as p";
		tables << "processing_system as sys";
		QStringList conditions;
		conditions << "s.id=ps.sample_id";
		conditions << "r.id=ps.sequencing_run_id";
		conditions << "p.id=ps.project_id";
		conditions << "sys.id=ps.processing_system_id";
		bool qc = getFlag("qc");
		bool outcome = getFlag("outcome");
		bool disease_details = getFlag("disease_details");
		QStringList qc_cols = db.getValues("SELECT name FROM qc_terms WHERE obsolete=0 ORDER BY qcml_id");

		//filter project
		QString project = escape(getString("project"));
		if (project!="")
		{
			//check that name is valid
			QVariant tmp = db.getValue("SELECT id FROM project WHERE name='"+project+"'", true);
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid project name '" + project + ".\nValid names are: " + db.getValues("SELECT name FROM project ORDER BY name ASC").join(", "));
			}
			conditions << "p.name='"+project+"'";
		}

		//filter processing system
		QString sys = escape(getString("sys"));
		if (sys!="")
		{
			//check that name is valid
			QVariant tmp = db.getValue("SELECT id FROM processing_system WHERE name_short='"+sys+ "'", true).toString();
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid processing system short name '"+sys+".\nValid names are: " + db.getValues("SELECT name_short FROM processing_system ORDER BY name_short ASC").join(", "));
			}
			conditions << "sys.name_short='"+sys+"'";
		}

		//filter sequencing run
		QString run = escape(getString("run"));
		if (run!="")
		{
			//check that name is valid
			QVariant tmp = db.getValue("SELECT id FROM sequencing_run WHERE name='"+run+ "'", true);
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid sequencing run name '"+run+".\nValid names are: " + db.getValues("SELECT name FROM sequencing_run ORDER BY name ASC").join(", "));
			}
			conditions << "r.name='"+run+"'";
		}

		//filter species
		QString species = escape(getString("species"));
		if (species!="")
		{
			//check that name is valid
			QString tmp = db.getValue("SELECT id FROM species WHERE name='"+species+ "'", true).toString();
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid species name '"+species+".\nValid names are: " + db.getValues("SELECT name FROM species ORDER BY name ASC").join(", "));
			}
			conditions << "s.species_id='"+tmp+"'";
		}

		//filter quality
		QString quality = getEnum("quality");
		if (quality=="medium")
		{
			conditions << "ps.quality!='bad'";
			conditions << "s.quality!='bad'";
			conditions << "r.quality!='bad'";
		}
		else if (quality=="good")
		{
			conditions << "ps.quality!='bad'";
			conditions << "s.quality!='bad'";
			conditions << "r.quality!='bad'";

			conditions << "ps.quality!='medium'";
			conditions << "s.quality!='medium'";
			conditions << "r.quality!='medium'";
		}

		//exclude tumor/ffpe
		if (getFlag("no_tumor")) conditions << "s.tumor='0'";
		if (getFlag("no_ffpe")) conditions << "s.ffpe='0'";

		//query NGSD
		QStringList fields;
		fields << "ps.id";
		fields << "CONCAT(s.name,'_',LPAD(ps.process_id,2,'0'))";
		fields << "s.name_external";
		fields << "s.gender";
		fields << "s.quality";
		fields << "s.tumor";
		fields << "s.ffpe";
		fields << "ps.quality";
		fields << "sys.name_short";
		fields << "p.name";
		fields << "p.type";
		fields << "r.name";
		fields << "r.fcid";
		fields << "r.recipe";
		fields << "r.quality";
		fields << "s.disease_group";
		fields << "s.disease_status";
		SqlQuery result = db.getQuery();
		result.exec("SELECT "+fields.join(", ")+" FROM "+tables.join(", ")+" WHERE "+conditions.join(" AND "));

		//write header line
		QSharedPointer<QFile> output = Helper::openFileForWriting(getOutfile("out"), true);
		QString line = "#";
		fields[1] = "ps.name";
		for (int i=1; i<result.record().count(); ++i) //start at 1 to skip ID
		{
			line += (i==1 ? "" : "\t") + fields[i];
		}
		line += "\tlast_analysis";
		QStringList disease_details_types;
		if (disease_details)
		{
			disease_details_types = db.getEnum("sample_disease_info", "type");
			disease_details_types.sort();
			foreach(QString type, disease_details_types)
			{
				line += "\tdisease_details_" + type.replace(" ", "_");
			}
		}
		if (outcome)
		{
			line += "\toutcome";
			line += "\toutcome_causal_genes";
			line += "\toutcome_inheritance_mode";
			line += "\toutcome_comment";
		}
		if (check_path) line += "\tcheck_path";
		if (qc)
		{
			foreach(QString qc_col, qc_cols)
			{
				line += "\tqc." + qc_col.replace(' ', '_');
			}
		}
		output->write(line.toLatin1() + "\n");

		//write content lines
		while(result.next())
		{
			QStringList tokens;

			//main data
			for (int i=0; i<result.record().count(); ++i)
			{
				tokens << result.value(i).toString();
			}


			//last analysis
			QString ps_id = result.value(0).toString();
			AnalysisJob analysis_job = db.analysisInfo(db.lastAnalysisOf(ps_id));
			tokens << (analysis_job.history.count() ? analysis_job.history.last().timeAsString() : "n/a");

			//check path
			if (check_path)
			{
				QString project_type = tokens[fields.indexOf("p.type")];
				QString project_name = tokens[fields.indexOf("p.name")];
				QString project_path = Settings::string("projects_folder") + "/" + project_type + "/" + project_name + "/";
				QString ps_name = tokens[fields.indexOf("ps.name")];

				if (!QFile::exists(project_path))
				{
					tokens << "not found";
				}
				else if (QFile::exists(project_path + "Sample_" + ps_name + "/"))
				{
					tokens << "found - " + project_path + "Sample_" + ps_name + "/";
				}
				else
				{
					QStringList matches = Helper::findFolders(project_path, "Sample_"+ps_name+"*", true);
					if (matches.count()!=0)
					{
						tokens << "special folder - " + matches[0];
					}
					else
					{
						matches = Helper::findFiles(project_path, ps_name+"*.fastq.gz", true);
						if (matches.count()!=0)
						{
							QFileInfo info(matches[0]);
							tokens << "merged - " + info.filePath().replace(info.fileName(), "");
						}
						else
						{
							tokens << "not found";
						}
					}
				}
			}

			//disease details
			if (disease_details)
			{
				QString sample_id = db.getValue("SELECT sample_id FROM processed_sample WHERE id='" + ps_id + "'").toString();
				foreach(QString type, disease_details_types)
				{
					QStringList tmp;
					QList<SampleDiseaseInfo> disease_info = db.getSampleDiseaseInfo(sample_id, type);
					foreach(const SampleDiseaseInfo& entry, disease_info)
					{
						if (type=="HPO term id")
						{
							tmp << entry.disease_info + " (" + db.phenotypeByAccession(entry.disease_info.toLatin1(), false).name() + ")";
						}
						else
						{
							tmp << entry.disease_info;
						}
					}
					tokens << tmp.join("; ");
				}
			}

			//diagnostic outcome
			if (outcome)
			{
				DiagnosticStatusData diag = db.getDiagnosticStatus(ps_id);
				tokens << diag.outcome;
				tokens << diag.genes_causal.replace("\t", " ").replace("\n", " ");
				tokens << diag.inheritance_mode;
				tokens << diag.comments.replace("\t", " ").replace("\n", " ");
			}

			//QC values
			if (qc)
			{
				SqlQuery qc_res = db.getQuery();
				qc_res.exec("SELECT n.name, nm.value FROM qc_terms n, processed_sample_qc nm WHERE nm.qc_terms_id=n.id AND nm.processed_sample_id='" + ps_id + "' AND n.obsolete=0 ORDER BY n.qcml_id");
				QMap<QString, QString> qc_map;
				while(qc_res.next())
				{
					qc_map.insert(qc_res.value(0).toString(), qc_res.value(1).toString());
				}
				foreach(QString qc_col, qc_cols)
				{
					tokens << qc_map.value(qc_col, "n/a");
				}
			}

			//write output
			for (int i=1; i<tokens.count(); ++i)
			{
				output->write((i==1 ? "" : "\t") + tokens[i].toLatin1());
			}
			output->write("\n");
		}
	}

	QString escape(QString sql_arg)
	{
		return sql_arg.trimmed().replace("\"", "").replace("'", "").replace(";", "").replace("\n", "");
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
