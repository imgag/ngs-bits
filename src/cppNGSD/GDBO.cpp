#include "GDBO.h"
#include "Exceptions.h"
#include "NGSD.h"
#include <QSqlError>
#include <QSqlRecord>

GDBO::GDBO(const QString& table)
	: id_(-1)
{
	table_infos_ = NGSD().tableInfo(table);
}

GDBO::GDBO(const QString& table, int id)
	: id_(id)
	, table_infos_()
{
	NGSD db;
	SqlQuery query = db.getQuery();
	query.exec("SELECT * FROM " + table + " WHERE id=" + QString::number(id_));
	if (query.size()!=1)
	{
		THROW(DatabaseException, "Zero rows received for NGSD query:\n" + query.lastQuery());
	}

	//load fields
	query.next();
	init(table, query);
}

GDBO::GDBO(const QString& table, const SqlQuery& query)
	: id_(-1)
	, table_infos_()
{
	//get ID
	bool ok = true;
	id_ = query.value(0).toInt(&ok);
	if (!ok)
	{
		THROW(DatabaseException, "Could not convert ID to integer for query:\n" + query.lastQuery());
	}

	//load fields
	 init(table, query);
}

void GDBO::init(const QString& table, const SqlQuery& query)
{
	table_infos_ = NGSD().tableInfo(table);

	foreach(const auto& info, table_infos_.fieldInfo())
	{
		QVariant value = query.value(info.name);
		fields_[info.name] = value.isNull() ? "" : value.toString();
	}
}

GDBO GDBO::getFkObject(const QString& name) const
{
	QString fk_table =  table_infos_.fieldInfo(name).fk_table;
	if (fk_table=="")
	{
		THROW(ProgrammingException, "Unset FK table for field '" + name + "' in table '" + table() + "'!");
	}
	bool ok = true;
	int id = get(name).toInt(&ok);
	if (!ok)
	{
		THROW(ProgrammingException, "Could not convert FK id '" + get(name) + "' of field '" + name + "' in table '" + table() + "' to integer!");
	}
	return GDBO(fk_table, id);
}

void GDBO::setFK(const QString& name, const QVariant& value)
{
	QString fk_table = table_infos_.fieldInfo(name).fk_table;
	if (fk_table=="")
	{
		THROW(ProgrammingException, "Unset FK table for field '" + name + "' in table '" + table() + "'!");
	}
	QString fk_field = table_infos_.fieldInfo(name).fk_field;

	QVariant id = NGSD().getValue("SELECT id FROM " + fk_table + " WHERE " + fk_field + "='" + value.toString() + "'", true);
	if (id.isNull())
	{
		THROW(ProgrammingException, "FK id could not be found in table '" + fk_table + "' with field '" + fk_field + "'='" + value.toString() + "'!");
	}

	set(name, id.toString());
}

void GDBO::store()
{
	bool new_row = (id()==-1);

	//extract name-value pairs
	QString set_part;
	foreach(const TableFieldInfo& info, table_infos_.fieldInfo())
	{
		if (info.primary_key) continue;

		QString value = fields_[info.name];
		if (!set_part.isEmpty()) set_part += ", ";
		if (info.nullable && value=="")
		{
			set_part += info.name + "=" + (new_row ? "DEFAULT" : "null");
		}
		else
		{
			set_part += info.name + "='" + value + "'";
		}
	}

	NGSD db;
	SqlQuery query = db.getQuery();
	if (new_row)
	{
		QString query_text = "INSERT INTO " + table() + " SET " + set_part;
		query.exec(query_text);

		//set ID
		bool ok = false;
		id_ = query.lastInsertId().toInt(&ok);
		if (!ok)
		{
			THROW(DatabaseException, "Could not convert last insert ID '" + query.lastInsertId().toString() + "' to int!\nQuery: " + query.lastQuery());
		}
	}
	else
	{
		QString query_text = "UPDATE " + table() + " SET " + set_part + " WHERE id='" + QString::number(id()) + "'";
		query.exec(query_text);
	}
}

QList<GDBO> GDBO::all(QString table, QStringList conditions)
{
	NGSD db;
	SqlQuery query = db.getQuery();
	QString cond = conditions.isEmpty() ? "" : " WHERE " + conditions.join(" AND ");
	query.exec("SELECT * FROM " + table + cond);

	QList<GDBO> output;
	output.reserve(query.size());
	while(query.next())
	{
		output.append(GDBO(table, query));
	}

	return output;
}
