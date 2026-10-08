#include "FileLoader.h"
#include "GenomeVisualizationWidget.h"
#include "TrackGroup.h"

#include <QApplication>
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
	TrackWidget* senderWidget = qobject_cast<TrackWidget*>(sender());
	if (senderWidget) {
		layout_->removeWidget(senderWidget);
		senderWidget->deleteLater();
		layout_->update();
	}

	if (layout_->count() == 1) deleteLater();
}

void TrackGroup::trackMoved()
{
	TrackWidget* senderWidget = qobject_cast<TrackWidget*>(sender());
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
	QString file_path = GenomeVisualizationWidget::getOpenFileName(tr("Open File"), "", tr("Bed Files(*.bed);;Bam Files(*.bam);;Cram Files(*.cram);;BAF Files(*.igv)"));
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
	QMenu menu(this);

	TrackWidget* track = getTrackUnderMouse(pos);
	if (track)
	{
		QMenu* track_menu = new QMenu("Track", this);
		track->populateContextMenu(*track_menu, track->mapFrom(this, pos));
		menu.addMenu(track_menu);
	}

	QAction* load_file = menu.addAction("Load file");

	menu.addSeparator();

	QAction* clear_panel = menu.addAction("Clear Panel");
	QAction* remove_panel = menu.addAction("Remove Panel");

	menu.addSeparator();

	QAction* add_panel_above = menu.addAction("Add Panel Above");
	QAction* add_panel_below = menu.addAction("Add Panel Below");

	connect(load_file, &QAction::triggered, this, &TrackGroup::loadTracksFromFile);
	connect(clear_panel, &QAction::triggered, this, &TrackGroup::clearLayout);
	connect(remove_panel, &QAction::triggered, this, &TrackGroup::clearLayoutAndDelete);
	connect(add_panel_above, &QAction::triggered, this, &TrackGroup::addPanelAbove);
	connect(add_panel_below, &QAction::triggered, this, &TrackGroup::addPanelBelow);

	menu.exec(mapToGlobal(pos));
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
	if (event->mimeData()->hasFormat("application/track-name"))
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
	TrackWidget* track = qobject_cast<TrackWidget*>(event->source());
	if (!track) return;

	QWidget* old_panel = track->parentWidget();
	if (!old_panel)
	{
		qDebug() << "Parent widget was not found for source on drop!" << Qt::endl;
		return;
	}

	if (old_panel != content_widget_) // came from a different Track
	{
		emit track->trackMoved(); //disconnects the old signals to the old TrackGroup

		connect(track, SIGNAL(trackDeleted()), this, SLOT(trackDeleted()));
		connect(track, SIGNAL(trackMoved()), this, SLOT(trackMoved()));
	}
	else // dropped in the same track
	{
		layout_->removeWidget(track);
	}

	int drop_index = getDropIndex(event->position().y());

	// this changes the parent of the source track
	layout_->insertWidget(drop_index, track);
	layout_->update();

	event->acceptProposedAction();
}

void TrackGroup::wheelEvent(QWheelEvent* event)
{
	if (event->modifiers() & Qt::ControlModifier)
	{
		event->ignore();
	}
	else QScrollArea::wheelEvent(event);
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

TrackGroup* TrackGroup::fromXml(const QDomElement& dom_element)
{
	QDomNodeList elements = dom_element.elementsByTagName("Track");
	TrackWidgetList tracks;
	for (int i =0; i < elements.count(); ++i)
	{
		const QDomElement& track_element = elements.at(i).toElement();

		QString type = track_element.attribute("type");
		TrackWidget* track = TrackWidget::fromXml(track_element, nullptr);
		if (track) tracks.append(track);
	}

	if (elements.count() > 0 && tracks.empty()) return nullptr;

	TrackGroup* tr = new TrackGroup;
	tr->addTrackWidgets(tracks);
	return tr;
}


