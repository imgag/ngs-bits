#include "DBTableWidget.h"
#include "GUIHelper.h"
#include "Exceptions.h"

#include <QHeaderView>
#include <QAction>

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
		for(int c=0; c<headers.count(); ++c)
		{
			setItem(r, c, createItem(table.row(r).value(c)));
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

QTableWidgetItem* DBTableWidget::createItem(const QString& text, int alignment)
{
	auto item = new QTableWidgetItem();
	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

	item->setText(text);

	item->setTextAlignment(alignment);

	return item;
}

void DBTableWidget::copyToClipboard()
{
	qDebug() << __LINE__;
}
