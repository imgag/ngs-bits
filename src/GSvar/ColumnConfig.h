#ifndef COLUMNCONFIG_H
#define COLUMNCONFIG_H

#include <QString>
#include <QHash>
#include <QTableWidget>
#include "VariantList.h"
#include "CnvList.h"

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
	void append(QString name, const ColumnInfo& info);
	//Returns column infos of given column (default constructed info if not contained)
	ColumnInfo info(const QString& name) const { return infos_.value(name); };

	//Returns the column order for the given small variant list
	void getOrder(const VariantList& vars, QStringList& col_order, QList<int>& anno_index_order);
	//Returns the column order for the given CNV variant list
	void getOrder(const CnvList& cnvs, QStringList& col_order, QList<int>& anno_index_order);
	//Apply column width settings to table
	void applyColumnWidths(QTableWidget* table, int max_width_for_not_contained=200);
	//Apply column hidden settings to table
	void applyHidden(QTableWidget* table);

	//Returns a tab-separated list of column configs. The fields of the inividual columns are separated by |. Used for storing the config in the settings INI.
	QString toString() const;
	//Helper function for parsing from text/file.
	static ColumnConfig fromString(const QString& text);

protected:
	QStringList columns_;
	QHash<QString, ColumnInfo> infos_;
};

#endif // COLUMNCONFIG_H
