#include "TrackWidget.h"
#include "SharedData.h"
#include "TrackManager.h"
#include "BafTrack.h"
#include "BedTrack.h"
#include "BamAlignmentTrack.h"
#include "BamCoverageTrack.h"
#include "GenomeVisualizationWidget.h"
#include "QInputDialog"

#include <QApplication>
#include <QDrag>
#include <QDialog>
#include <QLabel>
#include <QMenu>
#include <QMetaEnum>
#include <QMimeData>
#include <QPainter>
#include <QVBoxLayout>

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
	opts_[0] = menu.addAction("Reload Track");

	menu.addSeparator();

	opts_[1] = menu.addAction("Remove Track");

	opts_[2] = menu.addAction("Rename Track..");
}

void TrackWidget::handleContextMenuAction(QAction* action)
{
	if (action == opts_[0])
	{
		reloadTrack();
	}
	else if (action == opts_[1])
	{
		emit trackDeleted();
	}
	else if (action == opts_[2]) // rename track
	{
		handleTrackRename();
	}
}

void TrackWidget::handleTrackRename()
{
	bool ok;

	QString new_name = QInputDialog::getText(this, tr("Enter Track Name"), "", QLineEdit::Normal, name_, &ok);
	if (ok && !new_name.isEmpty()) {
		// Process the text
		qDebug() << "NAME: " << new_name << Qt::endl;
		name_ = new_name;
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

	//draw the drag block
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

void TrackWidget::drawLabel(QPainter& painter)
{

	int label_width = SharedData::settings().label_width;

	// draw text
	QRectF text_rect(0, 0, label_width-2, height());

	painter.setPen(Qt::black);
	painter.drawText(text_rect, Qt::AlignLeft, name_);

}

float Viewport::genomePosToScreen(int genome_pos) const
{
	float scale = (float)total_width / region.length();
	return ((float)(genome_pos - region.start())) * scale + x0;
}

float Viewport::genomeWidthToScreen(int genome_width) const
{
	return ((float)genome_width/region.length()) * total_width;
}

bool Viewport::isOutOfDrawRegion(int x) const
{
	return (x < x0 || x > x0 + total_width);
}

int Viewport::screenXToGenomePos(int x_pos) const
{
	float p = (float)(x_pos - x0) / total_width;
	return region.start() + static_cast<int>(p * region.length());
}

Viewport TrackWidget::getViewport()
{
	int w = width();
	int label_width = SharedData::settings().label_width;
	int total_width = w - label_width - 4;
	const BedLine& region = SharedData::region();
	float pixels_per_base = (float)total_width / region.length();

	return {region, total_width, label_width + 2, pixels_per_base};
}

void TrackWidget::showInfoPopup(QPointF global_pos, QString info)
{
	QDialog* popup = new QDialog(this, Qt::Popup);

	popup->setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout* layout = new QVBoxLayout(popup);

	QLabel* label = new QLabel(info);
	label->setTextInteractionFlags(Qt::TextSelectableByMouse);
	label->setWordWrap(false);

	layout->addWidget(label);

	popup->move(global_pos.x(), global_pos.y());

	popup->show();
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
	if (type == BedTrack::staticType()) return BedTrack::createTrack(parent, file_path, display_name);
	if (type == BamAlignmentTrack::staticType()) return BamAlignmentTrack::createTrack(parent, file_path, display_name);
	if (type == BamCoverageTrack::staticType()) return BamCoverageTrack::createTrack(parent, file_path, display_name);
	if (type == BafTrack::staticType()) return BafTrack::createTrack(parent, file_path, display_name);

	GenomeVisualizationWidget::displayError("Track type: " + type + " not supported.");
	return nullptr;
}

void TrackWidget::loadSettingsFromXml(const QDomNodeList& settings)
{
	for (int i =0; i < settings.count(); ++i)
	{
		QDomElement item =  settings.at(i).toElement();
		QString key = item.attribute("key");
		loadKeyValueFromXml(key, item);
	}
}
