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
	void setData(const DBTable& table, int max_col_width=200, QSet<QString> cols_double = QSet<QString>(), QSet<QString> cols_int = QSet<QString>());
	//returns the column index, or throws ArgumentException if column is not present.
	int columnIndex(const QString& column_header) const;
	//returns the column name, or throws ArgumentException if column is not present.
	QString columnHeader(int index) const;
	//Converts quality values to icons
	void setQualityIcons(const QString& column_header, const QStringList& quality_values);
	//Set tooltips for a column
	void setColumnTooltips(const QString& column_header, const QStringList& tooltips);
	//Set background color for a column
	void setColumnColors(const QString& column_header, const QList<QColor>& colors);
	//Set background color for a column if text matches
	void setBackgroundColorIfContains(const QString& column_header, const QColor& color, const QString& substring);
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
                table_item->setBackground(QBrush(QColor(color)));
			}
		}
	}
	//Uses the cell text additionally as tooltip
	void showTextAsTooltip(const QString& column_header);

	//Returns selected row indices.
	QSet<int> selectedRows() const;
	//Returns selected column indices (use only, if you changed selection behaviour).
	QSet<int> selectedColumns() const;
	//Returns the database ID of row r
	const QString& getId(int r) const;
	//Returns the database table name
	const QString& tableName() const;

	//Style cell by quality
	static void styleQuality(QTableWidgetItem* item, const QString& quality);

signals:
	//Emitted if a row is double-clicked
	void rowDoubleClicked(int row);

protected:
	void keyPressEvent(QKeyEvent* event) override;

protected slots:
	void copySelectionToClipboard();
	void copyTableToClipboard();
	void processDoubleClick(int row, int column);

protected:
	QString table_;
	QStringList ids_;

};

#endif // DBTABLEWIDGET_H
