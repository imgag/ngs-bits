#ifndef QCRULEMATCHER_H
#define QCRULEMATCHER_H

#include "cppNGS_global.h"
#include "XmlHelper.h"
#include "QCCollection.h"
#include <QString>
#include <QSet>

// This class is needed to handle QC cutoff values globally for GSvar: instead of having hardcoded values in several places across the repository
// now we can store rules for different QC terms in a single XML file (which is validated against a schema automatically), based on
// the QC values the system decides which quality should be assigned (good, medium, bad)
class CPPNGSSHARED_EXPORT QcRuleMatcher
{
public:
	// Default constructor where we read the XML and cache it
	QcRuleMatcher(QString cutoff_config_file);
	// Checks a given QC term against the rules and assignes a corresponding label: good, medium, bad or n\a
	QString evaluate(const QString& name_short, const QString& sys_type, const QString& term_name, double value, bool is_tumor);
	// Checks all QCs in given QC collection and assignes a corresponding label: good, medium, bad or n\a
	QString evaluate(const QString& name_short, const QString& sys_type, const QCCollection& qc_data, bool is_tumor);
	// Returns true if the XML config has been cached
	bool hasRules();
	// Returns true if the given QCCollection has all the rules listed in the corresponding rule inside the config
	bool hasAllQcMetrics(QSet<QString>& term_names, const QString& rule_type, bool needs_sys_override, bool is_tumor);

private:
	QDomElement xml_root_; // caching the XML config to avoid reading it for each rule validation

	bool matches(const QString& operation, double value, double cutoff);
	QString checkQcRule(const QString &sys_name, const QString &term_name, double value, QString expected_tumor_value, bool needs_sys_override);	
	bool has_rules_;
};

#endif // QCRULEMATCHER_H
