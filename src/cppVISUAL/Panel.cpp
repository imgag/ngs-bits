#include "Panel.h"
#include "BedTrack.h"

#include <QPainter>


Panel::Panel(QWidget*)
	:layout_(new QVBoxLayout(this))
{
	layout_->addStretch(1);
	layout_->setContentsMargins(0, 0, 0, height());
}

void Panel::trackAdded(Track tr)
{
	auto panel = new BedTrack(this, tr);
	connect(panel, SIGNAL(trackDeleted()), this, SLOT(trackDeleted()));
	layout_->insertWidget(layout_->count() - 1, panel);
	layout_->update();
}

void Panel::trackDeleted()
{
	QWidget* senderPanel = qobject_cast<QWidget*>(sender());
	if (senderPanel) {
		layout_->removeWidget(senderPanel);
		senderPanel->deleteLater();
		layout_->update();
	}
}

void Panel::clearLayout()
{
	if (!layout_) return;
	while (QLayoutItem* item = layout_->takeAt(0))
	{
		if (QWidget* widget = item->widget()) widget->deleteLater();
		delete item;
	}
}



