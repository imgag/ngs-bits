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

	//context menu
	setContextMenuPolicy(Qt::ActionsContextMenu);
	QAction* copy_action = new QAction("Copy", this);
	addAction(copy_action);
	connect(copy_action, SIGNAL(triggered(bool)), this, SLOT(copyToClipboard()));
}

void DBTableWidget::setData(const DBTable& table)
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
		setHorizontalHeaderItem(c, createItem(headers[c], Qt::AlignCenter));
	}

	//content
	for(int r=0; r<table.rowCount(); ++r)
	{
		const DBRow& row = table.row(r);
		ids_ <<row.id();
		for(int c=0; c<headers.count(); ++c)
		{
			setItem(r, c, createItem(row.value(c)));
		}
	}

	//fomatting
	GUIHelper::resizeTableCells(this, 200);
}

void DBTableWidget::setColumnTooltips(int c, const QStringList& tooltips)
{
	if (c<0 || c>=columnCount())
	{
		THROW(ArgumentException, "Invalid column index '" + QString::number(c) + "' in DBTableWidget::setColumnTooltips!");
	}
	if (tooltips.count()!=rowCount())
	{
		THROW(ArgumentException, "Invalid tooltip count '" + QString::number(tooltips.count()) + "' in DBTableWidget::setColumnTooltips - expected '" + QString::number(rowCount()) + "'!");
	}

	for(int r=0; r<rowCount(); ++r)
	{
		QTableWidgetItem* table_item = item(r, c);
		if (table_item==nullptr) continue;
		table_item->setToolTip(tooltips[r]);
	}
}

QSet<int> DBTableWidget::selectedRows() const
{
	QSet<int> output;

	foreach(const QTableWidgetSelectionRange& range, selectedRanges())
	{
		for (int row=range.topRow(); row<=range.bottomRow(); ++row)
		{
			output << row;
		}
	}

	return output;
}

const QString& DBTableWidget::getId(int r) const
{
	if (r<0 || r>=rowCount())
	{
		THROW(ArgumentException, "Invalid row index '" + QString::number(r) + "' in DBTableWidget::getId!");
	}

	return ids_[r];
}

const QString&DBTableWidget::tableName()
{
	return table_;
}

QTableWidgetItem* DBTableWidget::createItem(const QString& text, int alignment)
{
	auto item = new QTableWidgetItem();
	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

	item->setText(text);

	item->setTextAlignment(alignment);

	return item;
}

void DBTableWidget::keyPressEvent(QKeyEvent* event)
{
	if(event->matches(QKeySequence::Copy))
	{
		copyToClipboard();
		event->accept();
		return;
	}

	QTableWidget::keyPressEvent(event);
}

void DBTableWidget::copyToClipboard()
{
	//header
	QString output = "#";
	for (int col=0; col<columnCount(); ++col)
	{
		if (col!=0) output += "\t";
		output += horizontalHeaderItem(col)->text();
	}
	output += "\n";

	//rows
	QSet<int> selected_rows = selectedRows();
	for (int row=0; row<rowCount(); ++row)
	{
		//skip hidden
		if (isRowHidden(row)) continue;

		//skip unselected
		if (selected_rows.count()>0 && !selected_rows.contains(row)) continue;

		for (int col=0; col<columnCount(); ++col)
		{
			if (col!=0) output += "\t";
			output += item(row, col)->text();
		}
		output += "\n";
	}

	QApplication::clipboard()->setText(output);
}
