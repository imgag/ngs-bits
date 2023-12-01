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
		setDescription("For the given processed sample, lists all processed samples of the same patient or sample.");
		setExtendedDescription(QStringList() << "Does not contain the given processed sample itself.");
		addString("ps", "Processed sample name.", false);
		//optional
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addString("sample_type", "Comma-separated list of sample types.", true, "");
		addString("system_type", "Comma-separated list of processing system types.", true, "");
		addString("system", "Comma-separated list of processing system (short) names.", true, "");
		addEnum("mode", "Type of relation (either only same-sample or same-patient (includes same-sample).", true, QStringList() << "SAME_SAMPLE" << "SAME_PATIENT", "SAME_PATIENT");
		addFlag("include_merged", "Include merged samples in the output (will be ignored on default).");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2023, 11, 15, "initial commit");
		changeLog(2023, 11, 21, "remove merged sample on default");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QSharedPointer<QFile> output = Helper::openFileForWriting(getOutfile("out"), true);

		QString ps_name = getString("ps").trimmed();
		int provided_ps_id = db.processedSampleId(ps_name).toInt();
		int provided_s_id = db.sampleId(ps_name).toInt();

		//get filter parameter
		QSet<QString> filter_sample_types = getString("sample_type").split(',').toSet();
		filter_sample_types.remove("");
		QSet<QString> filter_system_types = getString("system_type").split(',').toSet();
		filter_system_types.remove("");
		QSet<QString> filter_systems = getString("system").split(',').toSet();
		filter_systems.remove("");

		//validate filter parameters
		QStringList valid_sample_types = db.getEnum("sample", "sample_type");
		foreach (const QString& sample_type, filter_sample_types)
		{
			if (!valid_sample_types.contains(sample_type)) THROW(ArgumentException, "Invalid sample type '" + sample_type + "' provided!\n Valid sample types are: " + valid_sample_types.join(","));
		}

		QStringList valid_system_types = db.getEnum("processing_system", "type");
		foreach (const QString& system_type, filter_system_types)
		{
			if (!valid_system_types.contains(system_type)) THROW(ArgumentException, "Invalid processing system type '" + system_type + "' provided!\n Valid system types are: " + valid_system_types.join(","));
		}
		QStringList valid_system_names = db.getValues("SELECT name_short FROM processing_system");
		foreach (const QString& system_name, filter_systems)
		{
			if (!valid_system_names.contains(system_name)) THROW(ArgumentException, "Invalid processing system (short) name '" + system_name + "' provided!");
		}

		//get same samples
		SameSampleMode mode = (getEnum("mode")=="SAME_PATIENT") ? SameSampleMode::SAME_PATIENT : SameSampleMode::SAME_SAMPLE;
		QSet<int> same_samples = db.sameSamples(provided_s_id, mode);
		// add provided sample id itself to report different processings
		same_samples.insert(provided_s_id);

		//get processed samples
		QStringList ps_table;
		foreach (int s_id, same_samples)
		{
			SampleData s_data = db.getSampleData(QString::number(s_id));
			QList<int> ps_ids = db.getValuesInt("SELECT id FROM processed_sample WHERE sample_id=:0", QString::number(s_id));
			foreach (int ps_id, ps_ids)
			{
				//skip sample itself:
				if (ps_id == provided_ps_id) continue;
				ProcessedSampleData ps_data = db.getProcessedSampleData(QString::number(ps_id));
				QDate run_start_date = db.getValue("SELECT start_date FROM sequencing_run WHERE name=:0", false, ps_data.run_name).toDate();
				QString sys_name_short = db.getValue("SELECT name_short FROM processing_system WHERE name_manufacturer=:0", false, ps_data.processing_system).toString();
				if (!getFlag("include_merged"))
				{
					if (db.getValue("SELECT COUNT(processed_sample_id) FROM merged_processed_samples WHERE processed_sample_id=:0", false, QString::number(ps_id)).toInt() > 0) continue;
				}


				//apply filter
				if (!filter_sample_types.isEmpty() && !filter_sample_types.contains(s_data.type)) continue;
				if (!filter_system_types.isEmpty() && !filter_system_types.contains(ps_data.processing_system_type)) continue;
				if (!filter_systems.isEmpty() && !filter_systems.contains(sys_name_short)) continue;

				QStringList line;
				line << ps_data.name;
				line << s_data.type;
				line << sys_name_short;
				line << ps_data.processing_system_type;
				line << ps_data.processing_system;
				line << ps_data.run_name;
				line << run_start_date.toString("dd.MM.yyyy");

				ps_table << line.join("\t");
			}
		}

		QStringList header_line;
		header_line << "#processed_sample";
		header_line << "sample_type";
		header_line << "processing_system_type";
		header_line << "processing_system_name";
		header_line << "processing_system_name_short";
		header_line << "run_id";
		header_line << "run_date";

		//sort by processed sample name
		std::sort(ps_table.begin(), ps_table.end());

		//write to output file
		output->write(header_line.join("\t").toUtf8() + '\n');
		output->write(ps_table.join("\n").toUtf8());
		output->flush();
		output->close();

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
