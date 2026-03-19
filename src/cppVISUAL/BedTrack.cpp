#include "BedTrack.h"
#include "SharedData.h"
#include "TrackManager.h"

#include <QApplication>
#include <QActionGroup>
#include <QDrag>
#include <QPainter>
#include <QMenu>
#include <QMimeData>


class Panel ;

BedTrack::BedTrack(QWidget* parent, QSharedPointer<Track> track)
	:QWidget(parent), track(track)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)) );
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(regionChanged()));

	num_rows_ = calculateNumRows();
	TrackManager::addTrackWidget(track->id, this);
	qDebug() << "Num rows in " << track->name << ": " << num_rows_ << Qt::endl;
}

void BedTrack::contextMenu(QPoint pos)
{
	// create menu
	QMenu menu(this);

	QAction* opt1 = menu.addAction("Collapsed");
	QAction* opt2 = menu.addAction("Expanded");
	menu.addSeparator();
	QAction* del_opt = menu.addAction("Remove Track");

	for (QAction *action : {opt1, opt2})
	{
		action->setCheckable(true);
	}

	switch (draw_mode_)
	{
		case EXPANDED:
			opt2->setChecked(true);
			break;
		case COLLAPSED:
			opt1->setChecked(true);
			break;
	}

	QAction* action = menu.exec(mapToGlobal(pos));

	if (action == opt1)
	{
		draw_mode_ = COLLAPSED;
		updateGeometry();
		update();
	}
	else if (action == opt2)
	{
		draw_mode_ = EXPANDED;
		updateGeometry();
		update();
	}
	else if (action == del_opt)
	{
		emit trackDeleted();
	}
}


void BedTrack::regionChanged()
{
	updateGeometry();
	update();
}

void BedTrack::paintEvent(QPaintEvent* /*event*/)
{
	const BedLine& region = SharedData::region();
	QPainter painter(this);

	painter.fillRect(rect(), QColor(250, 250, 250));

	int label_width = SharedData::settings().label_width;

	// draw text
	QRectF text_rect(0, 0, label_width-2, height());

	painter.setPen(Qt::black);
	painter.drawText(text_rect, Qt::AlignLeft, track->name);

	// draw bounding box
	QRectF bounding_rect(label_width + 2, 0, width() - label_width - 2, height() - 2);
	QPen outlinePen(QColor(192, 192, 192));
	outlinePen.setWidth(1);
	outlinePen.setStyle(Qt::SolidLine);
	painter.setPen(outlinePen);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(bounding_rect);

	if (track->bedfile.chromosomes().count() != 1) return;

	if (region.chr() == track->bedfile.chromosomes().values()[0])
	{

		int w = width();
		float total_width = w - label_width - 2;
		int prev_row_end = -1;
		float y_start = 0.0f;
		for (int idx =0; idx < track->bedfile.count(); ++idx)
			{
				int st = std::max(track->bedfile[idx].start(), region.start());
				int en = std::min(track->bedfile[idx].end(), region.end());

				float x_start = map(st, region.start(), region.end(), 0.0f, total_width);
				float width = map(static_cast<float>(en - st), 0.0f, static_cast<float>(region.length()), 0.0f, total_width);

				if (draw_mode_ == EXPANDED)
				{
					if (track->bedfile[idx].start() < prev_row_end)
					{
						y_start += (BLOCK_HEIGHT + BLOCK_PADDING);
					}
					prev_row_end = track->bedfile[idx].end();
				}

				QRectF chr_rect(label_width + 2 + x_start, y_start, width, BLOCK_HEIGHT);
				painter.setBrush(track->color);
				painter.drawRect(chr_rect);
			}
	}
}


QSize BedTrack::sizeHint() const {
	int rowHeight = BLOCK_HEIGHT + BLOCK_PADDING;
	int rowCount = 1;

	if (draw_mode_ == EXPANDED && SharedData::region().chr() == track->bedfile.chromosomes().values()[0])
	{
		rowCount = num_rows_;
	}

	return QSize(parentWidget() ? parentWidget()->width() : 200, rowCount * rowHeight + SPACING_BELOW);
}


int BedTrack::calculateNumRows()
{
	int num_rows =0;
	int last_row_end =-1;


	for (int i =0; i < track->bedfile.count(); ++i)
	{
		if (track->bedfile[i].start() < last_row_end || last_row_end < 0) num_rows++;
		last_row_end = track->bedfile[i].end();
	}

	return num_rows;
}

void BedTrack::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && event->pos().x() < SharedData::settings().label_width)
	{
		is_dragging_ = true;
		drag_start_pos_ = event->pos();
		// source_panel_ = dynamic_cast<Panel*>(parentWidget());
	}
	else
	{
		is_dragging_ = false;
		event->ignore();
	}
}


void BedTrack::mouseMoveEvent(QMouseEvent* event)
{
	if (!is_dragging_) {
		event->ignore();
		return;
	};
	if (!(event->buttons() & Qt::LeftButton)) return;
	if ((event->pos() - drag_start_pos_).manhattanLength() < QApplication::startDragDistance()) return;

	QDrag* drag = new QDrag(this);
	QMimeData* mime_data = new QMimeData;
	qDebug() << "Sending track id: " << track->id << Qt::endl;
	mime_data->setData("application/track-data", track->id.toByteArray());
	// QByteArray source_id = QByteArray::number(reinterpret_cast<quintptr>(this));
	// mime_data->setData("application/track-data", mime_data);
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
