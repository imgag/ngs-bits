#ifndef DBTABLE_H
#define DBTABLE_H

#include "cppNGSD_global.h"
#include <QString>
#include <QList>

//Database row with identifier and values
class CPPNGSDSHARED_EXPORT DBRow
{
	public:
		DBRow();

		const QString& id() const;
		void setId(const QString& id);

		const QStringList& values() const;
		void setValues(const QStringList& values);

protected:
		QString id_;
		QStringList values_;
};

//Database table
class CPPNGSDSHARED_EXPORT DBTable
		: public QList<DBRow>
{
	public:
		DBTable();

		QString tableName() const;
		void setTableName(const QString& table_name);

		const QStringList& headers() const;
		void setHeaders(const QStringList& headers);

	protected:
		QString table_name_;
		QStringList headers_;
};

#endif // DBTABLE_H
