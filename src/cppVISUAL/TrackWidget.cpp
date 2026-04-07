#include "FileLoader.h"
#include "TrackWidget.h"
#include "SharedData.h"
#include "TrackManager.h"
#include "BedTrack.h"
#include "GenomeVisualizationWidget.h"

#include <QApplication>
#include <QDrag>
#include <QMenu>
#include <QMetaEnum>
#include <QMimeData>
#include <QPainter>

TrackWidget::TrackWidget(QWidget* parent, QString file_path, QString name)
	:QWidget(parent), id_(QUuid::createUuid()), file_path_(file_path), name_(name)
{
	TrackManager::addTrackWidget(id_, this);
}

TrackWidget::~TrackWidget()
{
	TrackManager::removeTrackWidget(id_);
}

void TrackWidget::regionChanged()
{
	updateGeometry();
	update();
}

void TrackWidget::populateContextMenu(QMenu& menu)
{
	opts_[0] = menu.addAction("Collapsed");
	opts_[1] = menu.addAction("Expanded");

	for (QAction *action : {opts_[0], opts_[1]})
	{
		action->setCheckable(true);
	}

	switch (draw_mode_)
	{
	case COLLAPSED:
		opts_[0]->setChecked(true);
		break;
	case EXPANDED:
		opts_[1]->setChecked(true);
		break;
	}

	opts_[2] = menu.addAction("Reload Track");

	menu.addSeparator();
	opts_[3] = menu.addAction("Remove Track");
}

void TrackWidget::handleContextMenuAction(QAction* action)
{
	if (action == opts_[0])
	{
		draw_mode_ = COLLAPSED;
		updateGeometry();
		update();
	}
	else if (action == opts_[1])
	{
		draw_mode_ = EXPANDED;
		updateGeometry();
		update();
	}
	else if (action == opts_[2])
	{
		reloadTrack();
	}
	else if (action == opts_[3])
	{
		emit trackDeleted();
	}
}


void TrackWidget::mousePressEvent(QMouseEvent* event)
{
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
	};
	if (!(event->buttons() & Qt::LeftButton)) return;
	if ((event->pos() - drag_start_pos_).manhattanLength() < QApplication::startDragDistance()) return;

	QDrag* drag = new QDrag(this);
	QMimeData* mime_data = new QMimeData;
	mime_data->setData("application/track-data", id_.toByteArray());
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

void TrackWidget::writeToXml(QXmlStreamWriter& writer)
{
	writer.writeStartElement("Track");
	writer.writeAttribute("type", getType());
	writer.writeAttribute("file_name", file_path_);
	writer.writeAttribute("display_name", name_);
	auto settings = getSettings();
	for (auto it = settings.begin(); it != settings.end(); ++it)
	{
		writer.writeStartElement("Settings");
		writer.writeAttribute("key", it.key());
		writer.writeAttribute("value", it.value().toString());
		writer.writeEndElement(); // Settings
	}
	writer.writeEndElement(); // Track
}

TrackWidget* TrackWidget::fromXml(const QDomElement& track_element, QWidget* parent)
{
	QString type = track_element.attribute("type");
	QString file_path = track_element.attribute("file_name");
	QString display_name = track_element.attribute("display_name");

	QDomNodeList settings = track_element.elementsByTagName("Settings");

	TrackWidget* track = TrackWidget::fromType(type, parent, file_path, display_name);
	if (track) track->loadSettingsFromXml(settings);

	return track;
}

TrackWidget* TrackWidget::fromType(QString type, QWidget* parent, QString file_path, QString display_name)
{
	if (type == "BED")
	{
		return BedTrack::createTrack(parent, file_path, display_name);
	}
	GenomeVisualizationWidget::displayError("Track type: " + type + " not supported.");
	return nullptr;
}

QMap<QString, QVariant> TrackWidget::getSettings()
{
	QMap<QString, QVariant> settings;
	settings["draw_mode"] = static_cast<int>(draw_mode_);
	return settings;
}

void TrackWidget::loadSettingsFromXml(const QDomNodeList& settings)
{
	for (int i =0; i < settings.count(); ++i)
	{
		QDomElement item =  settings.at(i).toElement();
		QString key = item.attribute("key");
		if (key == "draw_mode")
		{
			int value = item.attribute("value").toInt();
			if (value != -1) draw_mode_ = static_cast<DrawMode>(value);
		}
	}
}
