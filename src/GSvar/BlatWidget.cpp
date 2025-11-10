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

	ui_.genome->setCurrentText(buildToString(GSvarHelper::build()));
}

void BlatWidget::performSearch()
{
	//clear
	ui_.table->setRowCount(0);

	//perform search
	try
	{
		//check
		Sequence sequence = ui_.sequence->text().toUtf8();
		if (sequence.length()<20) THROW(ArgumentException, "Input sequence too short! Must be at least 20 bases!");

        RequestUrlParams params;
        params.insert("sequence", sequence);
        QByteArray reply = ApiCaller().get("blat_search", params, HttpHeaders(), true, false, true);
        if (reply.length() == 0)
        {
            THROW(Exception, "Could not get BLAT search results");
        }


        // QByteArray response_text = http_handler.get("https://genome.ucsc.edu/cgi-bin/hgBlat?userSeq="+sequence+"&type=DNA&db="+ui_.genome->currentText()+"&output=json");
        // QJsonObject response = QJsonDocument::fromJson(response_text).object();

		//check reply format (https://genome.ucsc.edu/FAQ/FAQformat.html)
        // QJsonArray fields = response.value("fields").toArray();
        // if (fields.count()<21) THROW(ProgrammingException, "BLAT JSON reply field count below 21!");
        // if (fields[0]!="matches") THROW(ProgrammingException, "BLAT JSON reply field 0 is not 'matches'!");
        // if (fields[2]!="repMatches") THROW(ProgrammingException, "BLAT JSON reply field 2 is not 'matches'!");
        // if (fields[8]!="strand") THROW(ProgrammingException, "BLAT JSON reply field 8 is not 'strand'!");
        // if (fields[13]!="tName") THROW(ProgrammingException, "BLAT JSON reply field 13 is not 'tName'!");
        // if (fields[15]!="tStart") THROW(ProgrammingException, "BLAT JSON reply field 15 is not 'tStart'!");
        // if (fields[16]!="tEnd") THROW(ProgrammingException, "BLAT JSON reply field 16 is not 'tEnd'!");

		//update GUI
        QJsonArray blat_matches =  QJsonDocument::fromJson(reply).array();
		for(int i=0; i<blat_matches.count(); ++i)
		{
			ui_.table->setRowCount(ui_.table->rowCount()+1);

            QJsonObject current_object = blat_matches[i].toObject();
            qDebug() << current_object;
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


