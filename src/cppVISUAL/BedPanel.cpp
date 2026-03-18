#include "BedPanel.h"
#include "SharedData.h"
#include "TrackPanel.h"

#include <QPainter>


BedPanel::BedPanel(QWidget*)
	:layout_(new QVBoxLayout(this))
{
	layout_->addStretch(1);
	layout_->setContentsMargins(0, 0, 0, height());
	connect(SharedData::instance(), SIGNAL(tracksChanged()), this, SLOT(tracksChanged()));
}


void BedPanel::tracksChanged()
{
	/*
	 * TODO: this is adhoc and wrong,
	 * needs to be fixed.
	 */
	QVector<Track> tracks = SharedData::tracks();

	auto panel = new TrackPanel(this, tracks.last());
	// panel->setMinimumHeight(30);
	layout_->insertWidget(layout_->count() - 1, panel);
	layout_->update();
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



