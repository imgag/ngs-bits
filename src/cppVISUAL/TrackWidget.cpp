#include "TrackWidget.h"
#include "SharedData.h"
#include "TrackManager.h"

#include <QApplication>
#include <QDrag>
#include <QMenu>
#include <QMimeData>
#include <QPainter>

void TrackWidget::regionChanged()
{
	updateGeometry();
	update();
}

void TrackWidget::populateContextMenu(QMenu& menu)
{
	opt1_ = menu.addAction("Collapsed");
	opt2_ = menu.addAction("Expanded");

	for (QAction *action : {opt1_, opt2_})
	{
		action->setCheckable(true);
	}

	switch (draw_mode_)
	{
	case COLLAPSED:
		opt1_->setChecked(true);
		break;
	case EXPANDED:
		opt2_->setChecked(true);
		break;
	}
}

void TrackWidget::handleContextMenuAction(QAction* action)
{
	if (action == opt1_)
	{
		draw_mode_ = COLLAPSED;
		updateGeometry();
		update();
	}
	else if (action == opt2_)
	{
		draw_mode_ = EXPANDED;
		updateGeometry();
		update();
	}
}


void TrackWidget::contextMenu(QPoint pos)
{
	// create menu
	QMenu menu(this);

	populateContextMenu(menu);

	menu.addSeparator();
	QAction* del_opt = menu.addAction("Remove Track");


	QAction* action = menu.exec(mapToGlobal(pos));

	if (action == del_opt) emit trackDeleted();
	else handleContextMenuAction(action);
}


void TrackWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && event->pos().x() < SharedData::settings().label_width)
	{
		is_dragging_ = true;
		drag_start_pos_ = event->pos();
	}
	else
	{
		is_dragging_ = false;
		event->ignore();
	}
}


void TrackWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (!is_dragging_) {
		event->ignore();
		return;
	};
	if (!(event->buttons() & Qt::LeftButton)) return;
	if ((event->pos() - drag_start_pos_).manhattanLength() < QApplication::startDragDistance()) return;

	QDrag* drag = new QDrag(this);
	QMimeData* mime_data = new QMimeData;
	mime_data->setData("application/track-data", track_->id.toByteArray());
	drag->setMimeData(mime_data);


	int width = SharedData::settings().label_width;
	QPixmap pixmap(width, height());
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	QRect rect(0, 0, width, height());
	painter.setPen(QPen(QColor(0, 0, 0), 2));
	painter.setBrush(Qt::NoBrush);

	painter.drawRect(rect);

	drag->setPixmap(pixmap);
	drag->setHotSpot(event->pos());

	drag->exec(Qt::MoveAction);
}
