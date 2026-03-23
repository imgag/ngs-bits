#include "BedTrack.h"
#include "Panel.h"
#include "TrackManager.h"
#include "FileLoader.h"

#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QFileInfo>
#include <QFileDialog>

Panel::Panel(QWidget* parent)
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

void Panel::trackAdded(QSharedPointer<TrackData> tr)
{
	auto panel = new BedTrack(this, tr);
	connect(panel, SIGNAL(trackDeleted()), this, SLOT(trackDeleted()));
	connect(panel, SIGNAL(trackMoved()), this, SLOT(trackMoved()));
	layout_->insertWidget(layout_->count() - 1, panel);
	layout_->update();
}

void Panel::trackDeleted()
{
	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
	if (senderWidget) {
		layout_->removeWidget(senderWidget);
		senderWidget->deleteLater();
		layout_->update();
	}

	if (layout_->count() == 1) deleteLater();
}

void Panel::trackMoved()
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

void Panel::contextMenu(QPoint pos)
{
	QMenu menu(this);
	QAction* opt1 = menu.addAction("Load file");
	QAction* action = menu.exec(mapToGlobal(pos));

	if (action == opt1)
	{
		QString file_path = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Bed Files(*.bed);;Text Files (*.txt);;All Files (*)"));

		if (file_path.isEmpty()) return;

		TrackList tracks = FileLoader::load(file_path);
		if (!tracks.isEmpty())
		{
			foreach (QSharedPointer<TrackData> track, tracks)
			{
				trackAdded(track);
			}
		}
	}
}

void Panel::clearLayout()
{
	if (!layout_) return;
	while (QLayoutItem* item = layout_->takeAt(0))
	{
		if (QWidget* widget = item->widget()) widget->deleteLater();
		delete item;
	}
}


void Panel::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasFormat("application/track-data"))
	{
		event->acceptProposedAction();
	}
}

void Panel::removeTrack(QWidget* widget)
{
	if (widget)
	{
		layout_->removeWidget(widget);
		layout_->update();
	}
}


inline int Panel::getDropIndex(int y)
{
	int drop_index =0;
	for (int i = 0; i < layout_->count() - 1; ++i) {
		QWidget* w = layout_->itemAt(i)->widget();
		if (w && y > w->geometry().center().y()) {
			drop_index = i + 1;
		}
	}
	drop_index = std::max(0, std::min(drop_index, layout_->count() - 2));

	return drop_index;
}


void Panel::dropEvent(QDropEvent* event)
{
	if (event->mimeData()->hasFormat("application/track-data"))
	{
		QByteArray track_data = event->mimeData()->data("application/track-data");
		QUuid track_id = QUuid(track_data);

		if (TrackManager::hasTrackWidget(track_id))
		{
			BedTrack* source = qobject_cast<BedTrack*>(TrackManager::getTrackWidget(track_id));
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

				// source->setParent(content_widget_);
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


