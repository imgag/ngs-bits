#include "QcRuleMatcher.h"
#include "Exceptions.h"
#include "XmlHelper.h"

#include <QApplication>
#include <QDir>
#include <QFile>

QcRuleMatcher::QcRuleMatcher()
{
}

QcRuleMatcher &QcRuleMatcher::instance()
{
	static QcRuleMatcher inst;
	return inst;
}

QString QcRuleMatcher::evaluate(const QString& name_short, const QString &sys_type, const QString &term_name, double value, bool is_tumor)
{
	if (instance().xml_doc_.isNull())
	{
		QString cuttofs_file = QApplication::applicationDirPath() + QDir::separator() + "qc_cutoffs.xml";
		QFile file(cuttofs_file);

		if (!file.open(QIODevice::ReadOnly)) THROW(FileAccessException, "Could not open the cutoffs file: " + cuttofs_file);

		QDomDocument doc;
		if (!doc.setContent(&file))
		{
			THROW(FileParseException, "Invalid XML: " + cuttofs_file);
		}

		QString xml_errors = XmlHelper::isValidXml(cuttofs_file, ":/Resources/qc_rules_schema.xsd");
		if (!xml_errors.isEmpty()) THROW(FileParseException, "Could not validate the XML file '" + cuttofs_file + "' against the schema: " + xml_errors);

		instance().xml_doc_ = doc;
	}

	QString expected_tumor_value = is_tumor ? "true" : "false";

	QString result = checkQcRule(name_short, term_name, value, expected_tumor_value, true);
	if (result.isEmpty()) result = checkQcRule(sys_type, term_name, value, expected_tumor_value, false);

	return result;
}

bool QcRuleMatcher::matches(const QString &operation, double value, double cutoff)
{
	if (operation == "less than") return value < cutoff;
	if (operation == "less equal") return value <= cutoff;
	if (operation == "greater than") return value > cutoff;
	if (operation == "greater equal") return value >= cutoff;

	THROW(ProgrammingException, "Unknown operation: " + operation + "!");
}

QString QcRuleMatcher::checkQcRule(const QString &sys_name, const QString &term_name, double value, QString expected_tumor_value, bool needs_sys_override)
{
	QDomElement root = instance().xml_doc_.documentElement();
	QDomNodeList sys_level_rules = root.elementsByTagName("SysTypeRules");
	if (needs_sys_override) sys_level_rules = root.elementsByTagName("SysNameRules");
	for (int i = 0; i < sys_level_rules.count(); i++)
	{
		QDomElement sys_element = sys_level_rules.at(i).toElement();
		if (needs_sys_override && sys_element.attribute("name")!=sys_name) continue;
		if (!needs_sys_override && sys_element.attribute("type")!=sys_name) continue;
		if (sys_element.attribute("tumor")!=expected_tumor_value) continue;

		QDomNodeList term_rules = sys_element.elementsByTagName("TermRules");

		for (int j = 0; j < term_rules.count(); j++)
		{
			QDomElement term_element = term_rules.at(j).toElement();
			if (term_element.attribute("term_name") != term_name) continue;

			QDomNodeList rules = term_element.elementsByTagName("Rule");
			for (int k = 0; k < rules.count(); k++)
			{
				QDomElement rule_element = rules.at(k).toElement();

				QString operation = rule_element.attribute("operation");
				double cutoff = rule_element.attribute("cutoff").toDouble();
				QString current_result = rule_element.attribute("result");

				if (matches(operation, value, cutoff)) return current_result;
			}
			return "";
		}
	}
	return "";

}
