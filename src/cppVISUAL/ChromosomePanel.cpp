#include "ChromosomePanel.h"
#include "SharedData.h"
#include "NGSHelper.h"
#include "Settings.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QMouseEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <cmath>

ChromosomePanel::ChromosomePanel(QWidget* parent)
	: QWidget(parent)
{
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(updateRegion()));
	connect(SharedData::instance(), &SharedData::settingsChanged, this, &ChromosomePanel::updateRegion);
	connect(SharedData::instance(), &SharedData::bookmarksChanged, this, &ChromosomePanel::updateRegion);
}

void ChromosomePanel::updateRegion()
{
	bookmark_regions_.clear();
	par_regions_.clear();
	update();
}

int ChromosomePanel::genomicCoordinateAt(int x) const
{
	const int x0 = SharedData::settings().label_width + 2;
	const int usable_width = width() - x0 - 2;
	if (usable_width <= 0 || x < x0 || x >= x0 + usable_width) return 0;

	static const BedFile bands = NGSHelper::cytoBands();
	int start = -1;
	int end = 0;
	for (int i = 0; i < bands.count(); ++i)
	{
		if (bands[i].chr() != SharedData::region().chr()) continue;
		if (start < 0 || bands[i].start() < start) start = bands[i].start();
		end = std::max(end, bands[i].end());
	}
	if (start < 0) return 0;
	return 1 + int(double(x - x0) * (end - start + 1) / usable_width);
}


QColor ChromosomePanel::cytobandColor(QString stain)
{
	if (stain == "gneg") return QColor(255, 255, 255);
	if (stain == "gpos25") return QColor(191, 191, 191);
	if (stain == "gpos50") return QColor(127, 127, 127);
	if (stain == "gpos75") return QColor(63, 63, 63);
	if (stain == "gpos100") return QColor(0, 0 ,0);
	if (stain == "acen") return QColor(178, 0, 0);

	return QColor(255, 255, 255);
}

ChromosomePanel::ChromosomeIndexMap ChromosomePanel::buildChrIndexMap(const BedFile& bands){
	ChromosomeIndexMap map;
	for (int i =0; i < bands.count(); ++i)
	{
		map[bands[i].chr()].append(i);
	}
	return map;
}

void ChromosomePanel::paintEvent(QPaintEvent* /*event*/)
{
	bookmark_regions_.clear();
	par_regions_.clear();
	//init
	int h = height();
	int w = width();
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();
	band_y_ = region.chr().isX() ? 8 : 2;
	const int drawing_height = std::max(0, h - (band_y_ - 2));
	text_height_ = .5 * drawing_height;
	chr_height_ = .25 * drawing_height;

	QPainter painter(this);

	painter.setRenderHint(QPainter::Antialiasing, true);

	static BedFile bands = NGSHelper::cytoBands();
	static bool sorted = false;
	if (!sorted)
	{
		bands.sort();
		sorted = true;
	}

	static ChromosomeIndexMap chr_idx_map = buildChrIndexMap(bands);

	// painter.fillRect(rect(), Qt::white);

	QVector<int> idxes = chr_idx_map[region.chr()];
	if (idxes.size() >= 1)
	{
		int total_width = (w - label_width - 4);
		int total_length = bands[idxes[idxes.length() - 1]].end() - bands[idxes[0]].start() + 1;
		pixels_per_base_ = (double)(total_width) / (double)total_length;

		drawBands(&painter, bands, idxes, total_width, total_length);
		if (region.chr().isX()) drawPseudoAutosomalRegions(painter, total_length);

		//drawing rectangle for the region
		drawRegion(&painter, total_length);
		drawBookmarks(painter, total_length);


		drawDragRegion(&painter);
	}
}

void ChromosomePanel::drawBookmarks(QPainter& painter, int total_length)
{
	const int x0 = SharedData::settings().label_width + 2;
	const int usable_width = width() - x0 - 2;
	if (usable_width <= 0 || total_length <= 0) return;
	constexpr int indicator_height = 4;
	const double y = std::ceil(band_y_ + chr_height_) + 1;
	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setClipRect(QRectF(x0, y, usable_width, indicator_height));
	const QColor purple(128, 0, 128);
	const QStringList bookmarks = Settings::stringList("bookmark_regions", true);
	for (const QString& entry : bookmarks)
	{
		const QJsonObject bookmark = QJsonDocument::fromJson(entry.toUtf8()).object();
		if (!bookmark.value("show_in_chromosome_view").toBool(false)) continue;
		if (Chromosome(bookmark.value("chr").toString()) != SharedData::region().chr()) continue;
		const int start = bookmark.value("start").toInt();
		const int end = bookmark.value("end").toInt();
		if (start < 1 || end < start || start > total_length) continue;
		const double x = x0 + (start - 1) * pixels_per_base_;
		const double length = (std::min(end, total_length) - start + 1) * pixels_per_base_;
		const QRectF rect = QRectF(x, y, std::max(1.0, length), indicator_height).intersected(QRectF(x0, y, usable_width, indicator_height));
		painter.fillRect(rect, purple);
		bookmark_regions_.append({rect, bookmark.value("name").toString()});
	}
	painter.restore();
}

void ChromosomePanel::drawPseudoAutosomalRegions(QPainter& painter, int total_length)
{
	const int x0 = SharedData::settings().label_width + 2;
	const int usable_width = width() - x0 - 2;
	if (usable_width <= 0 || total_length <= 0) return;
	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setClipRect(QRectF(x0, 2, usable_width, 4));
	const QColor color(Qt::darkGray);
	const BedFile& par = NGSHelper::pseudoAutosomalRegion();
	for (int i = 0; i < par.count(); ++i)
	{
		const BedLine& region = par[i];
		if (!region.chr().isX()) continue;
		const double x = x0 + (region.start() - 1) * pixels_per_base_;
		const double length = region.length() * pixels_per_base_;
		const QRectF rect = QRectF(x, 2, std::max(1.0, length), 4).intersected(QRectF(x0, 2, usable_width, 4));
		painter.fillRect(rect, color);
		par_regions_.append({rect, QString("Pseudoautosomal region: %1").arg(region.toString(true))});
	}
	painter.restore();
}

bool ChromosomePanel::event(QEvent* event)
{
	if (event->type() == QEvent::ToolTip)
	{
		const auto* help = static_cast<QHelpEvent*>(event);
		QStringList names;
		QRectF tooltip_rect;
		for (const auto& bookmark : bookmark_regions_)
		{
			if (!bookmark.first.contains(help->pos())) continue;
			names.append("Bookmark: " + bookmark.second.toHtmlEscaped());
			tooltip_rect = tooltip_rect.united(bookmark.first);
		}
		for (const auto& par : par_regions_)
		{
			if (!par.first.contains(help->pos())) continue;
			names.append(par.second.toHtmlEscaped());
			tooltip_rect = tooltip_rect.united(par.first);
		}
		if (!names.isEmpty()) QToolTip::showText(help->globalPos(), "<qt>" + names.join("<br>") + "</qt>", this, tooltip_rect.toAlignedRect());
		else
		{
			QToolTip::hideText();
			event->ignore();
		}
		return true;
	}
	return QWidget::event(event);
}

void ChromosomePanel::mouseMoveEvent(QMouseEvent* event)
{
	if (is_dragging_)
	{
		drag_current_x_ = event->pos().x();
		update();
	}
}

void ChromosomePanel::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		int x = event->pos().x();
		is_dragging_ = true;
		drag_start_x_ = x;
	}
}

void ChromosomePanel::mouseReleaseEvent(QMouseEvent* event)
{
	int x = event->pos().x();
	int y = event->pos().y();
	int w = width();
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();

	if (event->button() == Qt::LeftButton && is_dragging_)
	{
		if (x > label_width + 2 && x < w - 2)
		{
			int y_end = band_y_ + chr_height_ + text_height_ + padding_;
			if (y >= band_y_ && y < y_end && abs(x - drag_start_x_) < 5)
			{
				int coordinate = std::floor((double)(x-label_width - 2) / pixels_per_base_);
				SharedData::setRegion(region.chr(), coordinate - region.length()/2, coordinate + region.length()/2);
			}

			else if (abs(x - drag_start_x_) >= 5)
			{
				int start = std::floor((double)(drag_start_x_ -label_width - 2) / pixels_per_base_);
				int end = std::floor((double)(x -label_width - 2) / pixels_per_base_);
				SharedData::setRegion(region.chr(), std::min(start, end), std::max(start, end));
			}
		}
	}

	is_dragging_ = false;
	update();
}


void ChromosomePanel::drawBands(QPainter* painter, const BedFile& bands, QVector<int> idxes,
								int total_width, int total_length)
{
	int label_width = SharedData::settings().label_width;
	float width_cum_sum = 0;
	char prev_arm = ' ';
	char current_arm;
	foreach (int i, idxes)
	{
		//drawing the band
		current_arm = bands[i].annotations()[0].at(0);

		float frac = (float)bands[i].length() / (total_length);
		float current_band_width = frac * total_width;
		QRectF rect(label_width + 2 + width_cum_sum, band_y_, current_band_width, chr_height_);

		// if color given in annotations - use that
		if (bands[i].annotations().size() > 1)
		{
			painter->setBrush(cytobandColor(bands[i].annotations()[1]));
		}
		else
		{
			painter->setBrush((i % 2 == 0) ? Qt::black : Qt::darkGray);
		}
		// arm switch - draw triangle
		if (current_arm == 'p' && i < bands.count()-1 && bands[i+1].annotations()[0].at(0) == 'q')
		{
			float p_start_x = label_width + 2 + width_cum_sum;
			QPolygonF p_tri;
			p_tri << QPointF(p_start_x, band_y_)
				  << QPointF(p_start_x + current_band_width, band_y_ + chr_height_/2.0)
				  << QPointF(p_start_x, band_y_ + chr_height_);
			painter->drawPolygon(p_tri);
		}
		else if (prev_arm == 'p' && current_arm == 'q')
		{

			float q_start_x = label_width + 2 + width_cum_sum;
			QPolygonF q_tri;
			q_tri << QPointF(q_start_x + current_band_width, band_y_)
				  << QPointF(q_start_x, band_y_ + chr_height_/2.0)
				  << QPointF(q_start_x + current_band_width, band_y_ + chr_height_);
			painter->drawPolygon(q_tri);
		}
		else
		{
			painter->drawRect(rect);
		}

		prev_arm = current_arm;

		//drawing the text
		QFont font = painter->font();
		font.setPointSize(8);
		painter->setFont(font);

		QRectF text_rect(label_width + 2 + width_cum_sum, band_y_ + chr_height_ + padding_, current_band_width, text_height_);
		QString label = bands[i].annotations()[0];
		int text_pixel_width = painter->fontMetrics().horizontalAdvance(label);
		if (current_band_width > text_pixel_width)
		{
			painter->setPen(Qt::black);
			painter->drawText(text_rect, Qt::AlignCenter, label);
		}

		width_cum_sum += current_band_width;
	}
}


void ChromosomePanel::drawRegion(QPainter* painter, int total_length)
{
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();
	int total_width = (width() - label_width - 4);


	float x_start = ((float)region.start() / total_length) * total_width;
	float width = ((float)region.length() / total_length) * total_width;
	QRectF region_rect(label_width + 2 + x_start, band_y_, width, chr_height_);

	painter->setPen(QPen(Qt::red, 2));
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(region_rect);
}


void ChromosomePanel::drawDragRegion(QPainter* painter)
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
