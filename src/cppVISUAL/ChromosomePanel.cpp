#include "ChromosomePanel.h"
#include "SharedData.h"
#include "NGSHelper.h"
#include "ChromosomalIndex.h"
#include <QPainter>
#include <QMouseEvent>

ChromosomePanel::ChromosomePanel(QWidget* parent)
	: QWidget(parent)
{
	setMouseTracking(true);
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(updateRegion()));
}

void ChromosomePanel::updateRegion()
{
	update();
}


QColor cytobandColor(const QString& stain)
{
	if (stain == "gneg") return QColor(255, 255, 255);
	if (stain == "gpos25") return QColor(191, 191, 191);
	if (stain == "gpos50") return QColor(127, 127, 127);
	if (stain == "gpos75") return QColor(63, 63, 63);
	if (stain == "gpos100") return QColor(0, 0 ,0);
	if (stain == "acen") return QColor(178, 0, 0);
	// if (stain == "stalk") return QColor(100, 127, 164);
	// if (stain == "gvar") return QColor(180, 180, 255);

	return QColor(255, 255, 255);
}

QMap<Chromosome, QVector<int>> buildMap(const BedFile& bands){
	QMap<Chromosome, QVector<int>> map;
	for (int i =0; i < bands.count(); ++i)
	{
		map[bands[i].chr()].append(i);
	}
	return map;
}

void ChromosomePanel::paintEvent(QPaintEvent* /*event*/)
{
	//init
	int h = height();
	int w = width();
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();
	text_height_ = .3 * h;
	chr_height_ = .2 * h;

	QPainter painter(this);
	//bands:
	//NGSHelper::cytoBands
	//TODO add band colors

	static BedFile bands = NGSHelper::cytoBands(GenomeBuild::HG38);
	static bool sorted = false;
	if (!sorted)
	{
		bands.sort();
		sorted = true;
	}

	static QMap<Chromosome, QVector<int>> map = buildMap(bands);

	painter.fillRect(rect(), Qt::white);

	// QVector<int> idxes = regions_idx.matchingIndices(region.chr(), region.start(), region.end());

	QVector<int> idxes = map[region.chr()];
	QTextStream(stdout) << region.chr().str() << ' ' << idxes.size() << ' ' << region.length() << Qt::endl;
	if (idxes.size() >= 1)
	{
		int total_width = (w - label_width - 4);
		float width_cum_sum = 0;
		static char prev_arm = ' ';
		char current_arm;
		int total_length = bands[idxes[idxes.length() - 1]].end() - bands[idxes[0]].start() + 1;
		pixels_per_base_ = (double)(w-label_width-4) / (double)total_length;
		foreach (int i, idxes)
		{
			//drawing the band
			current_arm = bands[i].annotations()[0].at(0);

			float frac = (float)bands[i].length() / (total_length);
			float current_band_width = frac * total_width;
			QRectF rect(label_width + 2 + width_cum_sum, 2, current_band_width, chr_height_);

			// if color given in annotations - use that
			if (bands[i].annotations().size() > 1)
			{
				painter.setBrush(cytobandColor(bands[i].annotations()[1]));
			}
			else
			{
				painter.setBrush((i % 2 == 0) ? Qt::black : Qt::darkGray);
			}
			// arm switch - draw triangle
			if (current_arm == 'p' && i < bands.count()-1 && bands[i+1].annotations()[0].at(0) == 'q')
			{
				float p_start_x = label_width + 2 + width_cum_sum;
				QPolygonF p_tri;
				p_tri << QPointF(p_start_x, 2)
					  << QPointF(p_start_x + current_band_width, (2+chr_height_)/2.0)
					  << QPointF(p_start_x, 2 + chr_height_);
				painter.drawPolygon(p_tri);
			}
			else if (prev_arm == 'p' && current_arm == 'q')
			{

				float q_start_x = label_width + 2 + width_cum_sum;
				QPolygonF q_tri;
				q_tri << QPointF(q_start_x + current_band_width, 2)
					  << QPointF(q_start_x, (2 + chr_height_)/2.0)
					  << QPointF(q_start_x + current_band_width, 2 + chr_height_);
				painter.drawPolygon(q_tri);
			}
			else
			{
				painter.drawRect(rect);
			}

			prev_arm = current_arm;

			//drawing the text
			QFont font = painter.font();
			font.setPointSize(8);
			painter.setFont(font);

			QRectF text_rect(label_width + 2 + width_cum_sum, 2 + chr_height_ + padding_, current_band_width, text_height_);
			QString label = bands[i].annotations()[0];
			int text_pixel_width = painter.fontMetrics().horizontalAdvance(label);
			if (current_band_width > text_pixel_width)
			{
				painter.setPen(Qt::black);
				painter.drawText(text_rect, Qt::AlignCenter, label);
			}

			width_cum_sum += current_band_width;
		}

		//drawing rectangle for the region
		float x_start = ((float)region.start() / total_length) * total_width;
		float width = ((float)region.length() / total_length) * total_width;
		QRectF region_rect(label_width + 2 + x_start, 2, width, chr_height_);

		painter.setPen(QPen(Qt::red, 2));
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(region_rect);
	}
}

void ChromosomePanel::mouseMoveEvent(QMouseEvent* event)
{
	//init
	int x = event->pos().x();
	int y = event->pos().y();
	int w = width();
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();

	//show
	if (x>label_width + 2 && x<w - 2)
	{
		int coordinate = std::floor((double)(x-label_width - 2) / pixels_per_base_);
		emit mouseCoordinate(region.chr().strNormalized(true) + ":" + QString::number(coordinate));

		if (y >= 2 && y < 2 + chr_height_ + text_height_ + padding_)
		{
			setCursor(Qt::PointingHandCursor);
		}
		else
		{
			unsetCursor();
		}
	}
	else
	{
		emit mouseCoordinate("");
	}
}

void ChromosomePanel::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		int x = event->pos().x();
		int y = event->pos().y();
		int w = width();
		int label_width = SharedData::settings().label_width;
		const BedLine& region = SharedData::region();

		if (x > label_width + 2 && x < w - 2 &&
			y >= 2 && y < 2 + chr_height_ + text_height_ + padding_)
		{
			// new center coordinate
			int coordinate = std::floor((double)(x-label_width - 2) / pixels_per_base_);
			SharedData::setRegion(region.chr(), coordinate - region.length()/2, coordinate + region.length()/2);
		}

		// is_dragging_ = true;
		// drag_start_x_ = x;
	}
}

// void ChromosomePanel::mouseReleaseEvent(QMouseEvent* event)
// {
// 	int x = event->pos().x();
// 	int y = event->pos().y();
// 	int w = width();
// 	int label_width = SharedData::settings().label_width;
// 	const BedLine& region = SharedData::region();

// 	if (event->button() == Qt::LeftButton && is_dragging_)
// 	{
// 		if (x > label_width + 2 && x < w - 2)
// 		{
// 			if (y >= 2 && y < 2 + chr_height_ + text_height_ + padding_ && abs(x - drag_start_x_) < 5)
// 			{
// 				int coordinate = std::floor((double)(x-label_width - 2) / pixels_per_base_);
// 				SharedData::setRegion(region.chr(), coordinate - region.length()/2, coordinate + region.length()/2);
// 			}
// 		}
// 	}
// }
