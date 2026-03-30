#include "TrackManager.h"
#include "FileLoader.h"
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

	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));

	setAcceptDrops(true);

	layout_->addStretch(1);
	layout_->setContentsMargins(0, 0, 0, height());
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

bool TrackGroup::loadFile()
{
	QString file_path = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Bed Files(*.bed);;Text Files (*.txt);;All Files (*)"));
	if (file_path.isEmpty()) return false;

	TrackWidgetList widgets = FileLoader::loadTracks(file_path, this);
	if (widgets.isEmpty()) return false;
	foreach (TrackWidget* widget, widgets)
	{
		connect(widget, SIGNAL(trackDeleted()), this, SLOT(trackDeleted()));
		connect(widget, SIGNAL(trackMoved()), this, SLOT(trackMoved()));
		layout_->insertWidget(layout_->count() - 1, widget);
		layout_->update();
	}
	return true;
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
	QAction* opt1 = menu.addAction("Load file");

	menu.addSeparator();

	QAction* opt2 = menu.addAction("Clear Panel");
	QAction* opt3 = menu.addAction("Remove Panel");

	menu.addSeparator();

	QAction* opt4 = menu.addAction("Add Panel Above");
	QAction* opt5 = menu.addAction("Add Panel Below");

	QAction* action = menu.exec(mapToGlobal(pos));

	if (action == opt1)		 loadFile();
	else if (action == opt2) clearLayout();
	else if (action == opt3) clearLayoutAndDelete();
	else if (action == opt4) emit addPanelAbove();
	else if (action == opt5) emit addPanelBelow();
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

void TrackGroup::removeTrack(QWidget* widget)
{
	if (widget)
	{
		layout_->removeWidget(widget);
		layout_->update();
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


