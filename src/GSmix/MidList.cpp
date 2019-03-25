#include "MidList.h"
#include "ui_MidList.h"
#include "NGSD.h"

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
	NGSD db;
	SqlQuery query = db.getQuery();
	query.exec("SELECT name, sequence FROM mid ORDER by name, sequence ASC");
	while(query.next())
	{
		ui->mids->addItem(query.value(0).toString() + " (" + query.value(1).toString() + ")");
	}
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
