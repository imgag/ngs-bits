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
	panel->setFixedHeight(height() / 30);
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


void BedPanel::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		int x = event->pos().x();
		is_dragging_ = true;
		drag_start_x_ = x;
		drag_start_region_ = SharedData::region();
	}
}



void BedPanel::mouseMoveEvent(QMouseEvent* event)
{
	int x = event->pos().x();

	if (is_dragging_)
	{
		int w = width();
		int label_width = SharedData::settings().label_width;
		float total_width = width() - label_width - 4;

		if (x > label_width + 2 && x < w - 2)
		{
			if (abs(x - drag_start_x_) >= 5)
			{
				float end = x;
				float start = drag_start_x_;
				float diff = ((end - start)/total_width) * (.4 * drag_start_region_.length());

				SharedData::setRegion(drag_start_region_.chr(), drag_start_region_.start() - diff, drag_start_region_.end() - diff);
			}
		}

		update();
	}
}


void BedPanel::mouseReleaseEvent(QMouseEvent* event)
{
	int x = event->pos().x();
	int w = width();
	int label_width = SharedData::settings().label_width;
	float total_width = width() - label_width - 4;

	if (event->button() == Qt::LeftButton && is_dragging_)
	{
		if (x > label_width + 2 && x < w - 2)
		{
			if (abs(x - drag_start_x_) >= 5)
			{
				float end = x;
				float start = drag_start_x_;
				float diff = ((end - start)/total_width) * (.4 * drag_start_region_.length());

				SharedData::setRegion(drag_start_region_.chr(), drag_start_region_.start() - diff, drag_start_region_.end() - diff);
			}
		}
	}

	is_dragging_ = false;
	update();
}
