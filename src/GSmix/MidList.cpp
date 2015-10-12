#include "MidList.h"
#include "MidCache.h"
#include <QTimer>
#include "ui_MidList.h"

MidList::MidList(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::MidList)
{
	ui->setupUi(this);
	connect(ui->search, SIGNAL(textEdited(QString)), this, SLOT(filter(QString)));

	//init delayed initialization
	QTimer* timer = new QTimer(this);
	timer->setSingleShot(true);
	timer->start(50);
	connect(timer, SIGNAL(timeout()), this, SLOT(delayedInizialization()));
}

MidList::~MidList()
{
	delete ui;
}

void MidList::delayedInizialization()
{
	loadMidsFromCache();

	//delete timer object
	qobject_cast<QTimer*>(sender())->deleteLater();
}

void MidList::loadMidsFromCache()
{
	const MidCache& mid_cache = MidCache::inst();
	for (int i=0; i<mid_cache.count(); ++i)
	{
		ui->mids->addItem(mid_cache[i].toString());
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
