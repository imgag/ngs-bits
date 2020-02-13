#ifndef DBTABLEWIDGET_H
#define DBTABLEWIDGET_H

#include <QTableWidget>

#include "DBTable.h"
#include "Exceptions.h"
#include <QDebug>

//Visualization of a database table.
class DBTableWidget
	: public QTableWidget
{
	Q_OBJECT

public:
	DBTableWidget(QWidget* parent = 0);

	//Set table data
	void setData(const DBTable& table);
	//returns the column index, or throws ArgumentException if column is not present.
	int columnIndex(const QString& column_header) const;
	//Set tooltips for a column
	void setColumnTooltips(const QString& column_header, const QStringList& tooltips);
	//Set background color for a column
	void setColumnColors(const QString& column_header, const QList<QColor>& colors);
	//Set background color for a column if text matches
	void setBackgroundColorIfEqual(const QString& column_header, const QColor& color, const QString& text);
	//Set background color for a column if the cell contains a number and if lower than the cutoff.
	void setBackgroundColorIfLt(const QString& column_header, const QColor& color, double cutoff);
	//Set background color for a column if the cell contains a number and if greater than the cutoff.
	void setBackgroundColorIfGt(const QString& column_header, const QColor& color, double cutoff);
	//Set background color for a column by predicate
	template<typename T>
	void setBackgroundColorIf(const QString& column_header, const QColor& color, T predicate)
	{
		int c = columnIndex(column_header);
		for (int r=0; r<rowCount(); ++r)
		{
			QTableWidgetItem* table_item = item(r, c);
			if (table_item==nullptr) continue;

			if (predicate(table_item->text()))
			{
				table_item->setBackgroundColor(color);
			}
		}
	}

	//Returns selected row indices
	QSet<int> selectedRows() const;
	//Returns the database ID of row r
	const QString& getId(int r) const;
	//Returns the database table name
	const QString& tableName() const;

protected:
	void keyPressEvent(QKeyEvent* event) override;

protected slots:
	void copyToClipboard();

protected:
	QString table_;
	QStringList ids_;

};

#endif // DBTABLEWIDGET_H
