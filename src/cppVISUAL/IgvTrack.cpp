#include "IgvTrack.h"
#include "FileLoader.h"
#include "SharedData.h"

#include <QActionGroup>
#include <QApplication>
#include <QPainter>
#include <QMenu>

IgvTrack::IgvTrack(QWidget* parent, QString file_path, QString name)
	: TrackWidget(parent, file_path, name)
{
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(regionChanged()));
}

IgvTrack* IgvTrack::createTrack(QWidget* parent, QString file_path, QString name)
{
	QSharedPointer<BedFile> bed_file = FileLoader::loadIgvFile(file_path);
	// TODO: validate if header is valid here first for instance view lim [1.4, 1.2] makes no sense
	if (bed_file)
	{
		QString display_name = name;
		if (display_name == "")
		{
			// load from file first
			display_name = getTrackNameFromIgvFile(bed_file);
			// fallback: set the name to the file name
			if (display_name == "") display_name = getDisplayNameFromFilePath(file_path);
		}
		IgvTrack* igv_track = new IgvTrack(parent, file_path, display_name);
		igv_track->setBedFile(bed_file);
		igv_track->parseTrackHeader(bed_file);

		return igv_track;
	}
	else return nullptr;
}

QSize IgvTrack::sizeHint() const
{
	return QSize( parentWidget() ? parentWidget()->width() : 200, track_height_);
}

void IgvTrack::setBedFile(QSharedPointer<BedFile> bed_file)
{
	if (bed_file)
	{
		bed_file_ = bed_file;
		chr_index_ = std::make_unique<ChromosomalIndex<BedFile>>(*bed_file);
		chr_index_->createIndex();
		update();
	}
}

void IgvTrack::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.fillRect(rect(), Qt::white);
	drawLabel(painter);
	if (bed_file_) drawPlot(painter);
	if (graph_mode_ != HEATMAP) drawScaleText(painter);
}

void IgvTrack::drawScaleText(QPainter& painter)
{
	Viewport viewport = getViewport();
	painter.setPen(Qt::black);
	QRect rec(viewport.x0, 0, width(), height());
	painter.drawText(rec, Qt::AlignLeft, "["+QString::number(view_min_) + "," + QString::number(view_max_) + "]");
}

void IgvTrack::drawPlot(QPainter& painter)
{
	const BedLine& region = SharedData::region();
	const Viewport& viewport = getViewport();

	drawReferenceLine(painter, view_min_, viewport.x0, viewport.total_width, track_height_);
	drawReferenceLine(painter, .5f * (view_min_ + view_max_), viewport.x0, viewport.total_width, track_height_);
	drawReferenceLine(painter, view_max_, viewport.x0, viewport.total_width, track_height_);

	int padding = region.length() / 3;
	int start = std::max(0, region.start() - padding);
	int end = region.end() + padding;
	const QVector<int>& idxes = chr_index_->matchingIndices(region.chr(), start, end);

	switch (graph_mode_)
	{
		case HEATMAP:
			drawHeatMap(painter, idxes);
			break;
		case BAR_CHART:
			drawBarChart(painter, idxes);
			break;
		case POINTS:
			drawPoints(painter, idxes);
			break;
		case LINE_PLOT:
			drawLinePlot(painter, idxes);
			break;
	}
}

void IgvTrack::drawPoints(QPainter& painter, const QVector<int>& idxes)
{
	const BedLine& region = SharedData::region();
	const Viewport& viewport = getViewport();

	foreach (int idx, idxes)
	{
		const BedLine& bd = (*bed_file_)[idx];
		if (bd.annotations().count() <= 1) continue;
		bool ok;
		float value = bd.annotations()[1].toFloat(&ok);
		if (!ok) continue;

		int pos = (bd.start() + bd.end()) / 2;

		if (pos < region.start()) continue;

		float p1 = viewport.genomePosToScreen(pos);
		float p2 = viewport.genomePosToScreen(pos + 1);
		float px = (p1 + p2) / 2.f;

		int py = valueToY(value, track_height_);

		painter.setBrush(Qt::blue);
		painter.drawEllipse(QPoint(px, py), 2, 2);
	}
}

void IgvTrack::drawLinePlot(QPainter& painter, const QVector<int>& idxes)
{
	painter.setRenderHint(QPainter::Antialiasing, true);
	const Viewport& viewport = getViewport();

	for (int i =1; i < idxes.count(); ++i)
	{
		const BedLine& bd1 = (*bed_file_)[idxes[i-1]];
		const BedLine& bd2 = (*bed_file_)[idxes[i]];

		if (bd1.annotations().count() <= 1 ||
			bd2.annotations().count() <= 1) continue;
		bool ok;

		float value1 = bd1.annotations()[1].toFloat(&ok);
		if (!ok) continue;
		float value2 = bd2.annotations()[1].toFloat(&ok);
		if (!ok) continue;

		int pos = (bd1.start() + bd1.end()) / 2;
		int pos2 = (bd2.start() + bd2.end()) / 2;

		int px1 = viewport.genomePosToScreen(pos);
		int py1 = valueToY(value1, track_height_);

		int px2 = viewport.genomePosToScreen(pos2);
		int py2 = valueToY(value2, track_height_);

		if ((px1 < viewport.x0 && px2 < viewport.x0) ||
			((px1 > viewport.x0 + viewport.total_width && px2 > viewport.x0 + viewport.total_width))) continue;


		if (px1 <= viewport.x0 && px2 >= viewport.x0) // need to interpolate px1
		{
			// y - y1 = (y2 - y1)/(x2 - x1)(x - x1)
			// y - y1 = m(x0-x1) --> y = m(x0 - x1) + y1
			if (px1 != px2)
			{
				float m = (py2 - py1)/(float)(px2 - px1);
				py1 = m*(viewport.x0 - px1) + py1;
			}
			px1 = viewport.x0;
		}

		// px1 < width && px2 > width case is automatically handled by Qt

		painter.setPen(QPen(Qt::blue, 2, Qt::SolidLine));
		painter.drawLine(px1, py1, px2, py2);
	}
}

static const QColor HEATMAP_COL1 = QColor(122, 122, 214, 90);
static const QColor HEATMAP_COL2 = QColor(0, 0, 255, 255);


void IgvTrack::drawHeatMap(QPainter& painter, const QVector<int>& idxes)
{
	painter.setRenderHint(QPainter::Antialiasing);

	const BedLine& region = SharedData::region();
	const Viewport& viewport = getViewport();

	painter.fillRect(viewport.x0, 0, viewport.total_width, track_height_, Qt::gray);
	foreach (int idx, idxes)
	{
		const BedLine& bd = (*bed_file_)[idx];
		if (bd.annotations().count() <= 1) continue;
		bool ok;
		float value = bd.annotations()[1].toFloat(&ok);
		if (!ok) continue;

		int pos = (bd.start() + bd.end()) / 2;

		if (pos < region.start()) continue;

		float px = viewport.genomePosToScreen(pos);
		float endx = viewport.genomePosToScreen(pos + 1);
		float dx = endx - px;

		float t = std::clamp(value, 0.0f, 1.0f);

		int r = HEATMAP_COL1.red()   + t * (HEATMAP_COL2.red()   - HEATMAP_COL1.red());
		int g = HEATMAP_COL1.green() + t * (HEATMAP_COL2.green() - HEATMAP_COL1.green());
		int b = HEATMAP_COL1.blue()  + t * (HEATMAP_COL2.blue()  - HEATMAP_COL1.blue());
		int a = HEATMAP_COL1.alpha() + t * (HEATMAP_COL2.alpha() - HEATMAP_COL1.alpha());

		QColor color(r, g, b, a);

		painter.setPen(color);
		painter.setBrush(color);
		painter.drawRect(px, 0, dx, track_height_);
	}
}

void IgvTrack::drawBarChart(QPainter& painter, const QVector<int>& idxes)
{
	painter.setRenderHint(QPainter::Antialiasing);

	const BedLine& region = SharedData::region();
	const Viewport& viewport = getViewport();

	foreach (int idx, idxes)
	{
		const BedLine& bd = (*bed_file_)[idx];
		if (bd.annotations().count() <= 1) continue;
		bool ok;
		float value = bd.annotations()[1].toFloat(&ok);
		if (!ok) continue;

		int pos = (bd.start() + bd.end()) / 2;

		if (pos < region.start()) continue;

		float px = viewport.genomePosToScreen(pos);
		float endx = viewport.genomePosToScreen(pos + 1);
		float dx = endx - px;

		float t = std::clamp(value, view_min_, view_max_);

		painter.setPen(Qt::blue);
		painter.setBrush(Qt::blue);
		int py = valueToY(t, track_height_);
		int pend = valueToY(view_min_, track_height_);
		int height = pend - py;
		painter.drawRect(px, py, dx, height);
	}
}


void IgvTrack::drawReferenceLine(QPainter& painter, float value_value, int x0, int total_width, int total_height)
{
	painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
	int y = valueToY(value_value, total_height);
	painter.drawLine(x0, y, x0 + total_width, y);
}

int IgvTrack::valueToY(float value, int draw_height)
{
	constexpr int margin = 4;
	if (qFuzzyCompare(view_max_, view_min_)) return margin;


	int usable_height = draw_height - 2 * margin;
	float normalized_val = (value - view_min_) / (view_max_ - view_min_);
	return margin + (int)((1.0f - normalized_val) * usable_height);
}

void IgvTrack::populateContextMenu(QMenu& menu)
{
	QMenu* sub_menu = menu.addMenu("Type Of Graph");

	QAction* heat_map  = sub_menu->addAction("Heatmap");
	QAction* bar_chart = sub_menu->addAction("Bar Chart");
	QAction* points    = sub_menu->addAction("Points");
	QAction* line_plot = sub_menu->addAction("Line Plot");

	heat_map->setData(HEATMAP);
	bar_chart->setData(BAR_CHART);
	points->setData(POINTS);
	line_plot->setData(LINE_PLOT);

	auto* group = new QActionGroup(sub_menu);
	group->setExclusive(true);

	for (QAction* a : {heat_map, bar_chart, points, line_plot}) {
		a->setCheckable(true);
		group->addAction(a);
	}

	group->actions().at(static_cast<int>(graph_mode_))->setChecked(true);

	connect(group, &QActionGroup::triggered, this,
			[this](QAction* action)
			{
				graph_mode_ = static_cast<GraphType>(action->data().toInt());
				update();
			});

	TrackWidget::populateContextMenu(menu);
}

void IgvTrack::mousePressEvent(QMouseEvent* event)
{
	mouse_press_pos_ = event->pos();
	TrackWidget::mousePressEvent(event);
}

void IgvTrack::mouseReleaseEvent(QMouseEvent* event)
{
	QPoint pos = event->pos();
	if ((pos - mouse_press_pos_).manhattanLength() >= QApplication::startDragDistance()
		|| event->button() != Qt::LeftButton)
	{
		TrackWidget::mousePressEvent(event); return;
	}

	handlePopupRequest(pos, event->globalPosition());
}

void IgvTrack::handlePopupRequest(QPoint local_pos, QPointF global_pos)
{
	const BedLine& region = SharedData::region();
	const Viewport viewport = getViewport();

	if (viewport.isOutOfDrawRegion(local_pos.x())) return;

	int click_position = viewport.screenXToGenomePos(local_pos.x());

	double bp_per_pixel = 1.0 / viewport.pixels_per_base;
	int padding = std::max(2, static_cast<int>(bp_per_pixel * 2));

	int start = std::max(0, click_position - padding);
	int end = click_position + padding;

	const QVector<int>& idxes = chr_index_->matchingIndices(region.chr(), start, end);
	QVector<int> candidates;

	foreach (int idx, idxes)
	{
		const BedLine& bd = (*bed_file_)[idx];
		if (bd.annotations().count() <= 1) continue;

		bool ok;
		float value = bd.annotations()[1].toFloat(&ok);
		if (!ok) continue;

		int py = valueToY(value, track_height_);

		if (graph_mode_ == POINTS || graph_mode_ == LINE_PLOT)
		{
			// Strict distance check for point structures
			if (std::abs(py - local_pos.y()) <= 6) candidates.push_back(idx);
		}
		else if (graph_mode_ == BAR_CHART)
		{
			// For bar charts, anywhere within the bar height counts
			int p_zero = valueToY(view_min_, track_height_);
			if (local_pos.y() >= py && local_pos.y() <= p_zero) candidates.push_back(idx);
		}
		else if (graph_mode_ == HEATMAP)
		{
			// For heatmaps, the whole vertical span of the track represents the data
			candidates.push_back(idx);
		}
	}

	if (candidates.empty()) return;

	QString info;
	foreach (int idx, candidates)
	{
		const BedLine& bd = (*bed_file_)[idx];
		info += getIgvText(bd);
	}
	showInfoPopup(global_pos, info);
}

QString IgvTrack::getIgvText(const BedLine& bd)
{
	return QString("%1: %2 %3, Value: %4\n")
		.arg(bd.chr().str())
		.arg(bd.start())
		.arg(bd.end())
		.arg(bd.annotations()[1]);
}

QMap<QString, QVariant> IgvTrack::getSettings()
{
	auto settings = TrackWidget::getSettings();
	settings["graph_mode"] = graph_mode_;
	return settings;
}

void IgvTrack::loadKeyValueFromXml(QString key, const QDomElement& item)
{
	if (key == "graph_mode")
	{
		int value = item.attribute("value").toInt();
		if (value != -1) graph_mode_ = static_cast<GraphType>(value);
	}
}

QString IgvTrack::getTrackNameFromIgvFile(QSharedPointer<BedFile> bed_file)
{
	//this function assumes isValidIgvFile has already been called

	if (!bed_file) return "";

	foreach (QByteArray header, bed_file->headers())
	{
		qDebug() << header << Qt::endl;
		if (header.startsWith("#track"))
		{
			QList<QByteArray> kv_pairs = header.split(' ');
			foreach (QByteArray attr, kv_pairs)
			{
				int idx = attr.indexOf("=");
				if (idx != -1)
				{
					QByteArray key = attr.left(idx);
					QByteArray val = attr.mid(idx + 1);
					if (key.toLower() == "name") return QString(val);
				}
			}
			// could not find name key
		}
		else
		{
			// fall back, use fourth column as name
			QList<QByteArray> columns = header.split('\t');

			if (columns.count() >= 5) return QString(columns[4]);
		}
	}
	return "";
}

void IgvTrack::parseTrackHeader(QSharedPointer<BedFile> bed_file)
{
	if (!bed_file) return;

	for (const QByteArray& header : bed_file->headers())
	{
		if (header.startsWith("#track"))
		{
			QList<QByteArray> kv_pairs = header.split(' ');

			foreach (const QByteArray& attr, kv_pairs)
			{
				int idx = attr.indexOf('=');
				if (idx == -1) continue;

				QByteArray key = attr.left(idx).toLower();
				QByteArray val = attr.mid(idx + 1);

				// 1. Parse graph type
				if (key == "graphtype")
				{
					if (val.toLower() == "points") graph_mode_ = POINTS;
					else if (val.toLower() == "heatmap") graph_mode_ = HEATMAP;
					else if (val.toLower() == "bar_chart") graph_mode_ = BAR_CHART;
					else if (val.toLower() == "line_plot") graph_mode_ = LINE_PLOT;
				}
				// 2. Parse view limits (e.g., "-0.2:1.2")
				else if (key == "viewlimits")
				{
					QList<QByteArray> limits = val.split(':');
					if (limits.size() == 2)
					{
						bool okMin, okMax;
						float minVal = limits[0].toFloat(&okMin);
						float maxVal = limits[1].toFloat(&okMax);
						if (okMin && okMax)
						{
							view_min_ = minVal;
							view_max_ = maxVal;
						}
					}
				}
				// 3. Parse track height max bounds (e.g., "80:80:80")
				else if (key == "maxheightpixels")
				{
					QList<QByteArray> heights = val.split(':');
					if (!heights.isEmpty())
					{
						bool ok;
						int parsed_height = heights[0].toInt(&ok); // Take default/min target target height
						if (ok)
						{
							track_height_ = parsed_height;
							// Trigger a layout resize update if parent widget manages size hints
							this->updateGeometry();
						}
					}
				}
			}
			break; // Header found and parsed
		}
	}
}
