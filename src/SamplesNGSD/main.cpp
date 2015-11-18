#include "Exceptions.h"
#include "ToolBase.h"
#include "NGSD.h"
#include "Helper.h"
#include "Exceptions.h"
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
		setDescription("Lists processed samples from NGSD.");
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addString("project", "Project name filter.", true, "");
		addString("sys", "Processing system short name filter.", true, "");
		addEnum("quality", "Minimum processed sample/sample/run quality filter.", true, QStringList() << "bad" << "medium" << "good", "bad");
		addFlag("no_tumor", "If set, tumor samples are excluded.");
		addFlag("no_ffpe", "If set, FFPE samples are excluded.");
		addFlag("check_path", "Checks the sample folder location.");
		addFlag("test", "Uses the test database instead of on the production database.");
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

		//filter project
		QString project = escape(getString("project"));
		if (project!="")
		{
			//check that name is valid
			QString tmp = db.getValue("SELECT id FROM project WHERE name='"+project+"'", true).toString();
			if (tmp=="")
			{
				QVariantList tmp2 = db.getValues("SELECT name FROM project");
				QStringList tmp3;
				foreach(QVariant v, tmp2)
				{
					tmp3 << v.toString();
				}
				THROW(DatabaseException, "Invalid project name '" + project + ".\nValid names are: "+tmp3.join(", "));
			}
			conditions << "p.name='"+project+"'";
		}

		//filter processing system
		QString sys = escape(getString("sys"));
		if (sys!="")
		{
			//check that name is valid
			QString tmp = db.getValue("SELECT id FROM processing_system WHERE name_short='"+sys+ "'", true).toString();
			if (tmp=="")
			{
				QVariantList tmp2 = db.getValues("SELECT name_short FROM procesing_system");
				QStringList tmp3;
				foreach(QVariant v, tmp2)
				{
					tmp3 << v.toString();
				}
				THROW(DatabaseException, "Invalid processing system short name '"+project+".\nValid names are: "+tmp3.join(", "));
			}
			conditions << "sys.name_short='"+sys+"'";
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
		fields << "CONCAT(s.name,'_',LPAD(ps.process_id,2,'0'))";
		fields << "s.name_external";
		fields << "s.gender";
		fields << "s.quality";
		fields << "s.tumor";
		fields << "s.ffpe";
		fields << "ps.quality";
		fields << "ps.last_analysis";
		fields << "sys.name_short";
		fields << "p.name";
		fields << "p.type";
		fields << "r.name";
		fields << "r.fcid";
		fields << "r.recipe";
		fields << "r.quality";
		SqlQuery result = db.getQuery();
		result.exec("SELECT "+fields.join(", ")+" FROM "+tables.join(", ")+" WHERE "+conditions.join(" AND "));

		//write header line
		QSharedPointer<QFile> output = Helper::openFileForWriting(getOutfile("out"), true);
		fields[0] = "ps.name";
		QString line = "#" + fields.join("\t");
		if (check_path) line += "\tcheck_path";
		output->write(line.toLatin1() + "\n");

		//write content lines
		while(result.next())
		{
			QStringList tokens;
			for (int i=0; i<result.record().count(); ++i)
			{
				tokens << result.value(i).toString();
			}
			if (check_path)
			{
				QString type = tokens[fields.indexOf("p.type")];
				if (type=="diagnostic") type = "gs_diag";
				else if (type=="research") type = "gs";
				else if (type=="test") type = "gs_test";
				else if (type=="extern") type = "gs_ext";
				else THROW(ProgrammingException, "Unknown NGSD project type '" + type + "'!");
				QString project_name = tokens[fields.indexOf("p.name")];
#ifdef _WIN32
			QString project_path = "W:/projects/" + type + "/" + project_name + "/";
#else
			QString project_path = "/mnt/projects/" + type + "/" + project_name + "/";
#endif
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
					QStringList matches;
					Helper::findFolders(project_path, "Sample_"+ps_name+"*", matches);
					if (matches.count()!=0)
					{
						tokens << "special folder - " + matches[0];
					}
					else
					{
						Helper::findFiles(project_path, ps_name+"*.fastq.gz", matches);
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
			output->write(tokens.join("\t").toLatin1() + "\n");
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
