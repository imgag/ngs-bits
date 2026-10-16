#ifndef XMLIMPORTVALIDATOR_H
#define XMLIMPORTVALIDATOR_H

#include "NGSD.h"
#include <QByteArray>
#include <QHash>
#include <QStringList>
#include <QVariant>

struct XmlValidationResult
{
	QHash<QString, QVariant> values;
	QStringList errors;
	QString errorsAsString()
	{
		return errors.join(", ");
	}

	bool isValid() const
	{
		return errors.isEmpty();
	}
};


class XmlImportValidator
{
public:
	explicit XmlImportValidator(bool test_db = false);
	XmlValidationResult validateInsert(const QByteArray& xml, const QString& table_name) const;

private:
	bool test_db_;
};

#endif // XMLIMPORTVALIDATOR_H