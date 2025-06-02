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

	QStringList validPathStrings()
	{
		QStringList output;

		output << "";

		foreach(PathType type, QList<PathType>() << PathType::SAMPLE_FOLDER << PathType::BAM << PathType::VCF << PathType::GSVAR << PathType::COPY_NUMBER_CALLS << PathType::STRUCTURAL_VARIANTS)
		{
			output << FileLocation::typeToString(type);
		}

		return output;
	}

	virtual void setup()
	{
		setDescription("Lists processed samples from the NGSD.");
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addString("sample", "Sample name filter (substring match).", true, "");
		addFlag("no_bad_samples", "If set, processed samples with 'bad' quality are excluded.");
		addFlag("no_tumor", "If set, tumor samples are excluded.");
		addFlag("no_normal", "If set, germline samples are excluded.");
		addFlag("no_ffpe", "If set, FFPE samples are excluded.");
		addFlag("no_resequencing", "If set, samples that are scheduled for resequencing are excluded.");
		addFlag("match_external_names", "If set, also samples for which the external name matches 'sample' are exported.");
		addFlag("with_merged", "If set, processed samples that were merged into another sample are included.");
		addFlag("only_with_small_variants", "If set, only processed samples that have small variants in NGSD are listed.");
		addString("species", "Species filter.", true, "");
		addString("tissue", "Tissue filter.", true, "");
		addString("ancestry", "Ancestry filter.", true, "");
		addString("disease_group", "Disease group filter", true, "");
		addString("disease_status", "Disease status filter", true, "");
		addString("phenotypes", "HPO phenotype identifiers separated by colon, e.g. 'HP:0002066;HP:0004322'", true, "");
		addString("sender", "Sample sender filter.", true, "");
		addString("study", "Processed sample study filter.", true, "");
		addString("project", "Project name filter.", true, "");
		addString("project_type", "Project type filter", true, "");
		addFlag("no_archived_projects", "If set, samples in archived projects are excluded.");
		addString("system", "Processing system name filter (short name).", true, "");
		addString("system_type", "Type of processing system filter", true, "");
		addString("run", "Sequencing run name filter.", true, "");
		addFlag("run_finished", "Only show samples where the analysis of the run is finished.");
		addString("run_device", "Sequencing run device name filter.", true, "");
		addString("run_before", "Sequencing run before or equal to the given date.", true, "");
		addString("run_after", "Sequencing run after or equal to the given date.", true, "");
		addFlag("no_bad_runs", "If set, sequencing runs with 'bad' quality are excluded.");
		addFlag("add_qc", "If set, QC columns are added to output.");
		addFlag("add_outcome", "If set, diagnostic outcome columns are added to output.");
		addFlag("add_disease_details", "If set, disease details columns are added to the output.");
		addEnum("add_path", "Adds a column with the given path type.", true, validPathStrings());
		addFlag("add_report_config", "Adds a column with report configuration information (if it exists and if causal variants exist).");
		addFlag("add_comments", "Adds sample and processed sample comments columns.");
		addFlag("add_normal_sample", "Adds a column with the normal germline sample associated to a tumor samples.");
		addFlag("add_dates", "Adds four columns with year of birth, order date, sampling date and sample receipt date.");
		addFlag("add_call_details", "Adds variant caller and version and variant calling date columns for small variants, CNVs and SVs.");
		addFlag("add_lab_columns", "Adds columns input, molarity, operator, processing method and batch number.");
		addFlag("add_study_column", "Add a column with studies of the sample.");
		addFlag("test", "Uses the test database instead of on the production database.");
		addEnum("preset", "Presets for different common searches. Note: presets are applied after argument parsing and thus override command line argument.", true, QStringList() << "none" << "germline", "none");

		changeLog(2025,  5, 19, "Added 'preset' and 'no_resequencing' parameters.");
		changeLog(2024,  8, 21, "Added 'add_study_column' flag.");
		changeLog(2024,  4, 24, "Added 'only_with_small_variants' flag.");
		changeLog(2024,  3,  4, "Added 'add_lab_columns' flag.");
		changeLog(2023, 11, 16, "Added 'add_call_details' flag.");
		changeLog(2023,  7, 13, "Added 'add_dates' flag.");
		changeLog(2022, 11, 11, "Added 'ancestry' and 'phenotypes' filter options.");
		changeLog(2022,  3,  3, "Added 'disease_group', 'disease_status', 'project_type' and 'tissue' filter options.");
		changeLog(2021,  4, 29, "Added 'run_before' filter option.");
		changeLog(2021,  4, 16, "Added ancestry column.");
		changeLog(2021,  4, 13, "Changed 'add_path' parameter to support different file/folder types.");
		changeLog(2020, 10,  8, "Added parameters 'sender' and 'study'.");
		changeLog(2020,  7, 20, "Added 'match_external_names' flag.");
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
		params.s_name_ext = getFlag("match_external_names");
		params.s_species = getString("species");
		params.s_sender = getString("sender");
		params.s_tissue = getString("tissue");
		params.s_ancestry = getString("ancestry");
		params.s_disease_group = getString("disease_group");
		params.s_disease_status = getString("disease_status");
		foreach(QString hpo_id, getString("phenotypes").split(";"))
		{
			hpo_id = hpo_id.trimmed();
			if (hpo_id.isEmpty()) continue;
			params.s_phenotypes << db.phenotype(db.phenotypeIdByAccession(hpo_id.toUtf8()));
		}
		params.s_study = getString("study");
		params.include_bad_quality_samples = !getFlag("no_bad_samples");
		params.include_tumor_samples = !getFlag("no_tumor");
		params.include_germline_samples = !getFlag("no_normal");
		params.include_ffpe_samples = !getFlag("no_ffpe");
		params.include_scheduled_for_resequencing_samples = !getFlag("no_resequencing");
		params.include_merged_samples = getFlag("with_merged");
		params.only_with_small_variants = getFlag("only_with_small_variants");
		params.p_name = getString("project");
		params.p_type = getString("project_type");
		params.include_archived_projects = !getFlag("no_archived_projects");
		params.sys_name = getString("system");
		params.sys_type = getString("system_type");
		params.r_name = getString("run");
		params.include_bad_quality_runs = !getFlag("no_bad_runs");
		params.run_finished = getFlag("run_finished");
		params.r_device_name = getString("run_device");
		if (getString("run_before").trimmed()!="")
		{
			QDate run_start_date = QDate::fromString(getString("run_before"), Qt::ISODate);
			if (!run_start_date.isValid())
			{
				THROW(ArgumentException, "Invalid date given for 'run_before' parameter.\nThe expected format is a ISO date, e.g. '2012-09-27'.");
			}
			params.r_before = run_start_date;
		}
		if (getString("run_after").trimmed()!="")
		{
			QDate run_start_date = QDate::fromString(getString("run_after"), Qt::ISODate);
			if (!run_start_date.isValid())
			{
				THROW(ArgumentException, "Invalid date given for 'run_after' parameter.\nThe expected format is a ISO date, e.g. '2012-09-27'.");
			}
			params.r_after = run_start_date;
		}
		params.add_qc = getFlag("add_qc");
		params.add_outcome = getFlag("add_outcome");
		params.add_disease_details = getFlag("add_disease_details");
		params.add_path = getEnum("add_path");
		params.add_report_config = getFlag("add_report_config");
		params.add_normal_sample = getFlag("add_normal_sample");
		params.add_comments = getFlag("add_comments");
		params.add_dates = getFlag("add_dates");
		params.add_call_details = getFlag("add_call_details");
		params.add_lab_columns = getFlag("add_lab_columns");
		params.add_study_column = getFlag("add_study_column");

		//apply presets
		if (getEnum("preset")=="germline")
		{
			params.include_ffpe_samples = false;
			params.include_tumor_samples = false;
			params.include_merged_samples = false;
			params.include_bad_quality_samples = false;
			params.include_scheduled_for_resequencing_samples = false;

			params.include_archived_projects = false;

			params.include_bad_quality_runs = false;
			params.run_finished = true;
		}

		//check parameters
		if (! params.include_germline_samples && ! params.include_tumor_samples)
		{
			THROW(ArgumentException, "Flags 'no_normal' and 'no_tumor' can't be provided for the same export.");
		}

		if (params.p_name!="")
		{
			//check that name is valid
			QVariant tmp = db.getValue("SELECT id FROM project WHERE name=:0", true, params.p_name);
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid project name '" + params.p_name + "'.\nValid names are: " + db.getValues("SELECT name FROM project ORDER BY name ASC").join(", "));
			}
		}

		if (params.p_type!="")
		{
			//check that type is valid
			QStringList values = db.getEnum("project", "type");
			if (! values.contains(params.p_type))
			{
				THROW(DatabaseException, "Invalid project type '" + params.p_type + "'.\nValid types are: " + values.join(", "));
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

		if (params.sys_type!="")
		{
			//check that type is valid
			QStringList values = db.getEnum("processing_system", "type");
			if (! values.contains(params.sys_type))
			{
				THROW(DatabaseException, "Invalid processing system type '" + params.sys_type + "'.\nValid types are: " + values.join(", "));
			}
		}

		if (params.r_name!="")
		{
			//check that name is valid
			QVariant tmp = db.getValue("SELECT id FROM sequencing_run WHERE name=:0", true, params.r_name);
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid sequencing run name '"+params.r_name+"'.\nValid names are: " + db.getValues("SELECT name FROM sequencing_run ORDER BY name ASC").join(", "));
			}
		}

		if (params.r_device_name!="")
		{
			//check that name is valid
			QVariant tmp = db.getValue("SELECT id FROM device WHERE name=:0", true, params.r_device_name);
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid sequencing run device name '"+params.r_device_name+"'.\nValid names are: " + db.getValues("SELECT name FROM device ORDER BY name ASC").join(", "));
			}
		}

		if (params.s_species!="")
		{
			//check that name is valid
			QString tmp = db.getValue("SELECT id FROM species WHERE name=:0", true, params.s_species).toString();
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid species name '"+params.s_species+"'.\nValid names are: " + db.getValues("SELECT name FROM species ORDER BY name ASC").join(", "));
			}
		}

		if (params.s_sender!="")
		{
			//check that name is valid
			QString tmp = db.getValue("SELECT id FROM sender WHERE name=:0", true, params.s_sender).toString();
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid sender name '"+params.s_sender+"'.\nValid names are: " + db.getValues("SELECT name FROM sender ORDER BY name ASC").join(", "));
			}
		}

		if (params.s_study!="")
		{
			//check that name is valid
			QString tmp = db.getValue("SELECT id FROM study WHERE name=:0", true, params.s_study).toString();
			if (tmp.isNull())
			{
				THROW(DatabaseException, "Invalid study name '"+params.s_study+"'.\nValid names are: " + db.getValues("SELECT name FROM study ORDER BY name ASC").join(", "));
			}
		}

		if (params.s_tissue !="")
		{
			QStringList values = db.getEnum("sample", "tissue");
			if (! values.contains(params.s_tissue))
			{
				THROW(DatabaseException, "Invalid sample tissue '"+params.s_tissue+"'.\nValid tissues are: " + values.join(", "));
			}
		}

		if (params.s_ancestry !="")
		{
			QStringList values = db.getEnum("processed_sample_ancestry", "population");
			if (! values.contains(params.s_ancestry))
			{
				THROW(DatabaseException, "Invalid sample ancestry '"+params.s_ancestry+"'.\nValid tissues are: " + values.join(", "));
			}
		}

		if (params.s_disease_group !="")
		{
			QStringList values = db.getEnum("sample", "disease_group");
			if (! values.contains(params.s_disease_group))
			{
				THROW(DatabaseException, "Invalid sample disease group '"+params.s_disease_group+"'.\nValid groups are: " + values.join(", "));
			}
		}

		if (params.s_disease_status !="")
		{
			QStringList values = db.getEnum("sample", "disease_status");
			if (! values.contains(params.s_disease_status))
			{
				THROW(DatabaseException, "Invalid sample disease status '"+params.s_disease_status+"'.\nValid statuses are: " + values.join(", "));
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
