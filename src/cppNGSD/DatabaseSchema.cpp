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
			"COLUMN_NAME,"
			"DATA_TYPE,"
			"COLUMN_TYPE,"
			"IS_NULLABLE,"
			"COLUMN_DEFAULT,"
			"CHARACTER_MAXIMUM_LENGTH,"
			"NUMERIC_PRECISION,"
			"NUMERIC_SCALE,"
			"EXTRA,"
			"COLUMN_COMMENT,"
			"GENERATION_EXPRESSION"
			" FROM INFORMATION_SCHEMA.COLUMNS"
			" WHERE TABLE_SCHEMA = :schema ORDER BY TABLE_NAME, ORDINAL_POSITION");

		query.bindValue(":schema", db.databaseName());
		query.exec();

		while (query.next())
		{
			const QString table_name = query.value(0).toString();
			ColumnSchema column;
			column.name = query.value(1).toString();
			column.data_type = query.value(2).toString().toLower();
			column.column_type = query.value(3).toString().toLower();
			column.nullable = query.value(4).toString().compare("YES", Qt::CaseInsensitive) == 0;
			column.has_default = !query.value(5).isNull();
			if (column.has_default) column.default_value = query.value(5);
			if (!query.value(6).isNull()) column.character_maximum_length = query.value(6).toLongLong();
			if (!query.value(7).isNull()) column.numeric_precision = query.value(7).toInt();
			if (!query.value(8).isNull()) column.numeric_scale = query.value(8).toInt();
			const QString extra = query.value(9).toString();
			column.auto_increment = extra.contains("auto_increment", Qt::CaseInsensitive);

			column.comment = query.value(10).toString();

			const QString generation_expression = query.value(11).toString();
			column.generated = !generation_expression.isEmpty();
			if (column.data_type == "enum") column.enum_values = parseEnumValues(column.column_type);

			TableSchema& table = result.tables_[table_name];
			table.name = table_name;
			table.columns.insert(column.name, column);
		}
	}
	catch(DatabaseException& e)
	{
		Log::error(e.message());
	}

	return result;
}