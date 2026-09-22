#include "SqlQuery.h"
#include "Exceptions.h"
#include <QSqlError> //Comment to prevent removal by fix_includes.php
#include <QElapsedTimer>
#include <QScopedPointer>
#include "Helper.h"
#include <QSqlRecord>

SqlQuery::SqlQuery(QSqlDatabase db, bool debug)
	: QSqlQuery(db)
	, debug_(debug)
{
}

void SqlQuery::exec(const QString& query)
{
	QScopedPointer<QElapsedTimer> timer;
	if (debug_)
	{
		timer.reset(new QElapsedTimer());
		timer->start();
	}
	bool success = QSqlQuery::exec(query);
	if (debug_)
	{
		qDebug() << "SqlQuery::exec():" << lastQuery() << "took: " << Helper::elapsedTime(timer->elapsed()) << "success: " << success;
	}
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
	QScopedPointer<QElapsedTimer> timer;
	if (debug_)
	{
		timer.reset(new QElapsedTimer());
		timer->start();
	}
	bool success = QSqlQuery::exec();
	if (debug_)
	{
		qDebug() << "SqlQuery::exec():" << lastQuery() << "took: " << Helper::elapsedTime(timer->elapsed()) << "success: " << success;
	}
	if (!success)
	{
		THROW(DatabaseException, lastError().text() + "\nQuery: " + lastQuery());
	}
}

QStringList SqlQuery::headers() const
{
	QStringList output;

	QSqlRecord record_data = record();
	for (int i=0; i<record().count(); ++i)
	{
		output << record_data.fieldName(i);
	}

	return output;
}

QStringList SqlQuery::toTSV()
{
	QStringList output;

	//header
	output << "#" + headers().join('\t');

	//data
	while (next())
	{
		QStringList row;

		for (int i=0; i<record().count(); ++i)
		{
			QString field;
			if (!isNull(i))
			{
				field = value(i).toString();
				field.replace('\t', ' ');
				field.replace('\r', ' ');
				field.replace('\n', ' ');
			}
			row << field;
		}

		output << row.join('\t');
	}

	return output;
}
