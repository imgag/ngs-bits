#include "MidList.h"
#include "GDBO.h"
#include <QTimer>
#include "ui_MidList.h"

MidList::MidList(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::MidList)
{
	ui->setupUi(this);
	connect(ui->search, SIGNAL(textEdited(QString)), this, SLOT(filter(QString)));

	loadMidsFromDB();
}

MidList::~MidList()
{
	delete ui;
}

void MidList::loadMidsFromDB()
{
	QList<GDBO> mids = GDBO::all("mid");
	foreach(const GDBO& mid, mids)
	{
		ui->mids->addItem(mid.get("name") + " (" + mid.get("sequence") + ")");
	}

	//delete timer object
	qobject_cast<QTimer*>(sender())->deleteLater();
}

void MidList::filter(QString text)
{
	if (text=="")
	{
		for(int i=0; i<ui->mids->count(); ++i)
		{
			ui->mids->item(i)->setHidden(false);
		}
		return;
	}

	for(int i=0; i<ui->mids->count(); ++i)
	{
		bool hidden = !ui->mids->item(i)->text().contains(text, Qt::CaseInsensitive);
		ui->mids->item(i)->setHidden(hidden);
	}
}
