#ifndef SQLQUERY_H
#define SQLQUERY_H

#include "cppNGSD_global.h"
#include <QSqlDatabase>
#include <QSqlQuery>

///Wrapper for QSqlQuery that performs error checks when executing a query
class CPPNGSDSHARED_EXPORT SqlQuery
	: public QSqlQuery
{
public:
		SqlQuery(QSqlDatabase db);
		SqlQuery(const SqlQuery& query);

        //Declare assignment operator as explicitly defaulted (implicit generation is not allowed because of explicit copy constructor)
        SqlQuery& operator=(const SqlQuery& other) = default;

		void exec(const QString& query);
		void prepare(const QString& query);
		void exec();
};

#endif // SQLQUERY_H
