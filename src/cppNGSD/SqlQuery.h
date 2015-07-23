#ifndef SQLQUERY_H
#define SQLQUERY_H

#include <QSqlDatabase>
#include <QSqlQuery>

class SqlQuery
	: public QSqlQuery
{
public:
		SqlQuery(QSqlDatabase db);
		void exec(const QString& query);
};

#endif // SQLQUERY_H
