#include "SegTrack.h"
#include "FileLoader.h"
#include "SharedData.h"
#include <QApplication>
#include <QPainter>
#include <QRegularExpression>
#include <cmath>

namespace
{
	QHash<QString, QString> trackAttributes(const QSharedPointer<SegFile>& file)
	{
		QHash<QString, QString> attributes;
		if (!file) return attributes;
		const QRegularExpression attribute("(\\w+)=(?:\"([^\"]*)\"|([^\\s]+))");
		for (const QByteArray& header : file->headers())
		{
			if (!header.startsWith("#track") && !header.startsWith("track ")) continue;
			auto matches = attribute.globalMatch(QString::fromUtf8(header));
			while (matches.hasNext())
			{
				const auto match = matches.next();
				attributes.insert(match.captured(1).toLower(), match.captured(2).isNull() ? match.captured(3) : match.captured(2));
			}
		}
		return attributes;
	}

	QColor trackColor(const QByteArray& text)
	{
		const auto rgb = text.split(',');
		if (rgb.size() != 3) return QColor(QString::fromUtf8(text));
		int channels[3];
		for (int i = 0; i < 3; ++i)
		{
			bool ok = false;
			channels[i] = rgb[i].toInt(&ok);
			if (!ok || channels[i] < 0 || channels[i] > 255) return QColor();
		}
		return QColor(channels[0], channels[1], channels[2]);
	}
}

SegTrack::SegTrack(QWidget* parent, QString file_path, QString name)
	: SegTrack(parent, file_path, name, {})
{
}

SegTrack::SegTrack(QWidget* parent, QString file_path, QString name, QSharedPointer<SegFile> file)
	: TrackWidget(parent, file_path, name, type()), file_(file)
{
	initializeSettings(&fast_settings_);
	if (file_) setSegFile(file_);
	connect(&settings_, &ParameterList::parameterChanged, this, &TrackWidget::regionChanged);
	connect(SharedData::instance(), &SharedData::regionChanged, this, &TrackWidget::regionChanged);
}

QList<Parameter> SegTrack::getParameters() const
{
	return {
		Parameter("draw mode", "Type of graph.", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, QByteArray("segments\tClinCNV")}}),
		Parameter("color", "Color above the reference value (name or R,G,B).", ParameterType::STRING, {}),
		Parameter("alt color", "Color below the reference value (name or R,G,B).", ParameterType::STRING, {}),
		Parameter("min", "Y axis minimum.", ParameterType::DOUBLE, {}),
		Parameter("max", "Y axis maximum.", ParameterType::DOUBLE, {}),
		Parameter("reference", "Reference line and color threshold.", ParameterType::DOUBLE, {}),
		Parameter("show reference", "Draw the reference line.", ParameterType::BOOL, {}),
		Parameter("track height", "Track height in pixels.", ParameterType::INT, {{ConstraintType::MIN, 10}})
	};
}

QHash<QByteArray, QVariant> SegTrack::getParameterDefaults() const
{
	QHash<QByteArray, QVariant> defaults{{"draw mode", QByteArray("ClinCNV")}, {"color", QByteArray("0,0,255")},
		{"alt color", QByteArray("255,0,0")}, {"min", 0.0}, {"max", 6.0}, {"reference", 2.0}, {"show reference", true}, {"track height", 80}};
	const auto attributes = trackAttributes(file_);
	for (auto it = attributes.cbegin(); it != attributes.cend(); ++it)
	{
		const QString& key = it.key();
		const QString& value = it.value();
		if (key == "color" || key == "altcolor")
		{
			if (trackColor(value.toUtf8()).isValid()) defaults[key == "color" ? "color" : "alt color"] = value.toUtf8();
		}
		else if (key == "viewlimits")
		{
			const auto limits = value.split(':');
			if (limits.size() != 2 && limits.size() != 3) continue;
			bool ok_min = false, ok_max = false;
			const double minimum = limits.first().toDouble(&ok_min);
			const double maximum = limits.last().toDouble(&ok_max);
			if (ok_min && ok_max && std::isfinite(minimum) && std::isfinite(maximum) && minimum < maximum)
			{
				defaults["min"] = minimum;
				defaults["max"] = maximum;
			}
		}
		else if (key == "ylinemark")
		{
			bool ok = false;
			const double reference = value.toDouble(&ok);
			if (ok && std::isfinite(reference)) defaults["reference"] = reference;
		}
		else if (key == "ylineonoff") defaults["show reference"] = (value.compare("on", Qt::CaseInsensitive) == 0);
		else if (key == "maxheightpixels")
		{
			const auto heights = value.split(':');
			bool ok = false;
			const int height = heights[heights.size() > 1 ? 1 : 0].toInt(&ok);
			if (ok && height >= 10) defaults["track height"] = height;
		}
	}
	return defaults;
}

SegTrack* SegTrack::createTrack(QWidget* parent, QString file_path, QString name)
{
	const auto file = FileLoader::loadSegFile(file_path);
	if (!file) return nullptr;
	if (name.isEmpty()) name = trackAttributes(file).value("name");
	if (name.isEmpty()) name = getDisplayNameFromFilePath(file_path);
	return new SegTrack(parent, file_path, name, file);
}

void SegTrack::setSegFile(QSharedPointer<SegFile> file)
{
	if (!file) THROW(ArgumentException, "Cannot set a null SEG file!");
	auto index = std::make_unique<ChromosomalIndex<SegFile>>(*file);
	chr_index_ = std::move(index);
	file_ = file;
	update();
}

void SegTrack::reloadTrack()
{
	const auto file = FileLoader::loadSegFile(file_path_);
	if (file) setSegFile(file);
	else emit trackDeleted();
}

QSize SegTrack::sizeHint() const
{
	return QSize(parentWidget() ? parentWidget()->width() : 200, fast_settings_.track_height);
}

double SegTrack::valueToY(double value) const
{
	return 4 + (1 - (value - fast_settings_.min) / (fast_settings_.max - fast_settings_.min)) * std::max(1, height() - 8);
}

double SegTrack::displayedValue(double value) const
{
	if (fast_settings_.draw_mode == "ClinCNV" && value < 0) value = 0;
	return std::clamp(value, fast_settings_.min, fast_settings_.max);
}

QColor SegTrack::segmentColor(double value) const
{
	const bool clincnv = fast_settings_.draw_mode == "ClinCNV";
	if (clincnv && value < 0) return Qt::black;
	const bool below = value < (clincnv ? 2.0 : fast_settings_.reference);
	QColor color = trackColor(below ? fast_settings_.alt_color : fast_settings_.color);
	if (!color.isValid()) color = Qt::blue;
	return color;
}

void SegTrack::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.fillRect(rect(), Qt::white);
	drawLabel(painter);
	const auto viewport = getViewport();
	if (!chr_index_ || viewport.usable_width <= 0 || !std::isfinite(fast_settings_.min)
		|| !std::isfinite(fast_settings_.max) || fast_settings_.min >= fast_settings_.max) return;
	for (const auto& plot : plotViewports()) drawPlot(painter, plot.first, plot.second);
	painter.setClipRect(QRect(viewport.x0, 0, viewport.usable_width, height()));
	painter.setPen(Qt::black);
	painter.drawText(QRect(viewport.x0, 0, viewport.usable_width, height()), Qt::AlignLeft | Qt::AlignTop,
		QString("[%1,%2]").arg(fast_settings_.min).arg(fast_settings_.max));
}

void SegTrack::drawPlot(QPainter& painter, const BedLine& region, const Viewport& viewport)
{
	painter.save();
	painter.setClipRect(QRect(viewport.x0, 0, viewport.usable_width, height()));
	painter.setRenderHint(QPainter::Antialiasing, false);
	for (int idx : chr_index_->matchingIndices(region.chr(), region.start(), region.end()))
	{
		const auto& line = (*file_)[idx];
		const double value = displayedValue(line.value());
		QColor color = segmentColor(line.value());
		if (fast_settings_.draw_mode == "ClinCNV") color.setAlphaF(color.alphaF() * file_->varianceAlpha(line.variance()));
		painter.setPen(QPen(color, 2));
		const double x = viewport.genomePosToScreen(std::max(line.start(), region.start()));
		const double end = viewport.genomePosToScreen(std::min(line.end(), region.end())) + viewport.pixels_per_base;
		const int y = qRound(valueToY(value));
		painter.drawLine(QPoint(qRound(x), y), QPoint(qRound(std::max(x + 1, end)), y));
	}
	if (fast_settings_.show_reference
		&& fast_settings_.reference >= fast_settings_.min && fast_settings_.reference <= fast_settings_.max)
	{
		painter.setRenderHint(QPainter::Antialiasing, false);
		painter.setPen(QPen(QColor(128, 128, 128, 128), 1));
		const int y = qRound(valueToY(fast_settings_.reference));
		painter.drawLine(viewport.x0, y, viewport.x0 + viewport.usable_width, y);
	}
	painter.restore();
}

void SegTrack::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) mouse_press_pos_ = event->pos();
	TrackWidget::mousePressEvent(event);
}

void SegTrack::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && (event->pos() - mouse_press_pos_).manhattanLength() < QApplication::startDragDistance())
	{
		handlePopupRequest(event->pos(), event->globalPosition());
	}
	TrackWidget::mouseReleaseEvent(event);
}

void SegTrack::handlePopupRequest(QPoint local_pos, QPointF global_pos)
{
	BedLine region;
	Viewport viewport = getViewport();
	for (const auto& plot : plotViewports())
	{
		if (local_pos.x() < plot.second.x0 || local_pos.x() >= plot.second.x0 + plot.second.usable_width) continue;
		region = plot.first;
		viewport = plot.second;
		break;
	}
	if (!region.isValid()) return;
	if (!chr_index_ || viewport.usable_width <= 0 || region.length() <= 0 || viewport.isOutOfDrawRegion(local_pos.x())
		|| !std::isfinite(fast_settings_.min) || !std::isfinite(fast_settings_.max) || fast_settings_.min >= fast_settings_.max) return;
	QStringList info;
	const int start = viewport.screenXToGenomePos(std::max(viewport.x0, local_pos.x() - 6));
	const int end = viewport.screenXToGenomePos(std::min(viewport.x0 + viewport.usable_width, local_pos.x() + 6));
	for (int idx : chr_index_->matchingIndices(region.chr(), start, end))
	{
		const auto& line = (*file_)[idx];
		const double value = displayedValue(line.value());
		if (std::abs(valueToY(value) - local_pos.y()) > 6) continue;
		if (local_pos.x() < viewport.genomePosToScreen(line.start()) - 6
			|| local_pos.x() > viewport.genomePosToScreen(line.end()) + viewport.pixels_per_base + 6) continue;
		info.append(QString("Region: %1:%2-%3\n%4: %5\nValue: %6")
			.arg(line.chr().str()).arg(line.start()).arg(line.end())
			.arg(QString::fromUtf8(file_->annotationName()), QString::fromUtf8(line.annotation())).arg(line.value(), 0, 'g', 15));
	}
	if (!info.isEmpty()) showInfoPopup(global_pos, info.join("\n\n"));
}
