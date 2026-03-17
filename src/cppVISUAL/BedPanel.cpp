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
	 * TODO: this is adhoc and extremely inefficient,
	 * this needs to be optimized.
	 */
	// clearLayout();
	QVector<Track> tracks = SharedData::tracks();

	auto panel = new TrackPanel(this, tracks.last());
	panel->setFixedHeight(height() / 30);
	layout_->insertWidget(layout_->count() - 1, panel);
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

void BedPanel::paintEvent(QPaintEvent*)
{
	// QTextStream(stdout) << "bed panel paint called" << Qt::endl;

	// QPainter painter(this);
	// painter.fillRect(rect(), Qt::black);
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
	// QTextStream(stdout) << "Cx: " << x << Qt::endl;
	current_mouse_x_ = x;
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


void BedPanel::wheelEvent(QWheelEvent* event)
{
	/*
	 * TODO: This position based zooming in is too unstable, it needs to be fixed
	 * until then the same logic as the buttons is used
	 */
	// int x = current_mouse_x_;
	// int w = width();
	// int label_width = SharedData::settings().label_width;
	// float total_width = w - label_width - 4;

	// static const float zoom_in_factor = 0.9f;
	// static const float zoom_out_factor = 1.1f;

	// const BedLine& region = SharedData::region();
	// float region_length = region.length();
	// float region_start = region.start();

	// // Calculate how far the mouse is from the left of the region
	// float mouse_pos_percentage = (x - label_width - 2) / total_width;
	// float region_pos = region_start + mouse_pos_percentage * region_length;

	// QTextStream(stdout) << x << ' ' << label_width  << ' ' << mouse_pos_percentage << ' ' << region_start << ' ' << region_pos << Qt::endl;

	// (zoom in)
	if (event->angleDelta().y() > 0)
	{
		// Zoom in: reduce the region size, centered around the mouse position
		// float zoom_in_amount = region_length * zoom_in_factor * .5f;
		// // float new_start = region_start + mouse_pos_percentage * zoom_in_amount;
		// // float new_end = region_end - (1 - mouse_pos_percentage) * zoom_in_amount;
		// float new_start = region_pos - zoom_in_amount;
		// float new_end	= region_pos + zoom_in_amount;

		// SharedData::setRegion(region.chr(), new_start, new_end);
		const BedLine& reg = SharedData::region();
		SharedData::setRegion(reg.chr(), reg.start()+reg.length()/4, reg.end()-reg.length()/4);
	}
	// (zoom out)
	else
	{
		// Zoom out: increase the region size, centered around the mouse position
		// float zoom_out_amount = region_length * zoom_out_factor * .5f;
		// float new_start = region_pos - zoom_out_amount;
		// float new_end	= region_pos + zoom_out_amount;

		// SharedData::setRegion(region.chr(), new_start, new_end);
		const BedLine& reg = SharedData::region();
		SharedData::setRegion(reg.chr(), reg.start()-reg.length()/2, reg.end()+reg.length()/2);
	}

	event->accept();
}
