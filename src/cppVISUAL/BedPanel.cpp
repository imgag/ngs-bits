#include "BedPanel.h"
#include "SharedData.h"
#include "BedTrack.h"

#include <QPainter>


BedPanel::BedPanel(QWidget*)
	:layout_(new QVBoxLayout(this))
{
	layout_->addStretch(1);
	layout_->setContentsMargins(0, 0, 0, height());
	connect(SharedData::instance(), SIGNAL(trackAdded(Track)), this, SLOT(trackAdded(Track)));
}

void BedPanel::trackAdded(Track tr)
{
	auto panel = new BedTrack(this, tr);
	connect(panel, SIGNAL(trackDeleted()), this, SLOT(trackDeleted()));
	layout_->insertWidget(layout_->count() - 1, panel);
	layout_->update();
}

void BedPanel::trackDeleted()
{
	QWidget* senderPanel = qobject_cast<QWidget*>(sender());
	if (senderPanel) {
		layout_->removeWidget(senderPanel);
		senderPanel->deleteLater();
		layout_->update();
	}
}

void BedPanel::clearLayout()
{
	if (!layout_) return;
	while (QLayoutItem* item = layout_->takeAt(0))
	{
		if (QWidget* widget = item->widget()) widget->deleteLater();
		delete item;
	}
}



