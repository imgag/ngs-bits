#ifndef DATABASESCHEMA_H
#define DATABASESCHEMA_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>

struct ColumnSchema
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

struct TableSchema
{
	QString name;
	QHash<QString, ColumnSchema> columns;
};

class DatabaseSchema
{
public:
	static DatabaseSchema loadFromDatabase();

	const TableSchema* table(const QString& name) const
	{
		const auto it = tables_.constFind(name);
		return it == tables_.constEnd() ? nullptr : &it.value();
	}

private:
	QHash<QString, TableSchema> tables_;
};

#endif // DATABASESCHEMA_H
