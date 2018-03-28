#ifndef DATABASECACHE_H
#define DATABASECACHE_H

#include "cppNGSD_global.h"
#include "NGSD.h"
#include <QString>
#include <QMap>
#include <QSharedPointer>
#include <QSqlQuery>

///General database field information that is shared between all objects.
struct CPPNGSDSHARED_EXPORT DatabaseFieldInfo
{
	enum Type
	{
		BOOL, INT, FLOAT, TEXT, VARCHAR, ENUM, DATE, FK
	};

	DatabaseFieldInfo();

	int index;
	Type type;
	QVariant type_restiction; //length of VARCHAR and value of ENUM
	bool nullable;
	QString default_value;
	QString fk_table;
	QString fk_field;
	QString fk_label;
};

///General database table information that is shared between all objects.
struct CPPNGSDSHARED_EXPORT DatabaseTableInfo
{
	QString name;
	QStringList fields;
	QMap<QString, DatabaseFieldInfo> field_info;
};

///Singleton database cache that creates/stores information about database tables
class CPPNGSDSHARED_EXPORT DatabaseCache
{
public:
	static DatabaseCache& inst();
	NGSD& ngsd();
	QSharedPointer<DatabaseTableInfo> getInfos(QString table);

protected:
	NGSD db_;
	QMap<QString, QSharedPointer<DatabaseTableInfo> > infos_;

	DatabaseCache(); //private
	DatabaseCache(const DatabaseCache& rhs) = delete;
};

#endif // DATABASECACHE_H
