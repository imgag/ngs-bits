#include "DatabaseInsert.h"
#include <QStringList>

static QString quoteIdentifier(const QString& identifier)
{
	QString escaped = identifier;
	escaped.replace('`', "``");

	return "`" + escaped + "`";
}


void DatabaseInsert::insert(NGSD& db, const TableSchema& table, const QHash<QString, QVariant>& values)
{	
	if (values.isEmpty()) THROW(DatabaseException, "No values supplied for INSERT");

	QStringList column_names;
	QStringList placeholders;
	QList<QString> keys;
	int index = 0;

	for (auto it = values.constBegin(); it != values.constEnd(); it++)
	{
		const QString& column_name = it.key();

		// The column must actually exist in the database schema.
		const auto column_it = table.columns.constFind(column_name);
		if (column_it == table.columns.constEnd()) THROW(DatabaseException, QString("Unknown column '%1' for table '%2'").arg(column_name, table.name));
		const ColumnSchema& column = column_it.value();

		// Do not allow values for auto_increment/generated columns.
		if (!column.writable()) THROW(DatabaseException, QString("Column '%1' is not writable").arg(column_name));

		keys.append(column_name);
		column_names.append(quoteIdentifier(column_name));
		placeholders.append("'" + values[column_name].toString() + "'");
		index++;
	}

	SqlQuery query = db.getQuery();	
	query.exec(QString("INSERT INTO %1 (%2) VALUES (%3)").arg(quoteIdentifier(table.name), column_names.join(", "), placeholders.join(", ")));	
}