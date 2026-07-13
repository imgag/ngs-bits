#include "IgvTrack.h"
#include "IgvTrackSettings.h"
#include "FileLoader.h"
#include "GenomeVisualizationWidget.h"
#include "SharedData.h"

#include <QActionGroup>
#include <QApplication>
#include <QPainter>
#include <QMenu>

IgvTrack::IgvTrack(QWidget* parent, QString file_path, QString name)
	: TrackWidget(parent, file_path, name)
{
	settings = QSharedPointer<IgvTrackSettings>::create(); // default
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(regionChanged()));
}

IgvTrack* IgvTrack::createTrack(QWidget* parent, QString file_path, QString name)
{
	QSharedPointer<BedFile> bed_file = FileLoader::loadIgvFile(file_path);
	QSharedPointer<IgvTrackSettings> settings = IgvTrackSettings::parseFromFile(bed_file);

	QString errors = settings->getValidationErrors();

	if (errors != "")
	{
		GenomeVisualizationWidget::displayError(errors);
		return nullptr;
	}

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
		igv_track->settings = settings;

		return igv_track;
	}
	else return nullptr;
}

QSize IgvTrack::sizeHint() const
{
	return QSize( parentWidget() ? parentWidget()->width() : 200, settings->track_height);
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
	if (settings->graph_mode != IgvTrackSettings::HEATMAP) drawScaleText(painter);
}

void IgvTrack::drawScaleText(QPainter& painter)
{
	Viewport viewport = getViewport();
	painter.setPen(Qt::black);
	QRect rec(viewport.x0, 0, width(), height());
	painter.drawText(rec, Qt::AlignLeft, "["+QString::number(settings->view_min) + "," + QString::number(settings->view_max) + "]");
}

void IgvTrack::drawPlot(QPainter& painter)
{
	const BedLine& region = SharedData::region();

	drawReferenceLine(painter, settings->view_min);
	drawReferenceLine(painter, .5f * (settings->view_min + settings->view_max));
	drawReferenceLine(painter, settings->view_max);

	int padding = region.length() / 3;
	int start = std::max(0, region.start() - padding);
	int end = region.end() + padding;
	const QVector<int>& idxes = chr_index_->matchingIndices(region.chr(), start, end);

	switch (settings->graph_mode)
	{
		case IgvTrackSettings::HEATMAP:
			drawHeatMap(painter, idxes);
			break;
		case IgvTrackSettings::BAR_CHART:
			drawBarChart(painter, idxes);
			break;
		case IgvTrackSettings::POINTS:
			drawPoints(painter, idxes);
			break;
		case IgvTrackSettings::LINE_PLOT:
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

		int py = valueToY(value);

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
		int py1 = valueToY(value1);

		int px2 = viewport.genomePosToScreen(pos2);
		int py2 = valueToY(value2);

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

	painter.fillRect(viewport.x0, 0, viewport.total_width, settings->track_height, Qt::gray);
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
		painter.drawRect(px, 0, dx, settings->track_height);
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

		float t = std::clamp(value, settings->view_min, settings->view_max);

		painter.setPen(Qt::blue);
		painter.setBrush(Qt::blue);
		int py = valueToY(t);
		int pend = valueToY(settings->view_min);
		int height = pend - py;
		painter.drawRect(px, py, dx, height);
	}
}


void IgvTrack::drawReferenceLine(QPainter& painter, float value)
{
	const Viewport& viewport = getViewport();
	painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
	int y = valueToY(value);
	painter.drawLine(viewport.x0, y, viewport.x0 + viewport.total_width, y);
}

int IgvTrack::valueToY(float value)
{
	constexpr int margin = 4;
	if (qFuzzyCompare(settings->view_max, settings->view_min)) return margin;


	int usable_height = settings->track_height - 2 * margin;
	float normalized_val = (value - settings->view_min) / (settings->view_max - settings->view_min);
	return margin + (int)((1.0f - normalized_val) * usable_height);
}

void IgvTrack::populateContextMenu(QMenu& menu, const QPoint& local_pos)
{
	QMenu* sub_menu = menu.addMenu("Type Of Graph");

	QAction* heat_map  = sub_menu->addAction("Heatmap");
	QAction* bar_chart = sub_menu->addAction("Bar Chart");
	QAction* points    = sub_menu->addAction("Points");
	QAction* line_plot = sub_menu->addAction("Line Plot");

	heat_map->setData(IgvTrackSettings::HEATMAP);
	bar_chart->setData(IgvTrackSettings::BAR_CHART);
	points->setData(IgvTrackSettings::POINTS);
	line_plot->setData(IgvTrackSettings::LINE_PLOT);

	auto* group = new QActionGroup(sub_menu);
	group->setExclusive(true);

	for (QAction* a : {heat_map, bar_chart, points, line_plot}) {
		a->setCheckable(true);
		group->addAction(a);
	}

	group->actions().at(static_cast<int>(settings->graph_mode))->setChecked(true);

	connect(group, &QActionGroup::triggered, this,
			[this](QAction* action)
			{
				settings->graph_mode = static_cast<IgvTrackSettings::GraphType>(action->data().toInt());
				update();
			});

	TrackWidget::populateContextMenu(menu, local_pos);
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

		int py = valueToY(value);

		if (settings->graph_mode == IgvTrackSettings::POINTS || settings->graph_mode == IgvTrackSettings::LINE_PLOT)
		{
			// Strict distance check for point structures
			if (std::abs(py - local_pos.y()) <= 6) candidates.push_back(idx);
		}
		else if (settings->graph_mode == IgvTrackSettings::BAR_CHART)
		{
			// For bar charts, anywhere within the bar height counts
			int p_zero = valueToY(settings->view_min);
			if (local_pos.y() >= py && local_pos.y() <= p_zero) candidates.push_back(idx);
		}
		else if (settings->graph_mode == IgvTrackSettings::HEATMAP)
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
	auto widget_settings = TrackWidget::getSettings();
	auto track_settings = settings->getSettings();
	widget_settings.insert(track_settings);
	return widget_settings;
}

void IgvTrack::loadKeyValueFromXml(QString key, QString value)
{
	settings->loadKeyValueFromXml(key, value);
}

QString IgvTrack::getTrackNameFromIgvFile(QSharedPointer<BedFile> bed_file)
{
	//this function assumes isValidIgvFile has already been called

	if (!bed_file) return "";

	foreach (QByteArray header, bed_file->headers())
	{
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
		else if (!header.startsWith("#"))
		{
			// fall back, use fourth column as name
			QList<QByteArray> columns = header.split('\t');

			if (columns.count() >= 5) return QString(columns[4]);
		}
	}
	return "";
}
