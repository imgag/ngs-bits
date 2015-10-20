#ifndef GDBO_H
#define GDBO_H

#include "cppNGSD_global.h"
#include "DatabaseCache.h"
#include <QVariant>
#include <QVector>

///Generic database object.
class CPPNGSDSHARED_EXPORT GDBO
{
public:
	///Constructor for obejct that does not yet exists
	GDBO(const QString& table);
	///Constructor for existing object (defined by id)
	GDBO(const QString& table, int id);
	///Constructor for existing object (defined by a query that is positioned at the right row)
	GDBO(const QString& table, const SqlQuery& query);

	///Returns the database ID
	int id() const
	{
		return id_;
	}
	///Returns the database table name
	const QString& table() const
	{
		return table_info_->name;
	}
	///Returns the field name list of the table
	const QStringList& fieldNames() const
	{
		return table_info_->fields;
	}
	///Returns the field information about a field
	const DatabaseFieldInfo& fieldInfo(const QString& name) const
	{
		return table_info_->field_info[name];
	}

	///Returns a non-null string value.
	QString get(const QString& name) const
	{
		return fields_[indexOf(name)];
	}
	///Set the value of a field.
	void set(const QString& name, const QString& value)
	{
		fields_[indexOf(name)] = value;
	}

	///Returns the non-null object a FK-field points to.
	GDBO getFkObject(const QString& name) const;

	///Set a FK-field by a value instead of by id.
	void setFK(const QString& name, const QVariant& value);

	///Stores the object to the database.
	void store();

	///Return all object of the table that match the SQL conditions.
	static QList<GDBO> all(QString table, QStringList conditions = QStringList());

protected:
	int id_;
	QSharedPointer<DatabaseTableInfo> table_info_;
	QVector<QString> fields_;

	void init(const QString& table, const SqlQuery& query);
	int indexOf(const QString& name) const;
};

#endif // GDBO_H
