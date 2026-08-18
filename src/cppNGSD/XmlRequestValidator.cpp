#include "XmlRequestValidator.h"
#include <QSet>
#include <QXmlStreamReader>
#include <limits>

XmlRequestValidator::XmlRequestValidator(const DatabaseSchema& schema)
	: schema_(schema)
{
}

XmlValidationResult XmlRequestValidator::validateInsert(const QByteArray& body, const QString& tableName) const
{
	XmlValidationResult result;
	constexpr qsizetype MAX_XML_SIZE = 1024 * 1024;

	if (body.isEmpty())
	{
		result.errors << "Request body is empty";
		return result;
	}

	if (body.size() > MAX_XML_SIZE)
	{
		result.errors << "XML request exceeds maximum allowed size";
		return result;
	}

	const TableSchema& table = schema_.table(tableName);

	QXmlStreamReader xml(body);
	if (!xml.readNextStartElement())
	{
		result.errors << (xml.hasError() ? xml.errorString() : QString("XML contains no root element"));
		return result;
	}

	if (xml.name() != tableName)
	{
		result.errors << QString("Expected root element <%1>, found <%2>").arg(tableName, xml.name().toString());
		return result;
	}

	if (!xml.attributes().isEmpty())
	{
		result.errors << QString("Attributes on <%1> are not allowed").arg(tableName);
	}

	QSet<QString> seen_fields;

	while (xml.readNextStartElement())
	{
		const QString fieldName = xml.name().toString();
		const auto column_it = table.columns.constFind(fieldName);

		if (column_it == table.columns.constEnd())
		{
			result.errors << QString("Unknown field <%1>").arg(fieldName);
			xml.skipCurrentElement();
			continue;
		}

		const ColumnSchema& column = column_it.value();

		if (seen_fields.contains(fieldName))
		{
			result.errors << QString("Field <%1> occurs more than once").arg(fieldName);
			xml.skipCurrentElement();
			continue;
		}

		seen_fields.insert(fieldName);

		if (!column.writable())
		{
			result.errors << QString("Field <%1> cannot be supplied").arg(fieldName);
			xml.skipCurrentElement();
			continue;
		}

		if (!xml.attributes().isEmpty())
		{
			result.errors << QString("Attributes on <%1> are not allowed").arg(fieldName);
		}

		const QString value = xml.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
		if (xml.hasError()) break;
		const ConvertedValue converted = convertValue(value, column);

		if (!converted.valid)
		{
			result.errors << converted.error;
			continue;
		}

		result.values.insert(fieldName, converted.value);
	}

	if (xml.hasError())
	{
		result.errors << QString("Malformed XML at line %1, column %2: %3").arg(xml.lineNumber()).arg(xml.columnNumber()).arg(xml.errorString());
		return result;
	}

	// Check mandatory database columns.
	for (auto it = table.columns.constBegin(); it != table.columns.constEnd(); it++)
	{
		const ColumnSchema& column = it.value();
		if (column.requiredForInsert() && !seen_fields.contains(column.name))
		{
			result.errors << QString("Required field <%1> is missing").arg(column.name);
		}
	}
	return result;
}

XmlRequestValidator::ConvertedValue XmlRequestValidator::convertValue(const QString &text, const ColumnSchema &column) const
{
	ConvertedValue result;
	const QString type = column.data_type.toLower();

	if (type == "char" || type == "varchar" || type == "tinytext" || type == "text" || type == "mediumtext" || type == "longtext")
	{
		if (column.character_maximum_length >= 0 && text.size() > column.character_maximum_length)
		{
			result.error = QString("Field '%1' exceeds maximum length of %2 characters").arg(column.name).arg(column.character_maximum_length);
			return result;
		}

		result.valid = true;
		result.value = text;
		return result;
	}

	if (type == "enum")
	{
		if (!column.enum_values.contains(text))
		{
			result.error = QString("Invalid value '%1' for field '%2'. Allowed values: %3").arg(text, column.name, column.enum_values.join(", "));
			return result;
		}

		result.valid = true;
		result.value = text;
		return result;
	}

	if (type == "tinyint" && column.column_type.startsWith("tinyint(1)"))
	{
		if (text == "0" || text.compare("false", Qt::CaseInsensitive) == 0)
		{
			result.valid = true;
			result.value = 0;
			return result;
		}

		if (text == "1" || text.compare("true", Qt::CaseInsensitive) == 0)
		{
			result.valid = true;
			result.value = 1;
			return result;
		}

		result.error = QString("Field '%1' must be 0, 1, true or false").arg(column.name);
		return result;
	}

	if (type == "int" || type == "integer")
	{
		const bool isUnsigned = column.column_type.contains("unsigned", Qt::CaseInsensitive);

		if (isUnsigned)
		{
			bool ok = false;
			const qulonglong value = text.toULongLong(&ok);

			if (!ok || value > std::numeric_limits<quint32>::max())
			{
				result.error = QString("Field '%1' must be an unsigned 32-bit integer").arg(column.name);
				return result;
			}

			result.valid = true;
			result.value = QVariant::fromValue(value);
			return result;
		}

		bool ok = false;

		const qlonglong value = text.toLongLong(&ok);

		if (!ok || value < std::numeric_limits<qint32>::min() || value > std::numeric_limits<qint32>::max())
		{
			result.error = QString("Field '%1' must be a signed 32-bit integer").arg(column.name);
			return result;
		}

		result.valid = true;
		result.value = QVariant::fromValue(value);
		return result;
	}

	result.error = QString("Unsupported database type '%1' for field '%2'").arg(column.column_type, column.name);
	return result;
}