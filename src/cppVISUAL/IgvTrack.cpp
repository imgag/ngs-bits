#include "IgvTrack.h"
#include "Exceptions.h"
#include "FileLoader.h"
#include "GenomeVisualizationWidget.h"
#include "SharedData.h"
#include <QMessageBox>
#include <QActionGroup>
#include <QApplication>
#include <QPainter>
#include <QMenu>
#include <cmath>

namespace
{
	const QByteArray HEATMAP = "HEATMAP";
	const QByteArray BAR_CHART = "BAR_CHART";
	const QByteArray POINTS = "POINTS";
	const QByteArray LINE_PLOT = "LINE_PLOT";
	const QByteArrayList GRAPH_MODES{HEATMAP, BAR_CHART, POINTS, LINE_PLOT};
}

QSharedPointer<ParameterList> IgvTrack::parametersFromFile(QSharedPointer<BedFile> bed_file)
{
	QList<Parameter> config;
	config.append(Parameter("graph_mode", "Type of graph", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, GRAPH_MODES.join('\t')}}));
	config.append(Parameter("track_height", "Track height in pixels", ParameterType::INT, {{ConstraintType::MIN, 1}}));
	config.append(Parameter("view_min", "Lower plot limit", ParameterType::FLOAT, {}));
	config.append(Parameter("view_max", "Upper plot limit", ParameterType::FLOAT, {}));
	QHash<QByteArray, QVariant> defaults{{"graph_mode", POINTS}, {"track_height", 100}, {"view_min", 0.0}, {"view_max", 1.0}};
	if (bed_file)
	{
		for (const QByteArray& header : bed_file->headers())
		{
			if (!header.startsWith("#track")) continue;
			for (const QByteArray& attr : header.split(' '))
			{
				int idx = attr.indexOf('=');
				if (idx == -1) continue;
				QByteArray key = attr.left(idx).toLower();
				QByteArray value = attr.mid(idx + 1);
				if (key == "graphtype")
				{
					QByteArray mode = value.toUpper();
					if (GRAPH_MODES.contains(mode)) defaults["graph_mode"] = mode;
				}
				else if (key == "viewlimits")
				{
					QByteArrayList limits = value.split(':');
					if (limits.size() == 2)
					{
						bool ok_min = false, ok_max = false;
						float minimum = limits[0].toFloat(&ok_min);
						float maximum = limits[1].toFloat(&ok_max);
						if (ok_min && ok_max)
						{
							defaults["view_min"] = minimum;
							defaults["view_max"] = maximum;
						}
					}
				}
				else if (key == "maxheightpixels")
				{
					bool ok = false;
					int height = value.split(':').first().toInt(&ok);
					if (ok) defaults["track_height"] = height;
				}
			}
			break;
		}
	}
	auto parameters = QSharedPointer<ParameterList>::create(type(), config, defaults);
	if (parameters->getFloat("view_min") >= parameters->getFloat("view_max"))
	{
		THROW(ArgumentException, "View min >= view max in IGV track.");
	}
	return parameters;
}

void IgvTrack::setParameters(QSharedPointer<ParameterList> parameters)
{
	if (settings_) disconnect(settings_.data(), nullptr, this, nullptr);
	settings_ = parameters;
	connect(settings_.data(), &ParameterList::parameterChanged, this, [this]()
	{
		updateGeometry();
		update();
	});
	updateGeometry();
	update();
}

IgvTrack::IgvTrack(QWidget* parent, QString file_path, QString name)
	: TrackWidget(parent, file_path, name, type())
{
	setParameters(parametersFromFile(nullptr));
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(regionChanged()));
}

IgvTrack::~IgvTrack()
{
}

IgvTrack* IgvTrack::createTrack(QWidget* parent, QString file_path, QString name)
{
	QSharedPointer<BedFile> bed_file = FileLoader::loadIgvFile(file_path);
	if (!bed_file) return nullptr;
	QSharedPointer<ParameterList> parameters;
	try
	{
		parameters = parametersFromFile(bed_file);
	}
	catch (const ArgumentException& e)
	{
		QMessageBox::warning(QApplication::activeWindow(), "Error", e.message());
		return nullptr;
	}

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
	igv_track->setParameters(parameters);
	return igv_track;
}

QSize IgvTrack::sizeHint() const
{
	return QSize( parentWidget() ? parentWidget()->width() : 200, settings_->getInt("track_height"));
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
	if (settings_->getString("graph_mode") != HEATMAP) drawScaleText(painter);
}

void IgvTrack::drawScaleText(QPainter& painter)
{
	const double view_min = settings_->getFloat("view_min");
	const double view_max = settings_->getFloat("view_max");
	Viewport viewport = getViewport();
	painter.setPen(Qt::black);
	QRect rec(viewport.x0, 0, width(), height());
	painter.drawText(rec, Qt::AlignLeft, "["+QString::number(view_min) + "," + QString::number(view_max) + "]");
}

IgvTrack::PlotScale IgvTrack::plotScale() const
{
	return {settings_->getFloat("view_min"), settings_->getFloat("view_max"), settings_->getInt("track_height")};
}

void IgvTrack::drawPlot(QPainter& painter)
{
	const BedLine& region = SharedData::region();
	const PlotScale scale = plotScale();
	const QByteArray graph_mode = settings_->getString("graph_mode");
	//XML settings are restored one at a time; do not draw an invalid intermediate range.
	if (scale.minimum >= scale.maximum) return;

	drawReferenceLine(painter, scale.minimum, scale);
	drawReferenceLine(painter, .5 * (scale.minimum + scale.maximum), scale);
	drawReferenceLine(painter, scale.maximum, scale);

	int padding = region.length() / 3;
	int start = std::max(0, region.start() - padding);
	int end = region.end() + padding;
	const QVector<int>& idxes = chr_index_->matchingIndices(region.chr(), start, end);

	if (graph_mode == HEATMAP) drawHeatMap(painter, idxes, scale);
	else if (graph_mode == BAR_CHART) drawBarChart(painter, idxes, scale);
	else if (graph_mode == POINTS) drawPoints(painter, idxes, scale);
	else if (graph_mode == LINE_PLOT) drawLinePlot(painter, idxes, scale);
}

void IgvTrack::drawPoints(QPainter& painter, const QVector<int>& idxes, const PlotScale& scale)
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

		int py = valueToY(value, scale);

		painter.setBrush(Qt::blue);
		painter.drawEllipse(QPoint(px, py), 2, 2);
	}
}

void IgvTrack::drawLinePlot(QPainter& painter, const QVector<int>& idxes, const PlotScale& scale)
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
		int py1 = valueToY(value1, scale);

		int px2 = viewport.genomePosToScreen(pos2);
		int py2 = valueToY(value2, scale);

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


void IgvTrack::drawHeatMap(QPainter& painter, const QVector<int>& idxes, const PlotScale& scale)
{
	painter.setRenderHint(QPainter::Antialiasing);

	const BedLine& region = SharedData::region();
	const Viewport& viewport = getViewport();

	painter.fillRect(viewport.x0, 0, viewport.total_width, scale.height, Qt::gray);
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
		painter.drawRect(px, 0, dx, scale.height);
	}
}

void IgvTrack::drawBarChart(QPainter& painter, const QVector<int>& idxes, const PlotScale& scale)
{
	painter.setRenderHint(QPainter::Antialiasing);

	const BedLine& region = SharedData::region();
	const Viewport& viewport = getViewport();
	const int pend = valueToY(scale.minimum, scale);

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

		float t = std::clamp(value, static_cast<float>(scale.minimum), static_cast<float>(scale.maximum));

		painter.setPen(Qt::blue);
		painter.setBrush(Qt::blue);
		int py = valueToY(t, scale);
		int height = pend - py;
		painter.drawRect(px, py, dx, height);
	}
}


void IgvTrack::drawReferenceLine(QPainter& painter, float value, const PlotScale& scale)
{
	const Viewport& viewport = getViewport();
	painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
	int y = valueToY(value, scale);
	painter.drawLine(viewport.x0, y, viewport.x0 + viewport.total_width, y);
}

int IgvTrack::valueToY(float value, const PlotScale& scale)
{
	constexpr int margin = 4;
	if (qFuzzyCompare(scale.maximum, scale.minimum)) return margin;


	int usable_height = scale.height - 2 * margin;
	float normalized_val = (value - scale.minimum) / (scale.maximum - scale.minimum);
	return margin + (int)((1.0f - normalized_val) * usable_height);
}

void IgvTrack::populateContextMenu(QMenu& menu, const QPoint& local_pos)
{
	const QByteArray graph_mode = settings_->getString("graph_mode");
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
		a->setChecked(a->data().toByteArray() == graph_mode);
		group->addAction(a);
	}

	connect(group, &QActionGroup::triggered, this,
			[this](QAction* action)
			{
				settings_->setString("graph_mode", action->data().toByteArray());
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
	if (!chr_index_) return;
	const PlotScale scale = plotScale();
	if (scale.minimum >= scale.maximum) return;
	const QByteArray graph_mode = settings_->getString("graph_mode");
	const int p_zero = valueToY(scale.minimum, scale);
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

		int py = valueToY(value, scale);

		if (graph_mode == POINTS || graph_mode == LINE_PLOT)
		{
			// Strict distance check for point structures
			if (std::abs(py - local_pos.y()) <= 6) candidates.push_back(idx);
		}
		else if (graph_mode == BAR_CHART)
		{
			// For bar charts, anywhere within the bar height counts
			if (local_pos.y() >= py && local_pos.y() <= p_zero) candidates.push_back(idx);
		}
		else if (graph_mode == HEATMAP)
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
	widget_settings.insert("graph_mode", settings_->getString("graph_mode"));
	widget_settings.insert("track_height", settings_->getInt("track_height"));
	widget_settings.insert("view_min", settings_->getFloat("view_min"));
	widget_settings.insert("view_max", settings_->getFloat("view_max"));
	return widget_settings;
}

void IgvTrack::loadKeyValueFromXml(QString key, QString value)
{
	bool ok = false;
	if (key == "graph_mode")
	{
		QByteArray mode = value.toUtf8().toUpper();
		//Accept the numeric graph modes stored by older sessions.
		int legacy_mode = value.toInt(&ok);
		if (ok && legacy_mode >= 0 && legacy_mode < GRAPH_MODES.count()) mode = GRAPH_MODES[legacy_mode];
		if (GRAPH_MODES.contains(mode)) settings_->setString("graph_mode", mode);
	}
	else if (key == "track_height")
	{
		int height = value.toInt(&ok);
		if (ok && height > 0) settings_->setInt("track_height", height);
	}
	else if (key == "view_min" || key == "view_max")
	{
		float limit = value.toFloat(&ok);
		if (ok && std::isfinite(limit) && limit >= 0 && (key == "view_min" || limit <= 1)) settings_->setFloat(key.toUtf8(), limit);
	}
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
