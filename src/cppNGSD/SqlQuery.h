#ifndef SQLQUERY_H
#define SQLQUERY_H

#include "cppNGSD_global.h"
#include <QSqlDatabase>
#include <QSqlQuery>

///Wrapper for SQL query that performs error checks when executing
class CPPNGSDSHARED_EXPORT SqlQuery
	: public QSqlQuery
{
public:
		SqlQuery(QSqlDatabase db);
		void exec(const QString& query);
		void exec();
};

#endif // SQLQUERY_H
