#include "DatabaseSchema.h"

static QStringList parseEnumValues(const QString& column_type)
{
	QStringList values;
	const int opening_bracket = column_type.indexOf('(');
	const int closing_bracket = column_type.lastIndexOf(')');

	if (opening_bracket < 0 || closing_bracket <= opening_bracket) return values;

	QString value;
	bool inside_string = false;

	for (int i = opening_bracket + 1; i < closing_bracket; ++i)
	{
		const QChar current = column_type[i];

		if (!inside_string)
		{
			if (current == '\'')
			{
				inside_string = true;
				value.clear();
			}
			continue;
		}

		if (current == '\\' && i + 1 < closing_bracket)
		{
			// handle MariaDB backslash escaping.
			value.append(column_type[++i]);
			continue;
		}

		if (current == '\'')
		{
			// SQL also supports escaping ' as ''.
			if (i + 1 < closing_bracket && column_type[i + 1] == '\'')
			{
				value.append('\'');
				i++;
				continue;
			}

			inside_string = false;
			values.append(value);
			continue;
		}

		value.append(current);
	}

	return values;
}

DatabaseSchema DatabaseSchema::loadFromDatabase(NGSD& db)
{
	DatabaseSchema result;
	try
	{
		SqlQuery query = db.getQuery();
		query.prepare("SELECT "
			"TABLE_NAME,"
			"COLUMN_NAME"
			" FROM INFORMATION_SCHEMA.COLUMNS"
			" WHERE TABLE_SCHEMA = :schema ORDER BY TABLE_NAME, ORDINAL_POSITION");

		query.bindValue(":schema", db.databaseName());
		query.exec();

		while (query.next())
		{
			const QString table_name = query.value(0).toString();
			const TableFieldInfo& field_info =  db.tableInfo(table_name).fieldInfo(query.value(1).toString());

			TableSchema& table = result.tables_[table_name];
			table.name = table_name;
			table.columns.insert(query.value(1).toString(), field_info);
		}
	}
	catch(DatabaseException& e)
	{
		Log::error("Failed to load the database schema: " + e.message());
	}

	return result;
}