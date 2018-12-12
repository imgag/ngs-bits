#include "DBTable.h"
#include <QSqlDriver>
#include <QSqlRecord>
#include <QSqlField>
#include <QDebug>

DBTable::DBTable()
{

}

QString DBTable::tableName() const
{
	return table_name_;
}

void DBTable::setTableName(const QString& table_name)
{
	table_name_ = table_name;
}

const QStringList& DBTable::headers() const
{
	return headers_;
}

void DBTable::setHeaders(const QStringList& headers)
{
	headers_ = headers;
}

DBRow::DBRow()
{

}

const QString& DBRow::id() const
{
	return id_;
}

void DBRow::setId(const QString& id)
{
	id_ = id;
}

const QStringList& DBRow::values() const
{
	return values_;
}

void DBRow::setValues(const QStringList& values)
{
	values_ = values;
}
