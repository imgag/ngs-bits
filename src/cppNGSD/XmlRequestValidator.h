#ifndef XMLREQUESTVALIDATOR_H
#define XMLREQUESTVALIDATOR_H

#include "DatabaseSchema.h"
#include "cppNGSD_global.h"
#include <QByteArray>
#include <QHash>
#include <QStringList>
#include <QVariant>

struct CPPNGSDSHARED_EXPORT XmlValidationResult
{
	QHash<QString, QVariant> values;
	QStringList errors;

	bool isValid() const
	{
		return errors.isEmpty();
	}
};


class CPPNGSDSHARED_EXPORT XmlRequestValidator
{
public:
	explicit XmlRequestValidator(const DatabaseSchema& schema);
	XmlValidationResult validateInsert(const QByteArray& xml, const QString& tableName) const;

private:
	struct ConvertedValue
	{
		bool valid = false;
		QVariant value;
		QString error;
	};

	ConvertedValue convertValue(const QString& text, const ColumnSchema& column) const;
	const DatabaseSchema& schema_;
};

#endif // XMLREQUESTVALIDATOR_H
