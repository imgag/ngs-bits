#include "GDBO.h"
#include "Exceptions.h"
#include "NGSD.h"
#include <QSqlError>
#include <QSqlRecord>

GDBO::GDBO(const QString& table)
	: id_(-1)
	, table_info_()
	, fields_()
{
	table_info_ = DatabaseCache::inst().getInfos(table);
	fields_.resize(table_info_->fields.count());
}

GDBO::GDBO(const QString& table, int id)
	: id_(id)
	, table_info_()
	, fields_()
{
	SqlQuery query = DatabaseCache::inst().ngsd().getQuery();
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
	, table_info_()
	, fields_()
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
	table_info_ = DatabaseCache::inst().getInfos(table);

	for (int i=0; i<fieldNames().count(); ++i)
	{
		const QVariant& value = query.value(i+1); //Why i+1? first field is "id", which is stored in id_ already

		fields_.append(value.isNull() ? "" : value.toString());
	}
}

GDBO GDBO::getFkObject(const QString& name) const
{
	QString fk_table = table_info_->field_info.value(name).fk_table;
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
	QString fk_table = table_info_->field_info.value(name).fk_table;
	if (fk_table=="")
	{
		THROW(ProgrammingException, "Unset FK table for field '" + name + "' in table '" + table() + "'!");
	}
	QString fk_field = table_info_->field_info.value(name).fk_field;

	QVariant id = DatabaseCache::inst().ngsd().getValue("SELECT id FROM " + fk_table + " WHERE " + fk_field + "='" + value.toString() + "'", true);
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
	QMap<QString, DatabaseFieldInfo>::ConstIterator it = table_info_->field_info.begin();
	while(it!=table_info_->field_info.end())
	{
		QString field = it.key();
		QVariant value = fields_[it->index];
		if (!set_part.isEmpty()) set_part += ", ";
		if (it->nullable && value=="")
		{
			set_part += field + "=" + (new_row ? "DEFAULT" : "null");
		}
		else
		{
			set_part += field + "='" + value.toString() + "'";
		}

		++it;
	}

	SqlQuery query = DatabaseCache::inst().ngsd().getQuery();
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
	SqlQuery query = DatabaseCache::inst().ngsd().getQuery();
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

int GDBO::indexOf(const QString& name) const
{
	int index = table_info_->field_info.value(name).index;
	if (index==-1)
	{
		THROW(ProgrammingException, "Unknown field '" + name + "' requested in table '" + table() + "'!");
	}
	return index;
}
