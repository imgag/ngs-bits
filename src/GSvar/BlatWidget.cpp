#include "BlatWidget.h"
#include "Sequence.h"
#include "Exceptions.h"
#include "GUIHelper.h"
#include "IgvSessionManager.h"
#include "ApiCaller.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMessageBox>
#include <QMenu>

BlatWidget::BlatWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	ui_.search_btn->setAutoDefault(false);
	ui_.search_btn->setDefault(false);
	connect(ui_.search_btn, SIGNAL(clicked(bool)), this, SLOT(performSearch()));
	connect(ui_.sequence, SIGNAL(returnPressed()), ui_.search_btn, SLOT(click()));
	connect(ui_.table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(resultContextMenu(QPoint)));
}

void BlatWidget::performSearch()
{
	//clear
	ui_.table->setRowCount(0);
	ui_.table->setColumnCount(0);
	ui_.search_btn->setDisabled(true);

	//perform search
	try
	{
		//remove previous search
		ui_.table->clear();

		//check
		Sequence sequence = ui_.sequence->text().toUtf8().trimmed();
		if (sequence.length()<20 || sequence.length()>500) THROW(ArgumentException, "Input sequence invalid! Must be between 20 and 500 bases!");

        RequestUrlParams params;
        params.insert("sequence", sequence);
		QByteArray reply = ApiCaller().get("blat_search", params, HttpHeaders(), false, false, true);
		if (reply.length()==0) THROW(Exception, "Could not get sequence search results: empty result!");

		//update GUI
		int cols = -1;
		foreach(QByteArray line, reply.split('\n'))
		{
			line = line.trimmed();
			if (line.isEmpty()) continue;

			QByteArrayList parts = line.split('\t');

			//header > set column headers
			if (line[0]=='#')
			{
				if (!line.startsWith("##"))
				{
					cols = parts.count();
					ui_.table->setColumnCount(cols);
					for (int i=0; i<parts.count(); ++i)
					{
						ui_.table->setHorizontalHeaderItem(i, GUIHelper::createTableItem(parts[i]));
					}
				}
				continue;
			}

			//content
			if (line.startsWith("*")) break; // to handle empty results
			if (parts.count()!=cols) THROW(FileParseException, "BLAT search output contains line with "+QString::number(parts.count())+" columns. "+QString::number(cols) + "expected!");
			int row = ui_.table->rowCount();
			ui_.table->setRowCount(row+1);
			for (int i=0; i<parts.count(); ++i)
			{				
				ui_.table->setItem(row, i, GUIHelper::createTableItem(parts[i]));
			}
		}
		GUIHelper::resizeTableCellWidths(ui_.table);
		GUIHelper::resizeTableCellHeightsToFirst(ui_.table);
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, windowTitle(), "Error during BLAT-like search: " + e.message());
	}
	ui_.search_btn->setDisabled(false);
}

void BlatWidget::resultContextMenu(QPoint pos)
{
	//extract selected rows
	QList<int> rows = GUIHelper::selectedTableRows(ui_.table);

	//create
	QMenu menu;
	QAction* a_igv = menu.addAction(QIcon(":/Icons/IGV.png"), "Open position in IGV");
	a_igv->setEnabled(rows.count()==1);

	//execute menu
	QAction* action = menu.exec(ui_.table->viewport()->mapToGlobal(pos));
	if  (action==a_igv)
	{
		IgvSessionManager::get(0).gotoInIGV(ui_.table->item(rows[0], 0)->text(), false);
	}
}


