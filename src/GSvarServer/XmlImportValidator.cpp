#include "XmlImportValidator.h"
#include <QSet>
#include <QXmlStreamReader>
#include <limits>

XmlImportValidator::XmlImportValidator(bool test_db)
	: test_db_(test_db)
{
}

XmlValidationResult XmlImportValidator::validateInsert(const QByteArray& body, const QString& table_name) const
{
	NGSD db(test_db_);
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

	// check if the table exists
	SqlQuery query = db.getQuery();
	query.prepare("SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA=:schema AND TABLE_NAME=:table_name");
	query.bindValue(":schema", db.databaseName());
	query.bindValue(":table_name", table_name);
	query.exec();
	if (!query.next())
	{
		result.errors << "The table '"+table_name+"' does not exist in the database";
		return result;
	}

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

	const TableInfo table_info = db.tableInfo(table_name);

	while (xml.readNextStartElement())
	{
		const QString field_name = xml.name().toString();
		if (!table_info.fieldNames().contains(field_name))
		{
			result.errors << QString("Unknown field <%1>").arg(field_name);
			xml.skipCurrentElement();
			continue;
		}


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

		const TableFieldInfo& column = table_info.fieldInfo(field_name);
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
	for (QString filed_name: table_info.fieldNames())
	{
		const TableFieldInfo& column = table_info.fieldInfo(filed_name);

		bool mandatory_field = true;
		if ((column.is_primary_key && column.has_auto_increment) || (column.is_nullable) || (!column.default_value.isEmpty())) mandatory_field = false;
		if (mandatory_field && !seen_fields.contains(column.name)) result.errors << QString("Required field <%1> is missing").arg(column.name);
	}
	return result;
}