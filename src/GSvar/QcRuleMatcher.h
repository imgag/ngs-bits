#ifndef QCRULEMATCHER_H
#define QCRULEMATCHER_H

#include <QDomDocument>
#include <QString>
#include <QDebug>

class QcRuleMatcher
{
public:
	static QString evaluate(const QString& name_short, const QString& sys_type, const QString& term_name, double value, bool is_tumor);


private:
	static bool matches(const QString& operation, double value, double cutoff);
	static QString checkQcRule(const QString &sys_name, const QString &term_name, double value, QString expected_tumor_value, bool needs_sys_override);
	QcRuleMatcher();
	static QcRuleMatcher& instance();
	QDomDocument xml_doc_;
};

#endif // QCRULEMATCHER_H
