#include "MidList.h"
#include "NGSD.h"
#include <QDebug>
#include <QMessageBox>

MidList::MidList(QWidget *parent)
	: QWidget(parent)
	, ui()
{

	ui.setupUi(this);
	connect(ui.search, SIGNAL(textEdited(QString)), this, SLOT(filter(QString)));

	loadMidsFromNGSD();
}

MidList::~MidList()
{
}

void MidList::loadMidsFromNGSD()
{
	try
	{
		QSqlQuery q = NGSD().getQuery();

		q.exec("SELECT name, number, sequence FROM mid");
		while(q.next())
		{
			ui.mids->addItem(q.value(0).toString() + " " + q.value(1).toString() + " (" + q.value(2).toString() + ")");
		}
	}
	catch(...)
	{

		QMessageBox::StandardButton button = QMessageBox::question(this, "Database error", "Could not connect to NGSD!\nLoad MIDs from file?");
		if (button==QMessageBox::Yes)
		{
			//TODO
		}
	}
}

void MidList::filter(QString text)
{
	if (text=="")
	{
		for(int i=0; i<ui.mids->count(); ++i)
		{
			ui.mids->item(i)->setHidden(false);
		}
		return;
	}

	for(int i=0; i<ui.mids->count(); ++i)
	{
		bool hidden = !ui.mids->item(i)->text().contains(text);
		ui.mids->item(i)->setHidden(hidden);
	}
}
