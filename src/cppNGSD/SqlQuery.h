#ifndef SQLQUERY_H
#define SQLQUERY_H

#include "cppNGSD_global.h"
#include <QSqlDatabase>
#include <QSqlQuery>

class CPPNGSDSHARED_EXPORT SqlQuery
	: public QSqlQuery
{
public:
		SqlQuery(QSqlDatabase db);
		void exec(const QString& query);
};

#endif // SQLQUERY_H
