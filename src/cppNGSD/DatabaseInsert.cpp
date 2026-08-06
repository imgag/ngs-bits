#include "DatabaseInsert.h"
#include <QStringList>

static QString quoteIdentifier(const QString& identifier)
{
	QString escaped = identifier;
	escaped.replace('`', "``");

	return "`" + escaped + "`";
}


void DatabaseInsert::insert(const TableSchema& table, const QHash<QString, QVariant>& values, qlonglong* inserted_id)
{
	NGSD db;
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
		placeholders.append(QString(":value_%1").arg(index));
		index++;
	}

	SqlQuery query = db.getQuery();
	query.prepare(QString("INSERT INTO %1 (%2) VALUES (%3)").arg(quoteIdentifier(table.name), column_names.join(", "), placeholders.join(", ")));

	for (int i = 0; i < keys.count(); i++) query.bindValue(QString(":value_%1").arg(i), values.value(keys[i]));

	query.exec();
	if (inserted_id != nullptr) *inserted_id = query.lastInsertId().toLongLong();
}