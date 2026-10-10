#include "FileLoader.h"
#include "QtXml/qdom.h"
#include "TrackGroup.h"
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QFileInfo>
#include <QFileDialog>


TrackGroup::TrackGroup(QWidget* parent)
	: QScrollArea(parent)
	, layout_(new QVBoxLayout(this))
	, content_widget_(new QWidget(this))
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
		disconnect(senderWidget, &TrackWidget::trackSelected, this, &TrackGroup::trackSelected);
		disconnect(senderWidget, &TrackWidget::editSettingsRequested, this, &TrackGroup::editSettingsRequested);
		layout_->removeWidget(senderWidget);
		layout_->update();

		if (layout_->count() == 1) deleteLater();
	}
}

void TrackGroup::connectTrackSignals(TrackWidget* track)
{
	connect(track, &TrackWidget::trackDeleted, this, &TrackGroup::trackDeleted);
	connect(track, &TrackWidget::trackMoved, this, &TrackGroup::trackMoved);
	connect(track, &TrackWidget::trackSelected, this, &TrackGroup::trackSelected);
	connect(track, &TrackWidget::editSettingsRequested, this, &TrackGroup::editSettingsRequested);
}

void TrackGroup::addTrackWidgets(QVector<TrackWidget*> widgets)
{
	foreach (TrackWidget* widget, widgets)
	{
		connectTrackSignals(widget);
		layout_->insertWidget(layout_->count() - 1, widget);
		layout_->update();
	}
}

void TrackGroup::loadTracksFromFile()
{
	QString file_path =  QFileDialog::getOpenFileName(QApplication::activeWindow(), "Open file(s)", "", "NGS files(*.bam *.cram *.bed *.igv);;All files(*.*)");
	if (file_path.isEmpty()) return;

	QVector<TrackWidget*> widgets = FileLoader::loadTracks(file_path, nullptr);
	addTrackWidgets(widgets);
}

TrackGroup* TrackGroup::fromFile(QString filename)
{
	TrackGroup* tr = new TrackGroup;
	tr->addTrackWidgets(FileLoader::loadTracks(filename, nullptr));
	return tr;
}

void TrackGroup::reloadTracks()
{
	foreach (TrackWidget* track_widget, findChildren<TrackWidget*>())
	{
		track_widget->reloadTrack();
	}
}

void TrackGroup::contextMenu(QPoint pos)
{
	QMenu menu(this);

	//add track sub-menu (if not empty TrackGroup)
	TrackWidget* track = getTrackUnderMouse(pos);
	if (track)
	{
		emit trackSelected(track);
		QMenu* track_menu = new QMenu("Track", this);
		track->populateContextMenu(*track_menu, track->mapFrom(viewport(), pos));
		menu.addMenu(track_menu);
	}

	//add TrackGroup sub-menu
	QMenu* track_group_menu = new QMenu("TrackGroup", this);
	QAction* load_file = track_group_menu->addAction("Add track(s) from file");
	connect(load_file, &QAction::triggered, this, &TrackGroup::loadTracksFromFile);
	track_group_menu->addSeparator();
	QAction* clear_track_group = track_group_menu->addAction("Clear TrackGroup");
	connect(clear_track_group, &QAction::triggered, this, &TrackGroup::clearLayout);
	QAction* remove_track_group = track_group_menu->addAction("Remove TrackGroup");
	connect(remove_track_group, &QAction::triggered, this, &TrackGroup::clearLayoutAndDelete);
	track_group_menu->addSeparator();
	QAction* add_track_group_above = track_group_menu->addAction("Add TrackGroup Above");
	connect(add_track_group_above, &QAction::triggered, this, &TrackGroup::addTrackGroupAbove);
	QAction* add_track_group_below = track_group_menu->addAction("Add TrackGroup Below");
	connect(add_track_group_below, &QAction::triggered, this, &TrackGroup::addTrackGroupBelow);
	menu.addMenu(track_group_menu);

	menu.exec(viewport()->mapToGlobal(pos));
}

void TrackGroup::clearLayout()
{
	while (QLayoutItem* item = layout_->takeAt(0))
	{
		if (QWidget* widget = item->widget()) widget->deleteLater();
		delete item;
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
	for (int i = 0; i < layout_->count() - 1; ++i)
	{
		QWidget* w = layout_->itemAt(i)->widget();
		if (w && y > w->geometry().center().y())
		{
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
		if (track->rect().contains(track->mapFrom(viewport(), pos))) return track;
	}
	return nullptr;
}


void TrackGroup::dropEvent(QDropEvent* event)
{
	TrackWidget* track = qobject_cast<TrackWidget*>(event->source());
	if (!track) return;

	QWidget* old_content_widget = track->parentWidget();
	if (!old_content_widget)
	{
		qDebug() << "Parent widget was not found for source on drop!" << Qt::endl;
		return;
	}

	if (old_content_widget != content_widget_) // came from a different TrackGroup
	{
		emit track->trackMoved(); //disconnects the old signals to the old TrackGroup

		connectTrackSignals(track);
	}
	else // dropped in the same TrackGroup
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
	foreach (TrackWidget* track, findChildren<TrackWidget*>())
	{
		track->writeToXml(writer);
	}
	writer.writeEndElement(); // TrackGroup
}

void TrackGroup::loadFromXml(const QDomElement& dom_element, QStringList& errors)
{
	QDomNodeList elements = dom_element.elementsByTagName("Track");
	for (int i =0; i < elements.count(); ++i)
	{
		const QDomElement& track_element = elements.at(i).toElement();
		TrackWidget* track = TrackWidget::fromXml(track_element, this, errors);
		if (track)
		{
			addTrackWidgets({track});
		}
	}
}

TrackGroup* TrackGroup::fromXml(const QDomElement& dom_element, QStringList& errors)
{
	QDomNodeList elements = dom_element.elementsByTagName("Track");
	QVector<TrackWidget*> tracks;
	for (int i =0; i < elements.count(); ++i)
	{
		const QDomElement& track_element = elements.at(i).toElement();
		TrackWidget* track = TrackWidget::fromXml(track_element, nullptr, errors);
		if (track) tracks.append(track);
	}

	if (elements.count() > 0 && tracks.empty()) return nullptr;

	TrackGroup* tr = new TrackGroup;
	tr->addTrackWidgets(tracks);
	return tr;
}

int TrackGroup::trackCount()
{
	return findChildren<TrackWidget*>().count();
}
