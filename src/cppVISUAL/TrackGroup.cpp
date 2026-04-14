#include "FileLoader.h"
#include "GenomeVisualizationWidget.h"
#include "TrackGroup.h"

#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QFileInfo>
#include <QFileDialog>


TrackGroup::TrackGroup(QWidget* parent)
	:QScrollArea(parent), layout_(new QVBoxLayout(this)), content_widget_(new QWidget(this))
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setContextMenuPolicy(Qt::CustomContextMenu);

	content_widget_->setLayout(layout_);

	setWidget(content_widget_);

	setWidgetResizable(true);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));

	setAcceptDrops(true);

	layout_->addStretch(1);
	layout_->setContentsMargins(0, 0, 0, 0);
}

void TrackGroup::trackDeleted()
{
	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
	if (senderWidget) {
		layout_->removeWidget(senderWidget);
		senderWidget->deleteLater();
		layout_->update();
	}

	if (layout_->count() == 1) deleteLater();
}

void TrackGroup::trackMoved()
{
	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
	if (senderWidget){
		disconnect(senderWidget, SIGNAL(trackDeleted()), this, SLOT(trackDeleted()));
		disconnect(senderWidget, SIGNAL(trackMoved()), this, SLOT(trackMoved()));
		layout_->removeWidget(senderWidget);
		layout_->update();

		if (layout_->count() == 1) deleteLater();
	}
}

void TrackGroup::addTrackWidgets(TrackWidgetList widgets)
{
	foreach (TrackWidget* widget, widgets)
	{
		connect(widget, SIGNAL(trackDeleted()), this, SLOT(trackDeleted()));
		connect(widget, SIGNAL(trackMoved()), this, SLOT(trackMoved()));
		layout_->insertWidget(layout_->count() - 1, widget);
		layout_->update();
	}
}

void TrackGroup::loadTracksFromFile()
{
	TrackWidgetList widgets = loadTrackWidgetsFromFile();
	addTrackWidgets(widgets);
}

TrackGroup* TrackGroup::fromFile()
{
	TrackWidgetList widgets = loadTrackWidgetsFromFile();
	if (widgets.isEmpty()) return nullptr;

	TrackGroup* tr = new TrackGroup;
	tr->addTrackWidgets(widgets);
	return tr;
}

TrackWidgetList TrackGroup::loadTrackWidgetsFromFile()
{
	QString file_path = GenomeVisualizationWidget::getOpenFileName(tr("Open File"), "", tr("Bed Files(*.bed);;Bam Files(*.bam);;Cram Files(*.cram)"));
	if (file_path.isEmpty()) return TrackWidgetList();

	TrackWidgetList widgets = FileLoader::loadTracks(file_path, nullptr);
	return widgets;
}

void TrackGroup::reloadTracks()
{
	QList<TrackWidget*> track_widgets = findChildren<TrackWidget*>();
	foreach (TrackWidget* track_widget, track_widgets)
	{
		track_widget->reloadTrack();
	}
}

void TrackGroup::contextMenu(QPoint pos)
{
	cur_context_track_ = nullptr;

	QMenu menu(this);

	TrackWidget* track = getTrackUnderMouse(pos);
	if (track)
	{
		QMenu* track_menu = new QMenu("Track", this);
		track->populateContextMenu(*track_menu);
		menu.addMenu(track_menu);
		cur_context_track_ = track;
	}

	QAction* opt1 = menu.addAction("Load file");

	menu.addSeparator();

	QAction* opt2 = menu.addAction("Clear Panel");
	QAction* opt3 = menu.addAction("Remove Panel");

	menu.addSeparator();

	QAction* opt4 = menu.addAction("Add Panel Above");
	QAction* opt5 = menu.addAction("Add Panel Below");

	QAction* action = menu.exec(mapToGlobal(pos));

	if (action == opt1)		 loadTracksFromFile();
	else if (action == opt2) clearLayout();
	else if (action == opt3) clearLayoutAndDelete();
	else if (action == opt4) emit addPanelAbove();
	else if (action == opt5) emit addPanelBelow();
	else if (cur_context_track_) cur_context_track_->handleContextMenuAction(action);

	cur_context_track_ = nullptr;
}

void TrackGroup::clearLayout()
{
	if (layout_)
	{
		while (QLayoutItem* item = layout_->takeAt(0))
		{
			if (QWidget* widget = item->widget()) widget->deleteLater();
			delete item;
		}
	}
}

void TrackGroup::clearLayoutAndDelete()
{
	clearLayout();
	deleteLater();
}


void TrackGroup::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasFormat("application/track-data"))
	{
		event->acceptProposedAction();
	}
}


inline int TrackGroup::getDropIndex(int y)
{
	int drop_index =0;
	for (int i = 0; i < layout_->count() - 1; ++i) {
		QWidget* w = layout_->itemAt(i)->widget();
		if (w && y > w->geometry().center().y()) {
			drop_index = i + 1;
		}
	}
	drop_index = std::max(0, std::min(drop_index, layout_->count() - 1));

	return drop_index;
}

TrackWidget* TrackGroup::getTrackUnderMouse(QPoint pos)
{
	foreach (TrackWidget* track, findChildren<TrackWidget*>())
	{
		if (track->geometry().contains(pos)) return track;
	}
	return nullptr;
}


void TrackGroup::dropEvent(QDropEvent* event)
{
	if (event->mimeData()->hasFormat("application/track-data"))
	{
		QByteArray track_data = event->mimeData()->data("application/track-data");
		QUuid track_id = QUuid(track_data);

		if (TrackManager::hasTrackWidget(track_id))
		{
			TrackWidget* source = TrackManager::getTrackWidget(track_id);
			if (!source) return;

			QWidget* old_panel = source->parentWidget();
			if (!old_panel)
			{
				qDebug() << "Parent widget was not found for source on drop!" << Qt::endl;
				return;
			}

			if (old_panel && old_panel != content_widget_)
			{
				emit source->trackMoved();

				connect(source, SIGNAL(trackDeleted()), this, SLOT(trackDeleted()));
				connect(source, SIGNAL(trackMoved()), this, SLOT(trackMoved()));
			}
			else
			{
				layout_->removeWidget(source);
			}

			int drop_index = getDropIndex(event->position().y());

			// this changes the parent of the source
			layout_->insertWidget(drop_index, source);
			layout_->update();
		}
	}
	event->acceptProposedAction();
}

void TrackGroup::writeToXml(QXmlStreamWriter& writer)
{
	writer.writeStartElement("TrackGroup");
	QList<TrackWidget*> tracks = findChildren<TrackWidget*>();

	foreach (TrackWidget* track, tracks)
	{
		track->writeToXml(writer);
	}
	writer.writeEndElement(); // TrackGroup
}

void TrackGroup::loadFromXml(const QDomElement& dom_element)
{
	QDomNodeList elements = dom_element.elementsByTagName("Track");
	for (int i =0; i < elements.count(); ++i)
	{
		const QDomElement& track_element = elements.at(i).toElement();

		QString type = track_element.attribute("type");
		TrackWidget* track = TrackWidget::fromXml(track_element, this);
		if (track)
		{
			connect(track, SIGNAL(trackDeleted()), this, SLOT(trackDeleted()));
			connect(track, SIGNAL(trackMoved()), this, SLOT(trackMoved()));
			layout_->insertWidget(layout_->count() - 1, track);
			layout_->update();
		}
	}
}


