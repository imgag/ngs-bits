#ifndef COLUMNCONFIG_H
#define COLUMNCONFIG_H

#include <QString>
#include <QString>

struct ColumnÍnfo
{
	int min_width;
	int max_width;
	bool hidden;
};

class ColumnConfig
{
public:
	ColumnConfig();

	QList<QString, ColumnÍnfo> cols_;
};

#endif // COLUMNCONFIG_H
