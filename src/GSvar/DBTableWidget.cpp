#include "DBTableWidget.h"
#include "Exceptions.h"
#include "GUIHelper.h"

#include <QHeaderView>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>

DBTableWidget::DBTableWidget(QWidget* parent)
	: QTableWidget(parent)
{
	//general settings
	verticalHeader()->setVisible(false);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setWordWrap(false);
	connect(this, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(processDoubleClick(int, int)));

	//context menu
	setContextMenuPolicy(Qt::ActionsContextMenu);
	QAction* copy_action = new QAction(QIcon(":/Icons/CopyClipboard.png"), "Copy selection", this);
	addAction(copy_action);
	connect(copy_action, SIGNAL(triggered(bool)), this, SLOT(copySelectionToClipboard()));
	copy_action = new QAction(QIcon(":/Icons/CopyClipboard.png"), "Copy all", this);
	addAction(copy_action);
	connect(copy_action, SIGNAL(triggered(bool)), this, SLOT(copyTableToClipboard()));
}

void DBTableWidget::setData(const DBTable& table, int max_col_width, QSet<QString> cols_double, QSet<QString> cols_int)
{
	QStringList headers = table.headers();

	//table
	table_ = table.tableName();

	//resize
	clearContents();
	setRowCount(table.rowCount());
	setColumnCount(headers.count());

	//headers
	for(int c=0; c<headers.count(); ++c)
	{
		setHorizontalHeaderItem(c, GUIHelper::createTableItem(headers[c], Qt::AlignCenter));
	}

	//determine numberic columns to allow correct alignment, sorting, etc.
	QVector<int> col_type;
	for(int c=0; c<table.columnCount(); ++c)
	{
		QString col = table.headers()[c];
		if (cols_double.contains(col))
		{
			col_type << QVariant::Double;
		}
		else if (cols_int.contains(col))
		{
			col_type << QVariant::Int;
		}
		else col_type << QVariant::String;
	}

	//content
	ids_.clear();
	for(int r=0; r<table.rowCount(); ++r)
	{
		const DBRow& row = table.row(r);
		ids_ << row.id();
		for(int c=0; c<headers.count(); ++c)
		{
			const QString& value = row.value(c);
			if (value.isEmpty())
			{
				setItem(r, c, GUIHelper::createTableItem(""));
			}
			else if (col_type[c]==QVariant::Double)
			{
				setItem(r, c, GUIHelper::createTableItem(row.value(c).toDouble()));
			}
			else if (col_type[c]==QVariant::Int)
			{
				setItem(r, c, GUIHelper::createTableItem(row.value(c).toInt()));
			}
			else
			{
				setItem(r, c, GUIHelper::createTableItem(value));
			}
		}
	}

	//fomatting
	GUIHelper::resizeTableCellWidths(this, max_col_width);
	GUIHelper::resizeTableCellHeightsToFirst(this);
}

int DBTableWidget::columnIndex(const QString& column_header) const
{
	return GUIHelper::columnIndex(this, column_header);
}

QString DBTableWidget::columnHeader(int index) const
{
	if (index<0 || index>=columnCount())
	{
		THROW(ArgumentException, "Invalid column index " + QString::number(index) + ". The table has " + QString::number(columnCount()) + " columns!");
	}

	return horizontalHeaderItem(index)->text();
}

void DBTableWidget::setQualityIcons(const QString& column_header, const QStringList& quality_values)
{
	//check
	if (quality_values.count()!=rowCount())
	{
		THROW(ArgumentException, "Invalid quality value count '" + QString::number(quality_values.count()) + "' in DBTableWidget::setQualityIcons - expected '" + QString::number(rowCount()) + "'!");
	}

	int c = columnIndex(column_header);
	for(int r=0; r<rowCount(); ++r)
	{
		QTableWidgetItem* table_item = item(r, c);
		if (table_item==nullptr) continue;

		const QString& quality = quality_values[r];

		styleQuality(table_item, quality);
	}

	setColumnWidth(c, columnWidth(c) + 25);
}

void DBTableWidget::setColumnTooltips(const QString& column_header, const QStringList& tooltips)
{
	if (tooltips.count()!=rowCount())
	{
		THROW(ArgumentException, "Invalid tooltip count '" + QString::number(tooltips.count()) + "' in DBTableWidget::setColumnTooltips - expected '" + QString::number(rowCount()) + "'!");
	}

	int c = columnIndex(column_header);
	for(int r=0; r<rowCount(); ++r)
	{
		QTableWidgetItem* table_item = item(r, c);
		if (table_item==nullptr) continue;
		table_item->setToolTip(tooltips[r]);
	}
}

void DBTableWidget::setColumnColors(const QString& column_header, const QList<QColor>& colors)
{
	if (colors.count()!=rowCount())
	{
		THROW(ArgumentException, "Invalid color count '" + QString::number(colors.count()) + "' in DBTableWidget::setColumnColors - expected '" + QString::number(rowCount()) + "'!");
	}

	int c = columnIndex(column_header);
	for (int r=0; r<rowCount(); ++r)
	{
		const QColor& color = colors[r];
		if (!color.isValid()) continue;

		QTableWidgetItem* table_item = item(r, c);
		if (table_item==nullptr) continue;

		table_item->setBackgroundColor(color);
	}
}

void DBTableWidget::setBackgroundColorIfContains(const QString& column_header, const QColor& color, const QString& substring)
{
	setBackgroundColorIf(column_header, color, [substring](const QString& str) { return str.contains(substring); });
}

void DBTableWidget::setBackgroundColorIfEqual(const QString& column_header, const QColor& color, const QString& text)
{
	setBackgroundColorIf(column_header, color, [text](const QString& str) { return str==text; });
}

void DBTableWidget::setBackgroundColorIfLt(const QString& column_header, const QColor& color, double cutoff)
{
	setBackgroundColorIf(column_header, color, [cutoff](const QString& str) { bool ok; double value = str.toDouble(&ok); if (!ok) return false; return value<cutoff; });
}

void DBTableWidget::setBackgroundColorIfGt(const QString& column_header, const QColor& color, double cutoff)
{
	setBackgroundColorIf(column_header, color, [cutoff](const QString& str) { bool ok; double value = str.toDouble(&ok); if (!ok) return false; return value>cutoff; });
}

void DBTableWidget::showTextAsTooltip(const QString& column_header)
{
	int c = columnIndex(column_header);
	for (int r=0; r<rowCount(); ++r)
	{
		QTableWidgetItem* table_item = item(r, c);
		if (table_item==nullptr) continue;

		table_item->setToolTip(table_item->text());
	}
}

QSet<int> DBTableWidget::selectedRows() const
{
	return GUIHelper::selectedTableRows(this).toSet();
}

QSet<int> DBTableWidget::selectedColumns() const
{
	return GUIHelper::selectedTableColumns(this).toSet();
}

const QString& DBTableWidget::getId(int r) const
{
	if (r<0 || r>=rowCount())
	{
		THROW(ArgumentException, "Invalid row index '" + QString::number(r) + "' in DBTableWidget::getId!");
	}

	return ids_[r];
}

const QString& DBTableWidget::tableName() const
{
	return table_;
}

void DBTableWidget::styleQuality(QTableWidgetItem* item, const QString& quality)
{
	//init
	static QIcon i_good = QIcon(":/Icons/quality_good.png");
	static QIcon i_medium = QIcon(":/Icons/quality_medium.png");
	static QIcon i_bad = QIcon(":/Icons/quality_bad.png");
	static QIcon i_na = QIcon(":/Icons/quality_unset.png");

	//icon
	if (quality=="good") item->setIcon(i_good);
	else if (quality=="medium") item->setIcon(i_medium);
	else if (quality=="bad") item->setIcon(i_bad);
	else item->setIcon(i_na);

	//tooltip
	item->setToolTip(quality);
}

void DBTableWidget::keyPressEvent(QKeyEvent* event)
{
	if(event->matches(QKeySequence::Copy))
	{
		copySelectionToClipboard();
		event->accept();
		return;
	}

	QTableWidget::keyPressEvent(event);
}

void DBTableWidget::copySelectionToClipboard()
{
	GUIHelper::copyToClipboard(this, true);
}

void DBTableWidget::copyTableToClipboard()
{
	GUIHelper::copyToClipboard(this, false);
}

void DBTableWidget::processDoubleClick(int row, int /*column*/)
{
	emit rowDoubleClicked(row);
}
