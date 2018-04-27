#include "DatabaseCache.h"
#include <QSqlError>
#include "NGSD.h"
#include "Exceptions.h"

DatabaseCache& DatabaseCache::inst()
{
	static DatabaseCache cache;
	return cache;
}

NGSD& DatabaseCache::ngsd()
{
	return db_;
}

QSharedPointer<DatabaseTableInfo> DatabaseCache::getInfos(QString table)
{
	QSharedPointer<DatabaseTableInfo> output = infos_.value(table);
	if (output.isNull())
	{
		output.reset(new DatabaseTableInfo());
		output->name = table;
		SqlQuery query = ngsd().getQuery();
		query.exec("DESCRIBE " + table);
		while(query.next())
		{
			QString field = query.value(0).toString();
			if (field=="id") continue;

			output->fields.append(field);

			DatabaseFieldInfo info;
			//index
			info.index = output->field_info.count();
			//type
			QString type = query.value(1).toString();
			if(type=="text") info.type = DatabaseFieldInfo::TEXT;
			else if(type=="float") info.type = DatabaseFieldInfo::FLOAT;
			else if(type=="date") info.type = DatabaseFieldInfo::DATE;
			else if(type=="tinyint(1)") info.type = DatabaseFieldInfo::BOOL;
			else if(type.startsWith("int(") || type.startsWith("tinyint(")) info.type = DatabaseFieldInfo::INT;
			else if(type.startsWith("enum("))
			{
				info.type = DatabaseFieldInfo::ENUM;
				info.type_restiction = type.mid(6, type.length()-8).split("','");
			}
			else if(type.startsWith("varchar("))
			{
				info.type = DatabaseFieldInfo::VARCHAR;
				info.type_restiction = type.mid(8, type.length()-9);
			}
			//nullable
			info.nullable = query.value(2).toString()=="YES";
			//FK
			if (table=="processed_sample")
			{
				if(field=="sample_id")
				{
					info.fk_table = "sample";
					info.fk_field = "name";
					info.fk_label = "sample";
					info.type = DatabaseFieldInfo::FK;
				}
				else if (field=="sequencing_run_id")
				{
					info.fk_table = "sequencing_run";
					info.fk_field = "name";
					info.fk_label = "run";
					info.type = DatabaseFieldInfo::FK;
				}
				else if (field=="mid1_i7")
				{
					info.fk_table = "mid";
					info.fk_field = "name";
					info.fk_label = "mid i7";
					info.type = DatabaseFieldInfo::FK;
				}
				else if (field=="mid2_i5")
				{
					info.fk_table = "mid";
					info.fk_field = "name";
					info.fk_label = "mid i5";
					info.type = DatabaseFieldInfo::FK;
				}
				else if (field=="operator_id")
				{
					info.fk_table = "user";
					info.fk_field = "name";
					info.fk_label = "operator";
					info.type = DatabaseFieldInfo::FK;
				}
				else if (field=="processing_system_id")
				{
					info.fk_table = "processing_system";
					info.fk_field = "name_manufacturer";
					info.fk_label = "processing_system";
					info.type = DatabaseFieldInfo::FK;
				}
				else if (field=="project_id")
				{
					info.fk_table = "project";
					info.fk_field = "name";
					info.fk_label = "project";
					info.type = DatabaseFieldInfo::FK;
				}
				else if (field=="normal_id")
				{
					info.fk_table = "processed_sample";
					info.fk_field = "process_id";
					info.fk_label = "normal sample";
					info.type = DatabaseFieldInfo::FK;
				}
			}
			output->field_info.insert(field, info);
		}

		infos_.insert(table, output);
	}

	return output;
}

DatabaseCache::DatabaseCache()
	: db_()
	, infos_()
{
}



DatabaseFieldInfo::DatabaseFieldInfo()
	: index(-1)
{

}
