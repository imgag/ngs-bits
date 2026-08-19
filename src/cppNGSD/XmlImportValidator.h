#ifndef XMLIMPORTVALIDATOR_H
#define XMLIMPORTVALIDATOR_H

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


class CPPNGSDSHARED_EXPORT XmlImportValidator
{
public:
	explicit XmlImportValidator(const DatabaseSchema& schema);
	XmlValidationResult validateInsert(const QByteArray& xml, const QString& table_name) const;

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

#endif // XMLIMPORTVALIDATOR_H
