#include "BlatWidget.h"
#include "Sequence.h"
#include "Exceptions.h"
#include "HttpHandler.h"
#include "GSvarHelper.h"
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
	connect(ui_.search_btn, SIGNAL(clicked(bool)), this, SLOT(performSearch()));
	connect(ui_.sequence, SIGNAL(returnPressed()), this, SLOT(performSearch()));
	connect(ui_.table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(resultContextMenu(QPoint)));
}

void BlatWidget::performSearch()
{
	//clear
	ui_.table->setRowCount(0);

    QStringList mandatory_fields = QStringList() << "matches" << "repMatches" << "strand" << "tName" << "tStart" << "tEnd";

	//perform search
	try
	{
		//check
		Sequence sequence = ui_.sequence->text().toUtf8();
		if (sequence.length()<20) THROW(ArgumentException, "Input sequence too short! Must be at least 20 bases!");

        RequestUrlParams params;
        params.insert("sequence", sequence);
        QByteArray reply = ApiCaller().get("blat_search", params, HttpHeaders(), false, false, true);
        if (reply.length()==0) THROW(Exception, "Could not get BLAT search results");

		//update GUI
        QJsonArray blat_matches =  QJsonDocument::fromJson(reply).array();
		for(int i=0; i<blat_matches.count(); ++i)
		{
			ui_.table->setRowCount(ui_.table->rowCount()+1);

            QJsonObject current_object = blat_matches[i].toObject();

            for (QString field: mandatory_fields)
            {
                if (!current_object.contains(field)) THROW(ProgrammingException, "BLAT JSON object does not have '" + field + "' field!")
            }

            ui_.table->setItem(i, 0, GUIHelper::createTableItem(QString::number(current_object.value("matches").toInt())+"/"+QString::number(sequence.size())));
            ui_.table->setItem(i, 1, GUIHelper::createTableItem(QString::number(current_object.value("repMatches").toInt())+"/"+QString::number(sequence.size())));
            ui_.table->setItem(i, 2, GUIHelper::createTableItem(current_object.value("strand").toString()));
            ui_.table->setItem(i, 3, GUIHelper::createTableItem(current_object.value("tName").toString()+":"+QString::number(current_object.value("tStart").toInt()+1)+"-"+QString::number(current_object.value("tEnd").toInt())));
		}
		GUIHelper::resizeTableCellWidths(ui_.table);
		GUIHelper::resizeTableCellHeightsToFirst(ui_.table);
	}
	catch (Exception& e)
	{
        QMessageBox::warning(this, windowTitle(), "Error during BLAT search: " + e.message());
	}
}

void BlatWidget::resultContextMenu(QPoint pos)
{
	//extract selected rows
	QList<int> rows = GUIHelper::selectedTableRows(ui_.table);
	if (rows.isEmpty()) return;

	//execute menu
	QMenu menu;
	QAction* a_igv = menu.addAction(QIcon(":/Icons/IGV.png"), "Open position in IGV");
	QAction* action = menu.exec(ui_.table->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	if  (action==a_igv)
	{
		QTableWidgetItem* item = ui_.table->itemAt(pos);
		if (item==nullptr) return;

		int r = item->row();
        IgvSessionManager::get(0).gotoInIGV(ui_.table->item(r, 3)->text(), false);
	}
}


