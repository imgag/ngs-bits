#ifndef DATABASESCHEMA_H
#define DATABASESCHEMA_H

#include "cppNGSD_global.h"
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>
#include "Exceptions.h"
#include "NGSD.h"

struct CPPNGSDSHARED_EXPORT TableSchema
{
	QString name;
	QHash<QString, TableFieldInfo> columns;
};

class CPPNGSDSHARED_EXPORT DatabaseSchema
{
public:
	static DatabaseSchema loadFromDatabase(NGSD& db);
	const TableSchema& table(const QString& name) const
	{
		const auto it = tables_.constFind(name);
		if (it == tables_.constEnd()) THROW(DatabaseException, QString("Table '%1' does not exist").arg(name));
		return it.value();
	}

private:
	QHash<QString, TableSchema> tables_;
};

#endif // DATABASESCHEMA_H
