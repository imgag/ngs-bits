#include "ChromosomeContextPanel.h"
#include "SharedData.h"

#include <QMouseEvent>
#include <QLocale>


constexpr unsigned int ARROW_AREA_HEIGHT = 10;

ChromosomeContextPanel::ChromosomeContextPanel(QWidget* parent)
	: QWidget(parent)
{
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(updateRegion()));
}

void ChromosomeContextPanel::updateRegion()
{
	update();
}

void ChromosomeContextPanel::paintEvent(QPaintEvent*)
{
	int label_width = SharedData::settings().label_width;
	int total_width = width() - label_width - 4;

	QPainter painter(this);

	drawArrowsAndSize(&painter, label_width, total_width);
	drawTicks(&painter, label_width, total_width);
	drawDragRegion(&painter);
}

void ChromosomeContextPanel::drawArrowsAndSize(QPainter* painter, int label_width, int total_width)
{
	const BedLine& region = SharedData::region();
	int x_start = label_width + 2;
	int x_end	= x_start + total_width;
	int y = ARROW_AREA_HEIGHT / 2;

	QString size_label = formatRegionSize(region.length());
	QFont font = painter->font();
	font.setPointSize(8);
	painter->setFont(font);
	QFontMetrics fm(font);

	int text_w = fm.horizontalAdvance(size_label);
	int text_x = x_start + (total_width - text_w)/2;
	int text_y = y + fm.ascent() / 2 - 1;

	drawLineWithArrow(painter, QPoint(text_x + text_w + 3, y), QPoint(x_end, y));
	drawLineWithArrow(painter, QPoint(text_x - 3, y), QPoint(x_start, y));

	painter->setPen(Qt::black);
	painter->drawText(text_x, text_y, size_label);
}

void ChromosomeContextPanel::drawLineWithArrow(QPainter* painter, QPoint start, QPoint end)
{
	painter->setRenderHint(QPainter::Antialiasing, true);

	qreal arrowSize = ARROW_AREA_HEIGHT;
	painter->setPen(Qt::black);
	painter->setBrush(Qt::black);

	QLineF line(end, start);

	double angle = std::atan2(-line.dy(), line.dx());
	QPointF arrowP1 = line.p1() + QPointF(sin(angle + M_PI / 3) * arrowSize,
										  cos(angle + M_PI / 3) * arrowSize);
	QPointF arrowP2 = line.p1() + QPointF(sin(angle + M_PI - M_PI / 3) * arrowSize,
										  cos(angle + M_PI - M_PI / 3) * arrowSize);

	QPolygonF arrowHead;
	arrowHead.clear();
	arrowHead << line.p1() << arrowP1 << arrowP2;
	painter->drawLine(line);
	painter->drawPolygon(arrowHead);
}

void ChromosomeContextPanel::drawTicks(QPainter* painter, int label_width, int total_width)
{
	const BedLine& region = SharedData::region();
	int range = region.length();
	int interval = tickInterval(range, 22);
	if (interval <= 0) return;

	int x_start = label_width + 2;
	int y_bottom = height() - 5;
	int y_tick  = y_bottom - 6;

	//draw line at the bottom
	painter->drawLine(x_start, y_bottom, x_start + total_width, y_bottom);

	QFont font = painter->font();
	font.setPointSize(8);
	painter->setFont(font);
	QFontMetrics fm(font);

	int first = (region.start()/interval + 1)*interval;
	bool drawText = true;
	for (int coord = first; coord < region.end(); coord += interval)
	{
		float frac = float(coord - region.start())/range;
		int x = x_start + total_width * frac;
		painter->drawLine(x, y_bottom, x, y_tick);

		//draw text
		QString tick_label = formatCoordinateLabel(coord, region.length());
		int text_w = fm.horizontalAdvance(tick_label);
		int text_x = x - text_w/2;

		if (text_x < x_start || text_x + text_w > x_start + total_width) drawText = false;

		if (drawText){
			painter->setPen(Qt::black);
			painter->drawText(text_x, y_tick - fm.ascent() + 6, tick_label);
		}

		drawText = !drawText;
	}
}

QString ChromosomeContextPanel::formatRegionSize(int size)
{
	QLocale locale;
	if (size >= 10'000'000) return locale.toString(size / 1'000'000) + " Mb";
	if (size >= 1000) return locale.toString(size / 1000) + " kb";
	return locale.toString(size) + " bp";
}

QString ChromosomeContextPanel::formatCoordinateLabel(int coord, int size)
{
	QLocale locale;
	if (size >= 10'000'000) return locale.toString(coord / 1'000'000) + " Mb";
	if (size >= 1000) return locale.toString(coord / 1000) + " kb";
	return locale.toString(coord) + " bp";
}

int ChromosomeContextPanel::tickInterval(int range, int max_ticks)
{
	if (range <= 0) return 1;
	double raw = (double)range/max_ticks;
	double mag = std::pow(10.0, std::floor(std::log10(raw)));
	double frac = raw / mag;
	double itvl;
	if (frac < 1.5) itvl = 1;
	else if (frac < 3.5) itvl = 2;
	else if (frac < 7.5) itvl = 5;
	else itvl = 10;

	return static_cast<int>(itvl * mag);
}

void ChromosomeContextPanel::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		is_dragging_ = true;
		drag_start_x_ = event->pos().x();
	}
}

void ChromosomeContextPanel::mouseMoveEvent(QMouseEvent* event)
{
	if (is_dragging_)
	{
		drag_current_x_ = event->pos().x();
		update();
	}
}


void ChromosomeContextPanel::mouseReleaseEvent(QMouseEvent* event)
{
	int x = event->pos().x();
	int y = event->pos().y();
	int w = width();
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();
	int total_width = width() - label_width - 4;
	if (event->button() == Qt::LeftButton && is_dragging_)
	{
		if (x > label_width + 2 && x < w - 2)
		{
			int y_end = height();
			if (y >= 2 && y < y_end && abs(x - drag_start_x_) < 5)
			{
				int coordinate = ((double)(x - label_width - 2) / total_width) * region.length() + region.start();
				SharedData::setRegion(region.chr(), coordinate - region.length()/2, coordinate + region.length()/2);
			}

			else if (abs(x - drag_start_x_) >= 5)
			{
				int start = ((double)(drag_start_x_ -label_width - 2) / total_width) * region.length() + region.start();
				int end = ((double)(x -label_width - 2) / total_width) * region.length() + region.start();
				SharedData::setRegion(region.chr(), std::min(start, end), std::max(start, end));
			}
		}
	}

	is_dragging_ = false;
	update();
}

void ChromosomeContextPanel::drawDragRegion(QPainter* painter)
{
	if (is_dragging_)
	{
		float xt = std::min(drag_current_x_, drag_start_x_);
		float y_start = 2.;
		float width = std::abs(drag_current_x_ - drag_start_x_);
		QRectF drag_rect(xt, y_start, width, height() - y_start);
		QPen outlinePen(QColor(0, 120, 215));
		outlinePen.setWidth(1);
		outlinePen.setStyle(Qt::SolidLine);
		painter->setPen(outlinePen);
		painter->setBrush(QColor(135, 206, 235, 60));
		painter->drawRect(drag_rect);
	}
}

