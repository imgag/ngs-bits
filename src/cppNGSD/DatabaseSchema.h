#ifndef DATABASESCHEMA_H
#define DATABASESCHEMA_H

#include "cppNGSD_global.h"
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>
#include "Exceptions.h"
#include "NGSD.h"

struct CPPNGSDSHARED_EXPORT ColumnSchema
{
	QString name;

	// Examples:
	// data_type:   "int", "varchar", "text", "enum"
	// column_type: "int(10) unsigned", "varchar(40)",
	//             "enum('synonym','previous')"
	QString data_type;
	QString column_type;
	bool nullable = false;
	bool has_default = false;
	QVariant default_value;
	qint64 character_maximum_length = -1;
	int numeric_precision = -1;
	int numeric_scale = -1;
	bool auto_increment = false;
	bool generated = false;
	QString comment;
	QStringList enum_values;

	bool writable() const
	{
		return !auto_increment && !generated;
	}

	bool requiredForInsert() const
	{
		return writable() && !nullable && !has_default;
	}
};

struct CPPNGSDSHARED_EXPORT TableSchema
{
	QString name;
	QHash<QString, ColumnSchema> columns;
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
