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
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2022,  1, 17, "Initial version."); //TODO update
	}

	//returns an array from the input JSON
	QJsonArray getArray(const QJsonObject& data, QString key)
	{
		if (!data.contains(key)) THROW(FileParseException, "JSON input file does not contain key '" + key + "'!");

		QJsonValue value = data.value(key);
		if (!value.isArray()) THROW(FileParseException, "JSON input file does contain key '" + key + "' with invalid type (not array)!");

		return value.toArray();
	}

	//returns an array from the input JSON
	QString getString(const QJsonObject& data, QString key)
	{
		if (!data.contains(key)) THROW(FileParseException, "JSON input file does not contain key '" + key + "'!");

		QJsonValue value = data.value(key);
		if (!value.isString()) THROW(FileParseException, "JSON input file does contain key '" + key + "' with invalid type (not string)!");

		return value.toString();
	}

	void addStudyObject(QJsonObject& parent, const QJsonObject& data, const NGSD& db)
	{
		QJsonObject obj;

		QString study = getString(data, "study");
		obj.insert("alias", study);
		obj.insert("title", study);

		QString desc = db.getValue("SELECT description FROM study WHERE name='" + study + "'").toByteArray();
		obj.insert("description", desc);

		obj.insert("affiliation", getArray(data, "affiliations"));

		//fixed
		obj.insert("type", "resequencing");
		obj.insert("has_attribute", QJsonValue());
		obj.insert("has_project", QJsonValue());
		obj.insert("has_publication", QJsonValue());
		obj.insert("schema_type", "CreateStudy");
		obj.insert("schema_version", getString(data, "version"));

		parent.insert("has_study", obj);
	}


	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));

		//load input JSON
		QJsonParseError parse_error;
		QJsonDocument data_doc = QJsonDocument::fromJson(Helper::loadTextFile(getInfile("data")).join("\n").toUtf8(), &parse_error);
		if (data_doc.isNull()) THROW(FileParseException, "Could not read '" + getInfile("data") + "': " + parse_error.errorString());

		QJsonObject data = data_doc.object();
		data.insert("version", "0.9.0");

		//create JSON
		QJsonObject root;
		root.insert("schema_type", "CreateSubmission");
		root.insert("schema_version", getString(data, "version"));

		addStudyObject(root, data, db);

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
