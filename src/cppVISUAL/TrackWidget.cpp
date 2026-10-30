#include "TrackWidget.h"
#include "SharedData.h"
#include "IgvTrack.h"
#include "SegTrack.h"
#include "BedTrack.h"
#include "VcfTrack.h"
#include "SvTrack.h"
#include "BamAlignmentTrack.h"
#include "BamCoverageTrack.h"
#include <QInputDialog>
#include <QApplication>
#include <QFileInfo>
#include <QHelpEvent>
#include <QDrag>
#include <QDialog>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QToolTip>
#include <QVBoxLayout>
#include <limits>

TrackWidget::TrackWidget(QWidget* parent, QString file_path, QString display_name, QString type)
	: QWidget(parent)
	, viewer_(GenomeVisualizationWidget::fromWidget(parent))
	, file_path_(file_path)
	, settings_(type.toUtf8(), {}, {})
	, display_name_(display_name)
	, type_(type)
{
	connect(SharedData::instance(), &SharedData::settingsChanged, this, &TrackWidget::regionChanged);
}

void TrackWidget::initializeSettings(QObject* fast_access_struct)
{
	settings_.reset(getParameters(), getParameterDefaults());
	settings_.link(fast_access_struct);
}

QSize TrackWidget::minimumSizeHint() const
{
	//Allow horizontal shrinking while keeping the label area and track height.
	return QSize(SharedData::settings().label_width + 5, sizeHint().height());
}

void TrackWidget::regionChanged()
{
	updateGeometry();
	update();
}

void TrackWidget::populateContextMenu(QMenu& context_menu, const QPoint&)
{
	QMenu& menu = *context_menu.addMenu("Track");
	QAction* settings = menu.addAction("Open settings");
	connect(settings, &QAction::triggered, this, [this]() { emit settingsRequested(this); });
	menu.addSeparator();
	QAction* reload = menu.addAction("Reload Track");
	connect(reload, &QAction::triggered, this, &TrackWidget::reloadTrack);
	menu.addSeparator();

	QAction* remove = menu.addAction("Remove Track");
	connect(remove, &QAction::triggered, this, &TrackWidget::trackDeleted);

	QAction* rename = menu.addAction("Rename Track...");
	connect(rename, &QAction::triggered,this, &TrackWidget::handleTrackRename);
}

void TrackWidget::handleTrackRename()
{
	bool ok;
	QString new_name = QInputDialog::getText(this, "Enter Track Name", "", QLineEdit::Normal, display_name_, &ok);

	if (ok && !new_name.isEmpty() && new_name != display_name_)
	{
		display_name_ = new_name;
		update();
	}
}


bool TrackWidget::event(QEvent* event)
{
	if (event->type() == QEvent::ToolTip)
	{
		auto* help_event = static_cast<QHelpEvent*>(event);
		const QRect header_rect(0, 0, SharedData::settings().label_width, height());
		if (header_rect.contains(help_event->pos()))
		{
			QToolTip::showText(help_event->globalPos(), display_name_, this, header_rect);
		}
		else
		{
			QToolTip::hideText();
			event->ignore();
		}
		return true;
	}
	return QWidget::event(event);
}

void TrackWidget::mousePressEvent(QMouseEvent* event)
{
	emit trackSelected(this);
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
	}
	if (!(event->buttons() & Qt::LeftButton))
	{
		is_dragging_ = false;
		event->ignore();
		return;
	}
	if ((event->pos() - drag_start_pos_).manhattanLength() < QApplication::startDragDistance()) return;

	QDrag* drag = new QDrag(this);
	QMimeData* mime_data = new QMimeData;
	mime_data->setData("application/track-name", display_name_.toUtf8());
	drag->setMimeData(mime_data);

	//draw the drag block
	int width = SharedData::settings().label_width;
	QPixmap pixmap(width, height());
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	QRect rect(0, 0, width, height());
	painter.setPen(QPen(QColor(0, 0, 0), 2));
	painter.setBrush(Qt::NoBrush);

	painter.drawRect(rect);
	painter.end();

	drag->setPixmap(pixmap);
	drag->setHotSpot(event->pos());

	//Reset before exec(), which runs an event loop and can move/delete the track.
	is_dragging_ = false;
	drag->exec(Qt::MoveAction);
}

void TrackWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) is_dragging_ = false;
	QWidget::mouseReleaseEvent(event);
}

void TrackWidget::drawLabel(QPainter& painter)
{

	int label_width = SharedData::settings().label_width;
	painter.save();
	painter.setPen(Qt::black);
	painter.drawLine(QPoint(label_width, 0), QPoint(label_width, height()));

	// draw text
	QRectF text_rect(0, 0, label_width-2, height());

	painter.drawText(text_rect, Qt::AlignLeft, display_name_);
	painter.restore();
}

bool TrackWidget::drawOverviewMessage(QPainter& painter)
{
	if (viewer_->region().isValid()) return false;
	const Viewport viewport = getViewport();
	painter.setPen(Qt::gray);
	painter.drawText(QRect(viewport.x0, 0, std::max(0, viewport.usable_width), height()), Qt::AlignCenter,
		"select single chromosome region to show content");
	return true;
}

float Viewport::genomePosToScreen(int genome_pos) const
{
	return (genome_pos - genome_start) * pixels_per_base + x0;
}

float Viewport::genomeWidthToScreen(int genome_width) const
{
	return genome_width * pixels_per_base;
}

bool Viewport::isOutOfDrawRegion(int x) const
{
	return (x < x0 || x > x0 + usable_width);
}

int Viewport::screenXToGenomePos(int x_pos) const
{
	return genome_start + static_cast<int>((x_pos - x0) / pixels_per_base);
}

Viewport TrackWidget::getViewport() const
{
	int label_width = SharedData::settings().label_width;
	int usable_width = width() - label_width - 4;
	float pixels_per_base = viewer_->region().isValid() ? (float)usable_width / viewer_->region().length() : 0.0f;

	return {usable_width, label_width + 2, pixels_per_base, viewer_->region().start()};
}

QVector<QPair<BedLine, Viewport>> TrackWidget::plotViewports() const
{
	const Viewport viewport = getViewport();
	if (viewer_->region().isValid()) return {{viewer_->region(), viewport}};
	QVector<QPair<BedLine, Viewport>> plots;
	if (viewport.usable_width <= 0) return plots;
	const BedFile& regions = SharedData::wholeGenomeRegions();
	qint64 total_length = 0;
	for (int i = 0; i < regions.count(); ++i) total_length += regions[i].length();
	if (total_length <= 0) return plots;
	qint64 offset = 0;
	for (int i = 0; i < regions.count(); ++i)
	{
		const int left = viewport.x0 + qRound(double(offset) / total_length * viewport.usable_width);
		offset += regions[i].length();
		const int right = viewport.x0 + qRound(double(offset) / total_length * viewport.usable_width);
		if (right <= left) continue;
		plots.append({regions[i], {right - left, left, float(right - left) / regions[i].length(), regions[i].start()}});
	}
	return plots;
}

void TrackWidget::showInfoPopup(QPointF global_pos, QString info)
{
	QDialog* popup = new QDialog(this, Qt::Popup);
	popup->setAttribute(Qt::WA_DeleteOnClose);

	QLabel* label = new QLabel(info.trimmed());
	label->setTextInteractionFlags(Qt::TextSelectableByMouse);
	label->setWordWrap(false);

	QVBoxLayout* layout = new QVBoxLayout(popup);
	layout->setContentsMargins(QMargins(4,4,4,4));
	layout->addWidget(label);

	popup->move(global_pos.x(), global_pos.y());
	popup->show();
}

QString TrackWidget::getDisplayNameFromFilePath(QString file_path)
{
	QString output;

	file_path = file_path.trimmed();

	if (Helper::isHttpUrl(file_path))
	{
		QString base_url = file_path.split('?')[0]; //remove arguments like tokens
		if (base_url.contains("/"))
		{
			output = base_url.split("/").last();
		}
		output = base_url;
	}
	else //local file
	{
		output = QFileInfo(file_path).fileName();
	}

	return output.isEmpty() ? file_path : output;
}

void TrackWidget::writeToXml(QXmlStreamWriter& writer)
{
	writer.writeStartElement("Track");
	writer.writeAttribute("type", type_);
	writer.writeAttribute("file_name", file_path_);
	writer.writeAttribute("display_name", display_name_);
	writer.writeAttribute("height", QString::number(height()));
	for(const Parameter& param: settings_.parameters())
	{
		writer.writeStartElement("Settings");
		writer.writeAttribute("name", QString::fromUtf8(param.name()));
		writer.writeAttribute("type", QString::fromUtf8(Parameter::toString(param.type())));
		if (param.type() == ParameterType::STRINGLIST)
		{
			for (const QByteArray& entry : param.value().value<QByteArrayList>())
			{
				writer.writeTextElement("Value", QString::fromUtf8(entry));
			}
		}
		else
		{
			//Preserve double precision when saving and loading a session.
			QByteArray value = param.value().toByteArray();
			if (param.type() == ParameterType::DOUBLE) value = QByteArray::number(param.value().toDouble(), 'g', std::numeric_limits<double>::max_digits10);
			else if (param.type() == ParameterType::BOOL) value = param.value().toBool() ? "yes" : "no";
			writer.writeTextElement("Value", QString::fromUtf8(value));
		}
		writer.writeEndElement(); // Settings
	}
	writer.writeEndElement(); // Track
}

TrackWidget* TrackWidget::fromXml(const QDomElement& track_element, QWidget* parent, QStringList& errors)
{
	QString type = track_element.attribute("type");
	QString file_path = track_element.attribute("file_name");
	QString display_name = track_element.attribute("display_name");

	bool ok = false;
	auto height = track_element.attribute("height").toInt(&ok);

	if (!ok || height <= 0) errors << "Missing or invalid track height for " + file_path;

	auto track = TrackWidget::load(LoadContext {
		type, file_path, display_name, errors, height
	}, parent);

	//load settings
	for (QDomElement item = track_element.firstChildElement("Settings"); !item.isNull(); item = item.nextSiblingElement("Settings"))
	{
		const QByteArray key = item.attribute("name").toUtf8();
		try
		{
			const Parameter& parameter = track->settings_.parameter(key);
			if (item.attribute("type").toUtf8() != Parameter::toString(parameter.type()))
			{
				THROW(ArgumentException, "Parameter type does not match '" + Parameter::toString(parameter.type()) + "'.");
			}
			QByteArrayList values;
			for (QDomElement entry = item.firstChildElement("Value"); !entry.isNull(); entry = entry.nextSiblingElement("Value"))
			{
				values.append(entry.text().toUtf8());
			}
			if (parameter.type() != ParameterType::STRINGLIST && values.size() != 1)
			{
				THROW(ArgumentException, "Expected exactly one Value element.");
			}
			const QString value = values.isEmpty() ? QString() : QString::fromUtf8(values.first());
			bool ok = false;
			switch (parameter.type())
			{
			case ParameterType::INT:
			{
				const int number = value.toInt(&ok);
				if (!ok) THROW(ArgumentException, "Invalid integer '" + value + "'.");
				track->settings_.setInt(key, number);
				break;
			}
			case ParameterType::DOUBLE:
			{
				const double number = value.toDouble(&ok);
				if (!ok) THROW(ArgumentException, "Invalid double '" + value + "'.");
				track->settings_.setDouble(key, number);
				break;
			}
			case ParameterType::BOOL:
			{
				const QString boolean = value.trimmed().toLower();
				if (boolean != "yes" && boolean != "no" && boolean != "true" && boolean != "false" && boolean != "1" && boolean != "0")
				{
					THROW(ArgumentException, "Invalid boolean '" + value + "'.");
				}
				track->settings_.setBool(key, boolean == "yes" || boolean == "true" || boolean == "1");
				break;
			}
			case ParameterType::STRING:
			{
				track->settings_.setString(key, values.first());
				break;
			}
			case ParameterType::STRINGLIST:
			{
				track->settings_.setStringList(key, values);
				break;
			}
			default: THROW(ProgrammingException, "Unhandled parameter type.");
			}
		}
		catch (const Exception& e)
		{
			errors << "Invalid setting '" + QString::fromUtf8(key) + "' for " + file_path + ": " + e.message();
		}
	}

	return track;
}

TrackWidget* TrackWidget::load(const LoadContext cx, QWidget* parent)
{
	auto type = cx.track_type;
	auto file_path = cx.file_path;
	auto display_name = cx.display_name;
	auto errors = cx.errors;

	//create track
	TrackWidget* track = nullptr;
	if (type == BedTrack::type()) track = BedTrack::createTrack(parent, file_path, display_name);
	else if (type == VcfTrack::type()) track = VcfTrack::createTrack(parent, file_path, display_name);
	else if (type == SvTrack::type()) track = SvTrack::createTrack(parent, file_path, display_name);
	else if (type == BamAlignmentTrack::type()) track = BamAlignmentTrack::createTrack(parent, file_path, display_name);
	else if (type == BamCoverageTrack::type()) track = BamCoverageTrack::createTrack(parent, file_path, display_name);
	else if (type == IgvTrack::type()) track = IgvTrack::createTrack(parent, file_path, display_name);
	else if (type == SegTrack::type()) track = SegTrack::createTrack(parent, file_path, display_name);
	else errors << ("Unsupported track type '"+type+"' for " + file_path);
	if (!track) return nullptr;

	track->session_height_ = cx.height;
	//Variant tracks grow with the number of overlapping variants at the current zoom level.
	if (type != VcfTrack::type() && type != SvTrack::type())
	{
		const int restored_height = type == BedTrack::type() ? std::max(cx.height, track->minimumHeight()) : cx.height;			track->setFixedHeight(restored_height);
	}

	return track;
}

LoadContext TrackWidget::loadContext(QStringList& errors) const
{
	return LoadContext {
		type_,
		file_path_,
		display_name_,
		errors,
		session_height_
	};
}
