#ifndef COLUMNCONFIG_H
#define COLUMNCONFIG_H

#include <QString>
#include <QHash>

struct ColumnInfo
{
	int min_width = 0;
	int max_width = 200;
	bool hidden = false;
};

class ColumnConfig
{
public:
	ColumnConfig();

	//Returns the ordered list of columns
	const QStringList& columns() const { return columns_; };
	//Returns if the given column is contained
	bool contains(const QString& name) const { return infos_.contains(name); };
	//Appends a column
	void append(const QString& name, const ColumnInfo& info);

	ColumnInfo info(const QString& name) const { return infos_.value(name); };

	//returns a tab-separated list of column configs. The fields of the inividual columns are separated by |. Used for storing the config in the settings INI.
	QString toString() const;

	static ColumnConfig fromString(const QString& text);

	QStringList columns_;
	QHash<QString, ColumnInfo> infos_;
};

#endif // COLUMNCONFIG_H
