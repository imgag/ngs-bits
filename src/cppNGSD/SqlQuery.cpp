#include <QSqlError>
#include "SqlQuery.h"
#include "Exceptions.h"

SqlQuery::SqlQuery(QSqlDatabase db)
	: QSqlQuery(db)
{
}

void SqlQuery::exec(const QString& query)
{
	bool success = QSqlQuery::exec(query);
	if (!success)
	{
		THROW(DatabaseException, lastError().text() + "\nQuery: " + lastQuery());
	}
}

void SqlQuery::prepare(const QString& query)
{
	bool success = QSqlQuery::prepare(query);
	if (!success)
	{
		THROW(DatabaseException, lastError().text() + "\nQuery: " + lastQuery());
	}
}

void SqlQuery::exec()
{
	bool success = QSqlQuery::exec();
	if (!success)
	{
		THROW(DatabaseException, lastError().text() + "\nQuery: " + lastQuery());
	}
}
