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
		THROW(DatabaseException, "SqlQuery error!\nQuery: " + lastQuery() + "\nError: " + lastError().text());
	}
}

void SqlQuery::exec()
{
	bool success = QSqlQuery::exec();
	if (!success)
	{
		THROW(DatabaseException, "SqlQuery error!\nQuery: " + lastQuery() + "\nError: " + lastError().text());
	}
}
