#include "IgvTrack.h"
#include "Exceptions.h"
#include "FileLoader.h"
#include "SharedData.h"
#include <QMessageBox>
#include <QApplication>
#include <QPainter>
#include <cmath>

QList<Parameter> IgvTrack::getParameters() const
{
	QList<Parameter> config;
	config.append(Parameter("draw mode", "Type of graph.", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, QByteArray("heatmap\tbar chart\tpoints\tline plot")}}));
	config.append(Parameter("min", "Y axis minimum of plot.", ParameterType::DOUBLE, {{ConstraintType::DISPLAY_DIGITS, 2}}));
	config.append(Parameter("max", "Y axis maximum of plot.", ParameterType::DOUBLE, {{ConstraintType::DISPLAY_DIGITS, 2}}));
	config.append(Parameter("track_height", "Track height in pixels.", ParameterType::INT, {{ConstraintType::MIN, 1}}));
	return config;
}

QHash<QByteArray, QVariant> IgvTrack::getParameterDefaults() const
{
	QHash<QByteArray, QVariant> defaults{{"draw mode", QByteArray("points")}, {"track_height", 100}, {"min", 0.0}, {"max", 1.0}};
	if (igv_file_)
	{
		for (const QByteArray& header : igv_file_->headers())
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
					QByteArray mode = value.toLower().replace('_', ' ');
					if (mode == "heatmap" || mode == "bar chart" || mode == "points" || mode == "line plot") defaults["draw mode"] = mode;
				}
				else if (key == "viewlimits")
				{
					QByteArrayList limits = value.split(':');
					if (limits.size() == 2)
					{
						bool ok_min = false, ok_max = false;
						double minimum = limits[0].toDouble(&ok_min);
						double maximum = limits[1].toDouble(&ok_max);
						if (ok_min && ok_max)
						{
							defaults["min"] = minimum;
							defaults["max"] = maximum;
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
	if (defaults.value("min").toDouble() >= defaults.value("max").toDouble())
	{
		THROW(ArgumentException, "View min >= view max in IGV track.");
	}
	return defaults;
}

IgvTrack::IgvTrack(QWidget* parent, QString file_path, QString name)
	: IgvTrack(parent, file_path, name, QSharedPointer<IgvFile>())
{
}

IgvTrack::IgvTrack(QWidget* parent, QString file_path, QString name, QSharedPointer<IgvFile> igv_file)
	: TrackWidget(parent, file_path, name, type())
	, igv_file_(igv_file)
{
	connect(&settings_, &ParameterList::parameterChanged, this, [this]()
	{
		updateGeometry();
		update();
	});
	initializeSettings();
	updateGeometry();
	update();
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(regionChanged()));
}

IgvTrack* IgvTrack::createTrack(QWidget* parent, QString file_path, QString name)
{
	QSharedPointer<IgvFile> igv_file = FileLoader::loadIgvFile(file_path);
	if (!igv_file) return nullptr;

	QString display_name = name;
	if (display_name.isEmpty()) display_name = getTrackNameFromIgvFile(igv_file);
	if (display_name.isEmpty()) display_name = getDisplayNameFromFilePath(file_path);

	try
	{
		IgvTrack* track = new IgvTrack(parent, file_path, display_name, igv_file);
		track->setIgvFile(igv_file);
		return track;
	}
	catch (const ArgumentException& e)
	{
		QMessageBox::warning(QApplication::activeWindow(), "Error", e.message());
		return nullptr;
	}
}

void IgvTrack::reloadTrack()
{
	QSharedPointer<IgvFile> igv_file = FileLoader::loadIgvFile(file_path_);
	if (igv_file) setIgvFile(igv_file);
	else emit trackDeleted();
}

QSize IgvTrack::sizeHint() const
{
	return QSize( parentWidget() ? parentWidget()->width() : 200, settings_.getInt("track_height"));
}

void IgvTrack::setIgvFile(QSharedPointer<IgvFile> igv_file)
{
	if (igv_file)
	{
		auto index = std::make_unique<ChromosomalIndex<IgvFile>>(*igv_file);
		igv_file_ = igv_file;
		chr_index_ = std::move(index);
		update();
	}
}

void IgvTrack::paintEvent(QPaintEvent*)
{
	QPainter painter(this);

	//fill background
	painter.fillRect(rect(), Qt::white);

	//draw label
	drawLabel(painter);

	//draw chart
	if (igv_file_) drawPlot(painter);

	if (settings_.getString("draw mode") != "heatmap") drawScaleText(painter);
}

void IgvTrack::drawScaleText(QPainter& painter)
{
	const double view_min = settings_.getDouble("min");
	const double view_max = settings_.getDouble("max");
	Viewport viewport = getViewport();
	painter.setPen(Qt::black);
	QRect rec(viewport.x0, 0, width(), height());
	painter.drawText(rec, Qt::AlignLeft, "["+QString::number(view_min) + "," + QString::number(view_max) + "]");
}

IgvTrack::PlotScale IgvTrack::plotScale() const
{
	return {settings_.getDouble("min"), settings_.getDouble("max"), settings_.getInt("track_height")};
}

void IgvTrack::drawPlot(QPainter& painter)
{
	const BedLine& region = SharedData::region();
	const PlotScale scale = plotScale();

	//check min is less than max
	if (scale.minimum >= scale.maximum) return;
	const Viewport viewport = getViewport();
	if (viewport.usable_width <= 0) return;
	painter.save();
	painter.setClipRect(QRectF(viewport.x0, 0, viewport.usable_width, height()), Qt::IntersectClip);

	//draw reference lines
	painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
	drawReferenceLine(painter, scale.minimum, scale);
	drawReferenceLine(painter, .5 * (scale.minimum + scale.maximum), scale);
	drawReferenceLine(painter, scale.maximum, scale);

	//get matching indices
	int padding = region.length() / 3;
	int start = std::max(0, region.start() - padding);
	int end = region.end() + padding;
	QVector<int> idxes = chr_index_->matchingIndices(region.chr(), start, end);

	//draw chart
	QByteArray draw_mode = settings_.getString("draw mode");
	if (draw_mode == "heatmap") drawHeatMap(painter, idxes, scale);
	else if (draw_mode == "bar chart") drawBarChart(painter, idxes, scale);
	else if (draw_mode == "points") drawPoints(painter, idxes, scale);
	else if (draw_mode == "line plot") drawLinePlot(painter, idxes, scale);
	painter.restore();
}

void IgvTrack::drawPoints(QPainter& painter, const QVector<int>& idxes, const PlotScale& scale)
{
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(Qt::NoPen);
	painter.setBrush(Qt::blue);
	const BedLine& region = SharedData::region();
	const Viewport& viewport = getViewport();

	foreach (int idx, idxes)
	{
		const IgvFileLine& bd = (*igv_file_)[idx];
		const double value = bd.value();

		int pos = (bd.start() + bd.end()) / 2;

		if (pos < region.start() || pos > region.end()) continue;

		float p1 = viewport.genomePosToScreen(pos);
		float p2 = viewport.genomePosToScreen(pos + 1);

		float px = (p1 + p2) / 2.f;
		int py = valueToY(value, scale);
		painter.drawEllipse(QPointF(px, py), 1.5, 1.5);
	}
}

void IgvTrack::drawLinePlot(QPainter& painter, const QVector<int>& idxes, const PlotScale& scale)
{
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(QPen(Qt::blue, 2, Qt::SolidLine));
	const Viewport& viewport = getViewport();

	for (int i =1; i < idxes.count(); ++i)
	{
		const IgvFileLine& bd1 = (*igv_file_)[idxes[i-1]];
		const IgvFileLine& bd2 = (*igv_file_)[idxes[i]];

		const double value1 = bd1.value();
		const double value2 = bd2.value();

		int pos = (bd1.start() + bd1.end()) / 2;
		int pos2 = (bd2.start() + bd2.end()) / 2;

		int px1 = viewport.genomePosToScreen(pos);
		int py1 = valueToY(value1, scale);

		int px2 = viewport.genomePosToScreen(pos2);
		int py2 = valueToY(value2, scale);

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

	painter.fillRect(viewport.x0, 0, viewport.usable_width, scale.height, Qt::gray);
	foreach (int idx, idxes)
	{
		const IgvFileLine& bd = (*igv_file_)[idx];
		const double value = bd.value();

		int pos = (bd.start() + bd.end()) / 2;

		if (pos < region.start() || pos > region.end()) continue;

		float px = viewport.genomePosToScreen(pos);
		float endx = viewport.genomePosToScreen(pos + 1);
		float dx = endx - px;

		double t = std::clamp(value, 0.0, 1.0);

		int r = HEATMAP_COL1.red()   + t * (HEATMAP_COL2.red()   - HEATMAP_COL1.red());
		int g = HEATMAP_COL1.green() + t * (HEATMAP_COL2.green() - HEATMAP_COL1.green());
		int b = HEATMAP_COL1.blue()  + t * (HEATMAP_COL2.blue()  - HEATMAP_COL1.blue());
		int a = HEATMAP_COL1.alpha() + t * (HEATMAP_COL2.alpha() - HEATMAP_COL1.alpha());

		QColor color(r, g, b, a);

		painter.setPen(color);
		painter.setBrush(color);
		painter.drawRect(QRectF(px, 0, dx, scale.height));
	}
}

void IgvTrack::drawBarChart(QPainter& painter, const QVector<int>& idxes, const PlotScale& scale)
{
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(Qt::blue);
	painter.setBrush(Qt::blue);

	const BedLine& region = SharedData::region();
	const Viewport& viewport = getViewport();

	foreach (int idx, idxes)
	{
		const IgvFileLine& bd = (*igv_file_)[idx];
		int pos = (bd.start() + bd.end()) / 2;

		if (pos < region.start() || pos > region.end()) continue;
		painter.drawRect(barRect(bd, viewport, scale));
	}
}


void IgvTrack::drawReferenceLine(QPainter& painter, double value, const PlotScale& scale)
{
	const Viewport& viewport = getViewport();
	int y = valueToY(value, scale);
	painter.drawLine(viewport.x0, y, viewport.x0 + viewport.usable_width, y);
}

int IgvTrack::valueToY(double value, const PlotScale& scale)
{
	constexpr int margin = 4;
	if (qFuzzyCompare(scale.maximum, scale.minimum)) return margin;

	int usable_height = scale.height - 2 * margin;
	double normalized_val = (value - scale.minimum) / (scale.maximum - scale.minimum);
	return margin + (int)((1.0 - normalized_val) * usable_height);
}

QRectF IgvTrack::barRect(const IgvFileLine& line, const Viewport& viewport, const PlotScale& scale)
{
	int pos = (line.start() + line.end()) / 2;
	float px = viewport.genomePosToScreen(pos);
	float endx = viewport.genomePosToScreen(pos + 1);
	int py = valueToY(std::clamp(line.value(), scale.minimum, scale.maximum), scale);
	return QRectF(px, py, endx - px, valueToY(scale.minimum, scale) - py).normalized();
}

void IgvTrack::mousePressEvent(QMouseEvent* event)
{
	mouse_press_pos_ = event->pos();
	TrackWidget::mousePressEvent(event);
}

void IgvTrack::mouseReleaseEvent(QMouseEvent* event)
{
	TrackWidget::mouseReleaseEvent(event);
	QPoint pos = event->pos();
	if ((pos - mouse_press_pos_).manhattanLength() >= QApplication::startDragDistance()
		|| event->button() != Qt::LeftButton)
	{
		return;
	}

	handlePopupRequest(pos, event->globalPosition());
}

void IgvTrack::handlePopupRequest(QPoint local_pos, QPointF global_pos)
{
	if (!chr_index_) return;
	const PlotScale scale = plotScale();
	if (scale.minimum >= scale.maximum) return;
	const QByteArray draw_mode = settings_.getString("draw mode");
	const BedLine& region = SharedData::region();
	const Viewport viewport = getViewport();

	if (viewport.usable_width <= 0 || viewport.isOutOfDrawRegion(local_pos.x())) return;

	int click_position = viewport.screenXToGenomePos(local_pos.x());

	double bp_per_pixel = 1.0 / viewport.pixels_per_base;
	int padding = std::max(2, static_cast<int>(std::ceil(bp_per_pixel * 6)) + 1);

	int start = std::max(0, click_position - padding);
	int end = click_position + padding;

	const QVector<int>& idxes = chr_index_->matchingIndices(region.chr(), start, end);
	QVector<int> candidates;

	foreach (int idx, idxes)
	{
		const IgvFileLine& bd = (*igv_file_)[idx];
		int pos = (bd.start() + bd.end()) / 2;
		if (draw_mode != "line plot" && (pos < region.start() || pos > region.end())) continue;
		float px = viewport.genomePosToScreen(pos);
		float endx = viewport.genomePosToScreen(pos + 1);

		if (draw_mode == "points" || draw_mode == "line plot")
		{
			float center_x = draw_mode == "points" ? (px + endx) / 2.f : static_cast<int>(px);
			int py = valueToY(bd.value(), scale);
			if (std::abs(center_x - local_pos.x()) <= 6 && std::abs(py - local_pos.y()) <= 6) candidates.push_back(idx);
		}
		else if (draw_mode == "bar chart")
		{
			// For bar charts, anywhere within the bar height counts
			if (barRect(bd, viewport, scale).adjusted(-0.5, -0.5, 0.5, 0.5).contains(local_pos)) candidates.push_back(idx);
		}
		else if (draw_mode == "heatmap")
		{
			// For heatmaps, the whole vertical span of the track represents the data
			if (local_pos.x() >= px - 0.5 && local_pos.x() <= endx + 0.5) candidates.push_back(idx);
		}
	}

	if (candidates.empty()) return;

	QString info;
	foreach (int idx, candidates)
	{
		const IgvFileLine& bd = (*igv_file_)[idx];
		info += QString("%1: %2 %3, Value: %4\n")
					.arg(bd.chr().str())
					.arg(bd.start())
					.arg(bd.end())
					.arg(bd.value(), 0, 'g', 15);
	}
	showInfoPopup(global_pos, info);
}

QString IgvTrack::getTrackNameFromIgvFile(QSharedPointer<IgvFile> igv_file)
{
	//this function assumes isValidIgvFile has already been called

	if (!igv_file) return "";

	foreach (QByteArray header, igv_file->headers())
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
