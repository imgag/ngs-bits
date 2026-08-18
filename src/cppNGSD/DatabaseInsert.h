#ifndef DATABASEINSERT_H
#define DATABASEINSERT_H

#include "cppNGSD_global.h"
#include "NGSD.h"
#include "DatabaseSchema.h"

class CPPNGSDSHARED_EXPORT DatabaseInsert
{
public:
	static void insert(NGSD& db, const TableSchema& table, const QHash<QString, QVariant>& values);
};

#endif // DATABASEINSERT_H
