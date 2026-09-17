#include "XmlImportValidator.h"
#include <QSet>
#include <QXmlStreamReader>
#include <limits>

XmlImportValidator::XmlImportValidator(const DatabaseSchema& schema)
	: schema_(schema)
{
}

XmlValidationResult XmlImportValidator::validateInsert(const QByteArray& body, const QString& table_name) const
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

	const TableSchema& table = schema_.table(table_name);

	QXmlStreamReader xml(body);
	if (!xml.readNextStartElement())
	{
		result.errors << (xml.hasError() ? xml.errorString() : QString("XML contains no root element"));
		return result;
	}

	if (xml.name() != table_name)
	{
		result.errors << QString("Expected root element <%1>, found <%2>").arg(table_name, xml.name().toString());
		return result;
	}

	if (!xml.attributes().isEmpty())
	{
		result.errors << QString("Attributes on <%1> are not allowed").arg(table_name);
	}

	QSet<QString> seen_fields;

	while (xml.readNextStartElement())
	{
		const QString field_name = xml.name().toString();
		const auto column_it = table.columns.constFind(field_name);

		if (column_it == table.columns.constEnd())
		{
			result.errors << QString("Unknown field <%1>").arg(field_name);
			xml.skipCurrentElement();
			continue;
		}

		const TableFieldInfo& column = column_it.value();

		if (seen_fields.contains(field_name))
		{
			result.errors << QString("Field <%1> occurs more than once").arg(field_name);
			xml.skipCurrentElement();
			continue;
		}
		seen_fields.insert(field_name);


		if (!xml.attributes().isEmpty())
		{
			result.errors << QString("Attributes on <%1> are not allowed").arg(field_name);
		}

		const QString value = xml.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement);
		if (xml.hasError()) break;

		if (!column.isValid(value))
		{

			result.errors << QString("Invalid value in the '%1'").arg(column.name);
			continue;
		}

		result.values.insert(field_name, value);
	}

	if (xml.hasError())
	{
		result.errors << QString("Malformed XML at line %1, column %2: %3").arg(xml.lineNumber()).arg(xml.columnNumber()).arg(xml.errorString());
		return result;
	}

	// Check mandatory database columns
	for (auto it = table.columns.constBegin(); it != table.columns.constEnd(); it++)
	{
		const TableFieldInfo& column = it.value();

		bool mandatory_field = true;
		if ((column.is_primary_key && column.has_auto_increment) || (column.is_nullable) || (!column.default_value.isEmpty())) mandatory_field = false;
		if (mandatory_field && !seen_fields.contains(column.name)) result.errors << QString("Required field <%1> is missing").arg(column.name);
	}
	return result;
}