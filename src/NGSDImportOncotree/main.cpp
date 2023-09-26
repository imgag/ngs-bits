#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "QJsonDocument"
#include "QJsonArray"

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
		setDescription("Imports Oncotree terms and their relations into the NGSD.");
		addInfile("tree", "Oncotree JSON file from 'https://oncotree.mskcc.org/api/tumorTypes/tree'.", false);

		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("force", "If set, overwrites old data.");
		addFlag("debug", "Enables debug output");
	}


	struct OncotreeCode
	{
		QString code;
		QString name;
		QString color;
		int level;
		QString parent_code;
		QString main_type;
		QStringList umls;
		QStringList nci;
		QString tissue;
		QStringList history; // deprecated codes
		QStringList revocations; //? also deprecated codes
		QStringList precursors; // previous names of the node?

	};


	OncotreeCode parseJsonObject(const QJsonObject& node, QList<QJsonObject>& children)
	{
		OncotreeCode parsed_node;
		parsed_node.code = node["code"].toString();
		parsed_node.name = node["name"].toString();
		parsed_node.color = node["color"].toString();
		parsed_node.level = node["level"].toInt(-1);
		if(parsed_node.level == -1)
		{
			THROW(ArgumentException, "The node level of " + parsed_node.name + "with code " + parsed_node.code + " could not be parsed to int! String: "+ node["level"].toString());
		}

		parsed_node.parent_code = node["parent"].toString();
//		qDebug() << "code: " << parsed_node.code << "\t" << parsed_node.parent_code;
		parsed_node.main_type = node["mainType"].toString();
		parsed_node.tissue = node["tissue"].toString();

		//external references:
		if (! node["externalReferences"].isObject()) THROW(ArgumentException, "The external references of node " + parsed_node.name + " with code " + parsed_node.code + " was not an JsonObject.");
		QJsonObject refs = node["externalReferences"].toObject();
		if (refs.contains("UMLS"))
		{
			if (! refs["UMLS"].isArray()) THROW(ArgumentException, "The external reference UMLS of node " + parsed_node.name + " with code " + parsed_node.code + " was not an JsonArray.");
			foreach (QJsonValue var, refs["UMLS"].toArray()) {
				parsed_node.umls.append(var.toString());
			}
		}

		if (refs.contains("NCI"))
		{
			if (! refs["NCI"].isArray()) THROW(ArgumentException, "The external reference NCI of node " + parsed_node.name + " with code " + parsed_node.code + " was not an JsonArray.");
			foreach (QJsonValue var, refs["NCI"].toArray()) {
				parsed_node.nci.append(var.toString());
			}
		}

		if (! node["history"].isArray()) THROW(ArgumentException, "The history of node " + parsed_node.name + " with code " + parsed_node.code + " was not an JsonArray.");
		foreach (QJsonValue var, node["history"].toArray()) {
			parsed_node.history.append(var.toString());
		}

		if (! node["revocations"].isArray()) THROW(ArgumentException, "The revocations of node " + parsed_node.name + " with code " + parsed_node.code + " was not an JsonArray.");
		foreach (QJsonValue var, node["revocations"].toArray()) {
			parsed_node.revocations.append(var.toString());
		}

		if (! node["precursors"].isArray()) THROW(ArgumentException, "The precursors of node " + parsed_node.name + " with code " + parsed_node.code + " was not an JsonArray.");
		foreach (QJsonValue var, node["precursors"].toArray()) {
			parsed_node.precursors.append(var.toString());
		}
		children.clear();
		if (! node["children"].isObject()) THROW(ArgumentException, "The children of node " + parsed_node.name + " with code " + parsed_node.code + " were not an JsonObject.");
		QJsonObject children_obj = node["children"].toObject();
		foreach(QString key, children_obj.keys())
		{
			if (! children_obj[key].isObject()) THROW(ArgumentException, "The child " + key + " of node " + parsed_node.name + " with code " + parsed_node.code + " were not an JsonObject.");
			children.append(children_obj[key].toObject());
		}

		return parsed_node;
	}

	void importOncotree(const NGSD& db, QList<OncotreeCode> codes, bool debug)
	{
		QTextStream out(stdout);
		SqlQuery qi_term = db.getQuery();
		qi_term.prepare("INSERT INTO oncotree_term (oncotree_code, name, color, level, UMLS, NCI) VALUES (:0, :1, :2, :3, :4, :5);");
		SqlQuery qi_parent = db.getQuery();
		qi_parent.prepare("INSERT INTO oncotree_parent (parent, child) VALUES (:0, :1);");
		SqlQuery qi_obs = db.getQuery();
		qi_obs.prepare("INSERT INTO oncotree_obsolete (oncotree_code, reason, replaced_by) VALUES (:0, :1, :2);");

		int count_obsolete = 0;
		int count_parent_relations = 0;

		foreach (const OncotreeCode& code, codes) {
			if (debug) out << "INSERT oncotree_term (oncotree_code, name, color, level, UMLS, NCI) (" << code.code << ", " << code.name << ", " << code.color << ", " << code.level << ", '" << code.umls.join(", ") <<"', '" << code.nci.join(", ") << "')\n";
			qi_term.bindValue(0, code.code);
			qi_term.bindValue(1, code.name);
			qi_term.bindValue(2, code.color);
			qi_term.bindValue(3, code.level);
			qi_term.bindValue(4, code.umls.join(", "));
			qi_term.bindValue(5, code.nci.join(", "));
			qi_term.exec();

			int code_db_id = db.getValue("SELECT id from oncotree_term WHERE oncotree_code='"+code.code+"'").toInt();

			if (code.parent_code != "")
			{
				int parent_id = db.getValue("SELECT id from oncotree_term WHERE oncotree_code='"+code.parent_code+"'").toInt();

				if (debug) out << "INSERT oncotree_parent (parent_id, child_id) (" << parent_id << ", " << code_db_id << ")\n";
				qi_parent.bindValue(0, parent_id);
				qi_parent.bindValue(1, code_db_id);
				qi_parent.exec();
				count_parent_relations++;
			}

			if (code.history.count() != 0)
			{
				foreach(QString c, code.history)
				{
					if (debug) out << "INSERT oncotree_obsolete (oncotree_code, reason, replaced_by) (" << c << ", history, " << code_db_id << ")\n";
					qi_obs.bindValue(0, c);
					qi_obs.bindValue(1, "history");
					qi_obs.bindValue(2, code_db_id);
					qi_obs.exec();
					count_obsolete++;
				}
			}

			if (code.precursors.count() != 0)
			{
				foreach(QString c, code.precursors)
				{
					if (debug) out << "INSERT oncotree_obsolete (oncotree_code, reason, replaced_by) (" << c << ", precursors, " << code_db_id << ")\n";
					qi_obs.bindValue(0, c);
					qi_obs.bindValue(1, "precursors");
					qi_obs.bindValue(2, code_db_id);
					qi_obs.exec();
					count_obsolete++;
				}
			}

			if (code.revocations.count() != 0)
			{
				foreach(QString c, code.revocations)
				{
					if (debug) out << "INSERT oncotree_obsolete (oncotree_code, reason, replaced_by) (" << c << ", revocations, " << code_db_id << ")\n";
					qi_obs.bindValue(0, c);
					qi_obs.bindValue(1, "revocations");
					qi_obs.bindValue(2, code_db_id);
					qi_obs.exec();
					count_obsolete++;
				}
			}
		}
		out << "Imported " << codes.count() << "  Oncotree terms." << endl;
		out << "Imported " << count_parent_relations << "  Oncotree parent-child relations." << endl;
		out << "Imported " << count_obsolete << " obsolete Oncotree terms." << endl;
	}

	QList<OncotreeCode> parsingTree(QJsonObject root)
	{
		QList<OncotreeCode> codes;

		QList<QJsonObject> children;

		OncotreeCode root_node = parseJsonObject(root, children);
		codes.append(root_node);

		foreach(QJsonObject child, children)
		{
			QList<OncotreeCode> child_codes = parsingTree(child);
			codes.append(child_codes);
		}

		return codes;
	}


	QList<OncotreeCode> parseOncotreeJson(QString oncotree_file, bool debug)
	{
		QSharedPointer<QFile> fp = Helper::openFileForReading(oncotree_file);
		QByteArray content = fp->readAll();

		QJsonDocument doc = QJsonDocument::fromJson(content);

		if (doc.isNull())
		{
			THROW(FileParseException, "Given JSON file could not be parsed into as a valid json document!");
		}

		if (! doc.isObject())
		{
			THROW(FileParseException, "Oncotree JSON file is expected to start with the tree root as a json object!");
		}

		QJsonObject root = doc.object();

		QList<OncotreeCode> codes = parsingTree(root["TISSUE"].toObject());

		return codes;
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);
		bool debug = getFlag("debug");


		//check tables exist
		db.tableExists("oncotree_parent");
		db.tableExists("oncotree_obsolete");
		db.tableExists("oncotree_term");

		//clear tables if not empty
		if (!db.tableEmpty("oncotree_term") || !db.tableEmpty("oncotree_parent") || !db.tableEmpty("oncotree_obsolete"))
		{
			if (getFlag("force"))
			{
				db.clearTable("oncotree_parent");
				db.clearTable("oncotree_obsolete");
				db.clearTable("oncotree_term");
			}
			else
			{
				THROW(DatabaseException, "Tables already contain data! Use '-force' to overwrite old data!");
			}
		}


		QList<OncotreeCode> codes = parseOncotreeJson(getInfile("tree"), debug);
		importOncotree(db, codes, debug);

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
