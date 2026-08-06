#ifndef DATABASEINSERT_H
#define DATABASEINSERT_H

#include "NGSD.h"
#include "DatabaseSchema.h"

class DatabaseInsert
{
public:
	static void insert(const TableSchema& table, const QHash<QString, QVariant>& values, qlonglong* inserted_id = nullptr);
};

#endif // DATABASEINSERT_H
