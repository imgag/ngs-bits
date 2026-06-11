#include "QcRuleMatcher.h"
#include "Exceptions.h"
#include <QFile>

QcRuleMatcher::QcRuleMatcher(QString cutoff_config_file)
	: has_rules_(false)
{
	if (xml_root_.isNull())
	{
		QFile file(cutoff_config_file);

		if (!file.open(QIODevice::ReadOnly)) THROW(FileAccessException, "Could not open the cutoffs file: " + cutoff_config_file);

		QDomDocument doc;
		if (!doc.setContent(&file))
		{
			THROW(FileParseException, "Invalid XML: " + cutoff_config_file);
		}

		QString xml_errors = XmlHelper::isValidXml(cutoff_config_file, ":/Resources/qc_rules_schema.xsd");
		if (!xml_errors.isEmpty()) THROW(FileParseException, "Could not validate the XML file '" + cutoff_config_file + "' against the schema: " + xml_errors);

		// populating the cache
		xml_root_ = doc.documentElement();

		QDomNodeList sys_level_rules;		
		for (int s=0; s<2; s++)
		{
			if (s==0) sys_level_rules = xml_root_.elementsByTagName("SysTypeRules");
			if (s==1) sys_level_rules = xml_root_.elementsByTagName("SysNameRules");

			for (int i=0; i<sys_level_rules.count(); i++)
			{
				QDomElement sys_element = sys_level_rules.at(i).toElement();
				QDomNodeList term_rules = sys_element.elementsByTagName("TermRules");

				for (int j = 0; j < term_rules.count(); j++) rule_term_name_.insert(term_rules.at(j).toElement().attribute("term_name"));
			}
		}
		if (!xml_root_.isNull()) has_rules_ = true;
	}
}

QString QcRuleMatcher::evaluate(const QString& name_short, const QString &sys_type, const QString &term_name, double value, bool is_tumor)
{
	QString expected_tumor_value = is_tumor ? "true" : "false";

	// first we check if there are rules for the processing system short name and use them for validation
	QString result = checkQcRule(name_short, term_name, value, expected_tumor_value, true);
	// if there are no matches, we fall back to using processing system type rules
	if (result.isEmpty()) result = checkQcRule(sys_type, term_name, value, expected_tumor_value, false);

	return result;
}

QString QcRuleMatcher::evaluate(const QString& name_short, const QString& sys_type, const QCCollection& qc_data, bool is_tumor)
{
	int good_count = 0;
	int medium_count = 0;
	int bad_count = 0;

	QSet<QString> qc_names_available;
	for (int i=0; i<qc_data.count(); ++i) qc_names_available.insert(qc_data[i].name());

	bool found_all_qc_metrics = hasAllQcMetrics(qc_names_available, name_short, true, is_tumor);
	if (!found_all_qc_metrics) found_all_qc_metrics = hasAllQcMetrics(qc_names_available, sys_type, false, is_tumor);

	if (!found_all_qc_metrics) return "n/a";

	for (int i=0; i<qc_data.count(); ++i)
	{
		QString quality = evaluate(name_short, sys_type, qc_data[i].name(), qc_data[i].asDouble(), is_tumor);
		if (quality == "good") good_count++;
		if (quality == "medium") medium_count++;
		if (quality == "bad") bad_count++;
	}
	if (bad_count>0) return "bad";
	if (medium_count>0) return "medium";
	if (good_count>0) return "good";

	return "";
}

bool QcRuleMatcher::hasRules()
{
	return has_rules_;
}

bool QcRuleMatcher::hasAllQcMetrics(QSet<QString>& term_names, const QString& rule_type, bool needs_sys_override, bool is_tumor)
{
	bool result = false;
	QDomNodeList sys_level_rules = xml_root_.elementsByTagName("SysTypeRules");
	if (needs_sys_override) sys_level_rules = xml_root_.elementsByTagName("SysNameRules");
	for (int i = 0; i < sys_level_rules.count(); i++)
	{
		QDomElement sys_element = sys_level_rules.at(i).toElement();
		if (needs_sys_override && sys_element.attribute("name")!=rule_type) continue;
		if (!needs_sys_override && sys_element.attribute("type")!=rule_type) continue;
		if (sys_element.attribute("tumor")!=(is_tumor ? "true" : "false")) continue;

		QDomNodeList term_rules = sys_element.elementsByTagName("TermRules");
		for (int j = 0; j < term_rules.count(); j++)
		{
			if (!term_names.contains(term_rules.at(j).toElement().attribute("term_name"))) return false;
			else result = true;
		}
	}
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
	QDomNodeList sys_level_rules = xml_root_.elementsByTagName("SysTypeRules");
	if (needs_sys_override) sys_level_rules = xml_root_.elementsByTagName("SysNameRules");
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
		}
	}
	return "";
}
