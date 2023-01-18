#include "ToolBase.h"
#include "NGSD.h"
#include "Helper.h"
#include <QJsonDocument>
#include <QJsonArray>

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
		setDescription("Exports meta data of a study from NGSD to a JSON format for import into GHGA.");
		addInfile("data", "JSON file with data that is not contained in NGSD.", false);
		addOutfile("out", "Output JSON file.", false, false);

		//optional
		addFlag("test", "Test mode: uses the test NGSD, does not calcualte size/checksum of BAMs, ...");

		changeLog(2022,  1, 17, "Initial version."); //TODO update
	}

	//returns an array from the input JSON
	QStringList getArray(const QJsonObject& data, QString key)
	{
		if (!data.contains(key)) THROW(FileParseException, "JSON input file does not contain key '" + key + "'!");

		QJsonValue value = data.value(key);
		if (!value.isArray()) THROW(FileParseException, "JSON input file does contain key '" + key + "' with invalid type (not array)!");

		QStringList output;
		foreach(const QJsonValue& entry, value.toArray())
		{
			output << entry.toString();
		}
		return output;
	}

	//returns an array from the input JSON
	QString getString(const QJsonObject& data, QString key)
	{
		if (!data.contains(key)) THROW(FileParseException, "JSON input file does not contain key '" + key + "'!");

		QJsonValue value = data.value(key);
		if (!value.isString()) THROW(FileParseException, "JSON input file does contain key '" + key + "' with invalid type (not string)!");

		return value.toString();
	}

	//Processed sample heler struct
	struct PSData
	{
		QString ngsd_id;
		QString name;
		QString bam;
		QString pseudonym; //processed sample ID left-padded with '0' to 6 digits (prefixed with 3-character object type to generate the alias)

	};

	//Common data helper struct
	struct CommonData
	{
		//general data
		QString version;
		bool test_mode;

		//study
		QString study_name;
		QStringList study_affilitions;

		//processed samples
		QList<PSData> ps_list;
	};

	void addStudy(QJsonObject& parent, const CommonData& data, const NGSD& db)
	{
		QJsonObject obj;

		obj.insert("alias", data.study_name);
		obj.insert("title", data.study_name);

		QString desc = db.getValue("SELECT description FROM study WHERE name='" + data.study_name + "'").toByteArray();
		obj.insert("description", desc);

		obj.insert("affiliation", QJsonArray::fromStringList(data.study_affilitions));

		//fixed
		obj.insert("type", "resequencing");
		obj.insert("has_attribute", QJsonValue());
		obj.insert("has_project", QJsonValue());
		obj.insert("has_publication", QJsonValue());
		obj.insert("schema_type", "CreateStudy");
		obj.insert("schema_version", data.version);

		parent.insert("has_study", obj);
	}


	void addExperiments(QJsonObject& parent, const CommonData& data, NGSD& db)
	{
		QJsonArray array;

		foreach(const PSData& ps_data, data.ps_list)
		{
			ProcessedSampleData data_ngsd = db.getProcessedSampleData(ps_data.ngsd_id);

			QJsonObject obj;
			obj.insert("alias", "EXP" + ps_data.pseudonym);
			obj.insert("description", "short-read sequencing (" + data_ngsd.processing_system_type + ")");
			//TODO add experiment type to JSON (Email from 17.01.23)

			obj.insert("has_file", QJsonArray::fromStringList(QStringList() << "BAM" + ps_data.pseudonym));
			obj.insert("has_protocol", QJsonArray::fromStringList(QStringList() << "LIB" + ps_data.pseudonym << "SEQ" + ps_data.pseudonym));
			obj.insert("has_sample", QJsonArray::fromStringList(QStringList() << "SAM" + ps_data.pseudonym));

			//fixed
			obj.insert("has_study", data.study_name);
			obj.insert("biological_replicates", "1");
			obj.insert("technical_replicates", "1");
			obj.insert("title", QJsonValue());
			obj.insert("schema_type", "CreateExperiment");
			obj.insert("schema_version", data.version);

			array.append(obj);
		}

		parent.insert("has_experiment", array);
	}


	void addFiles(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;

		foreach(const PSData& ps_data, data.ps_list)
		{
			QJsonObject obj;
			obj.insert("alias", "BAM" + ps_data.pseudonym);
			obj.insert("name", ps_data.pseudonym + ".bam");

			if (data.test_mode)
			{
				obj.insert("size", "6000000");
				obj.insert("checksum", "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
			}
			else
			{
				QFile file(ps_data.bam);
				if (!file.open(QFile::ReadOnly)) THROW(FileAccessException, "Could not open file " + ps_data.bam);
				obj.insert("size", file.size());

				QCryptographicHash hash(QCryptographicHash::Md5);
				if (!hash.addData(&file)) THROW(Exception, "Could not calcualate checksum of " + ps_data.bam);
				obj.insert("checksum", QString(hash.result()));
			}

			//fixed
			obj.insert("format", "bam");
			obj.insert("checksum_type", "MD5");
			obj.insert("schema_type", "CreateExperiment");
			obj.insert("schema_version", data.version);

			array.append(obj);
		}

		parent.insert("has_file", array);
	}

	virtual void main()
	{
		//init
		QJsonParseError parse_error;
		QJsonDocument data_doc = QJsonDocument::fromJson(Helper::loadTextFile(getInfile("data")).join("\n").toUtf8(), &parse_error);
		if (data_doc.isNull()) THROW(FileParseException, "Could not read '" + getInfile("data") + "': " + parse_error.errorString());

		QJsonObject data_obj = data_doc.object();

		CommonData data;
		data.version = "0.9.0";
		data.test_mode = getFlag("test");
		data.study_name = getString(data_obj, "study");
		data.study_affilitions = getArray(data_obj, "affiliations");
		NGSD db(data.test_mode);

		//determine processed samples to export
		ProcessedSampleSearchParameters params;
		params.include_bad_quality_runs = false;
		params.include_bad_quality_samples = false;
		params.include_merged_samples = false;
		params.s_study = data.study_name;
		params.add_path = "BAM";
		DBTable ps_table = db.processedSampleSearch(params);
		int bam_idx = ps_table.columnIndex("path");
		for(int r=0; r<ps_table.rowCount(); ++r)
		{
			const DBRow& row = ps_table.row(r);

			QString ps = row.value(0);
			QString bam = row.value(bam_idx);
			if (!QFile::exists(bam) && !data.test_mode) THROW(Exception, "Processed sample " + ps + " BAM missing: " + bam);
			data.ps_list << PSData{row.id(), ps, bam, row.id().rightJustified(6, '0')};
		}

		//create JSON
		QJsonObject root;
		root.insert("schema_type", "CreateSubmission");
		root.insert("schema_version", data.version);
		addStudy(root, data, db);
		addExperiments(root, data, db);
		addFiles(root, data);

		//store JSON
		QJsonDocument doc(root);
		Helper::storeTextFile(getOutfile("out"), QStringList() << doc.toJson());
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
