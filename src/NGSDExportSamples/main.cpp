#include "ToolBase.h"
#include "NGSD.h"

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
		addString("sample", "Sample name filter (substring match).", true, "");
		addString("species", "Species filter.", true, "");
		addFlag("no_bad_samples", "If set, processed samples with 'bad' quality are excluded.");
		addFlag("no_tumor", "If set, tumor samples are excluded.");
		addFlag("no_ffpe", "If set, FFPE samples are excluded.");
		addFlag("with_merged", "If set, processed samples that were merged into another sample are included.");
		addString("project", "Project name filter.", true, "");
		addString("system", "Processing system name filter (short name).", true, "");
		addString("run", "Sequencing run name filter.", true, "");
		addFlag("run_finished", "Only show samples where the analysis of the run is finished.");
		addString("run_device", "Sequencing run device name filter.", true, "");
		addFlag("no_bad_runs", "If set, sequencing runs with 'bad' quality are excluded.");
		addFlag("add_qc", "If set, QC columns are added to output.");
		addFlag("add_outcome", "If set, diagnostic outcome columns are added to output.");
		addFlag("add_disease_details", "If set, disease details columns are added to output.");
		addFlag("add_path", "Checks if the sample folder is present at the defaults location in the 'projects_folder' (as defined in the 'settings.ini' file).");
		addFlag("add_report_config", "Adds a column with report configuration information (exists/has_small_variants/has_cnvs).");
		addFlag("add_comments", "Adds sample and processed sample comments columns.");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2019, 12, 11, "Added 'run_finished' and 'add_report_config' flags.");
		changeLog(2019,  5, 17, "Added 'with_merged' flag.");
		changeLog(2019,  4, 12, "Complete refactoring and interface change.");
		changeLog(2019,  1, 10, "Added 'species' filter.");
		changeLog(2018, 10, 23, "Added 'outcome' flag.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QSharedPointer<QFile> output = Helper::openFileForWriting(getOutfile("out"), true);

		ProcessedSampleSearchParameters params;
		params.s_name = getString("sample");
		params.s_species = getString("species");
		params.include_bad_quality_samples = !getFlag("no_bad_samples");
		params.include_tumor_samples = !getFlag("no_tumor");
		params.include_ffpe_samples = !getFlag("no_ffpe");
		params.include_merged_samples = getFlag("with_merged");
		params.p_name = getString("project");
		params.sys_name = getString("system");
		params.r_name = getString("run");
		params.include_bad_quality_runs = !getFlag("no_bad_runs");
		params.run_finished = getFlag("run_finished");
		params.r_device_name = getString("run_device");
		params.add_qc = getFlag("add_qc");
		params.add_outcome = getFlag("add_outcome");
		params.add_disease_details = getFlag("add_disease_details");
		params.add_path = getFlag("add_path");
		params.add_report_config = getFlag("add_report_config");
		params.add_comments = getFlag("add_comments");

		//check parameters
		if (params.p_name!="")
		{
			//check that name is valid
			QVariant tmp = db.getValue("SELECT id FROM project WHERE name=:0", true, params.p_name);
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid project name '" + params.p_name + ".\nValid names are: " + db.getValues("SELECT name FROM project ORDER BY name ASC").join(", "));
			}
		}

		if (params.sys_name!="")
		{
			//check that name is valid
			int sys_id = db.processingSystemId(params.sys_name, false);
			if (sys_id==-1)
			{
				THROW(DatabaseException, "Invalid processing system short name '" + params.sys_name + "'.\nValid names are: " + db.getValues("SELECT name_short FROM processing_system ORDER BY name_short ASC").join(", "));
			}
		}

		if (params.r_name!="")
		{
			//check that name is valid
			QVariant tmp = db.getValue("SELECT id FROM sequencing_run WHERE name=:0", true, params.r_name);
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid sequencing run name '"+params.r_name+".\nValid names are: " + db.getValues("SELECT name FROM sequencing_run ORDER BY name ASC").join(", "));
			}
		}

		if (params.r_device_name!="")
		{
			//check that name is valid
			QVariant tmp = db.getValue("SELECT id FROM device WHERE name=:0", true, params.r_device_name);
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid sequencing run device name '"+params.r_device_name+".\nValid names are: " + db.getValues("SELECT name FROM device ORDER BY name ASC").join(", "));
			}
		}

		if (params.s_species!="")
		{
			//check that name is valid
			QString tmp = db.getValue("SELECT id FROM species WHERE name=:0", true, params.s_species).toString();
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid species name '"+params.s_species+".\nValid names are: " + db.getValues("SELECT name FROM species ORDER BY name ASC").join(", "));
			}
		}

		//process
		DBTable table = db.processedSampleSearch(params);
		QTextStream stream(output.data());
		table.write(stream);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
