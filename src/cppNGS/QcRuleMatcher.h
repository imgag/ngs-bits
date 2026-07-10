#ifndef QCRULEMATCHER_H
#define QCRULEMATCHER_H

#include "cppNGS_global.h"
#include "QCCollection.h"
#include <QSet>

//Calcualtes quality classes (good, medium, bad) based on QC metrics
class CPPNGSSHARED_EXPORT QcRuleMatcher
{
public:
	//default constructor
	QcRuleMatcher(QString rules_xml_file);

	//checks a QC term against the rules and assignes a corresponding label: good, medium, bad or "" (if no rule matched)
	QString evaluate(const QString& term_name, double value, const QString& name_short, const QString& sys_type, bool is_tumor);
	//checks a QC collection and assignes a corresponding label: good, medium, bad, "" (if no rule matched) or n/a (if not all required QC terms are present)
	QString evaluate(const QCCollection& qc_data, const QString& sys_type, const QString& name_short, bool is_tumor);

private:
	QDomElement xml_root_;
	QSet<QString> used_qc_term_names_; //cache all used QC terms to improve runtime

	// Returns true if the given QCCollection has all the rules listed in the corresponding rule inside the config
	bool hasAllQcMetrics(const QCCollection& qc_data, const QString& name_short, const QString& sys_type, bool is_tumor);

	//Returns the rule set to be used for the given processing system
	QDomNodeList getRules(const QString& name_short, const QString& sys_type, bool is_tumor);

	bool matches(const QString& operation, double value, double cutoff);
	QString checkQcRule(const QString &sys_name, const QString &term_name, double value, bool is_tumor, bool needs_sys_override);
};

#endif // QCRULEMATCHER_H
