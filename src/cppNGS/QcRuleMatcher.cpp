#include "QcRuleMatcher.h"
#include "Exceptions.h"
#include <QFile>
#include "Helper.h"
#include "XmlHelper.h"

QcRuleMatcher::QcRuleMatcher(QString rules_xml_file)
{
	//check file
	QFile file(rules_xml_file);
	if (!file.open(QIODevice::ReadOnly)) THROW(FileAccessException, "Could not open the cutoffs file: " + rules_xml_file);

	QDomDocument doc;
	if (!doc.setContent(&file)) THROW(FileParseException, "Invalid XML: " + rules_xml_file);

	QString xml_errors = XmlHelper::isValidXml(rules_xml_file, ":/Resources/qc_rules_schema.xsd");
	if (!xml_errors.isEmpty()) THROW(FileParseException, "Could not validate the XML file '" + rules_xml_file + "' against the schema: " + xml_errors);

	//populating the cache
	xml_root_ = doc.documentElement();

	//get used QC term names
	foreach(QDomNodeList sys_level_rules, QList<QDomNodeList>() << xml_root_.elementsByTagName("SysTypeRules") << xml_root_.elementsByTagName("SysNameRules"))
	{
		for (int i=0; i<sys_level_rules.count(); ++i)
		{
			QDomNodeList term_rules = sys_level_rules.at(i).toElement().elementsByTagName("TermRules");
			for (int j=0; j<term_rules.count(); ++j)
			{
				used_qc_term_names_.insert(term_rules.at(j).toElement().attribute("term_name"));
			}
		}
	}
}

QString QcRuleMatcher::evaluate(const QString& term_name, double value, const QString& name_short, const QString& sys_type, bool is_tumor)
{
	//if the QC term is not used in any rule, we ignore it
	if (!used_qc_term_names_.contains(term_name)) return "";

	//check rules
	QDomNodeList term_rules = getRules(name_short, sys_type, is_tumor);
	for (int i=0; i<term_rules.count(); ++i)
	{
		QDomElement term_element = term_rules.at(i).toElement();
		if (term_element.attribute("term_name")!=term_name) continue;

		QDomNodeList rules = term_element.elementsByTagName("Rule");
		for (int j=0; j<rules.count(); ++j)
		{
			QDomElement rule_element = rules.at(j).toElement();
			QString operation = rule_element.attribute("operation");
			double cutoff = rule_element.attribute("cutoff").toDouble();
			if (matches(operation, value, cutoff)) return rule_element.attribute("result");
		}
	}

	//no rule matched
	return "";
}

QString QcRuleMatcher::evaluate(const QCCollection& qc_data, const QString& name_short, const QString& sys_type, bool is_tumor)
{
	if (!hasAllQcMetrics(qc_data, name_short, sys_type, is_tumor)) return "n/a";

	int good_count = 0;
	int medium_count = 0;
	int bad_count = 0;
	for (int i=0; i<qc_data.count(); ++i)
	{
		//if the QC term is not used in any rule, we ignore it
		if (!used_qc_term_names_.contains(qc_data[i].name())) continue;

		QString quality = evaluate(qc_data[i].name(), Helper::toDouble(qc_data[i].toString()), name_short, sys_type, is_tumor);
		if (quality == "good") good_count++;
		if (quality == "medium") medium_count++;
		if (quality == "bad") bad_count++;
	}
	if (bad_count>0) return "bad";
	if (medium_count>0) return "medium";
	if (good_count>0) return "good";

	return "";
}

bool QcRuleMatcher::hasAllQcMetrics(const QCCollection& qc_data, const QString& name_short, const QString& sys_type, bool is_tumor)
{
	//get all QC term names that have a numeric value
	QSet<QString> qc_term_names;
	for (int i=0; i<qc_data.count(); ++i)
	{
		bool ok = false;
		qc_data[i].toString().toDouble(&ok);
		if (ok) qc_term_names << qc_data[i].name();
	}

	//check if all terms used in the rule set are present
	QDomNodeList term_rules = getRules(name_short, sys_type, is_tumor);
	for (int i=0; i<term_rules.count(); ++i)
	{
		QString term_name = term_rules.at(i).toElement().attribute("term_name");
		if (!qc_term_names.contains(term_name)) return false;
	}
	return true;
}

QDomNodeList QcRuleMatcher::getRules(const QString& name_short, const QString& sys_type, bool is_tumor)
{
	QString expected_tumor_string = (is_tumor ? "true" : "false");

	//rules for processing system name
	QDomNodeList sys_level_rules = xml_root_.elementsByTagName("SysNameRules");
	for (int i=0; i<sys_level_rules.count(); ++i)
	{
		QDomElement sys_element = sys_level_rules.at(i).toElement();
		if (sys_element.attribute("name")==name_short && sys_element.attribute("tumor")==expected_tumor_string) return sys_element.elementsByTagName("TermRules");
	}

	//rules for processing system type
	sys_level_rules = xml_root_.elementsByTagName("SysTypeRules");
	for (int i=0; i<sys_level_rules.count(); ++i)
	{
		QDomElement sys_element = sys_level_rules.at(i).toElement();
		if (sys_element.attribute("type")==sys_type && sys_element.attribute("tumor")==expected_tumor_string) return sys_element.elementsByTagName("TermRules");
	}

	return QDomNodeList();
}

bool QcRuleMatcher::matches(const QString& operation, double value, double cutoff)
{
	if (operation == "less than") return value < cutoff;
	if (operation == "less equal") return value <= cutoff;
	if (operation == "greater than") return value > cutoff;
	if (operation == "greater equal") return value >= cutoff;

	THROW(ProgrammingException, "Unknown operation: " + operation + "!");
}
