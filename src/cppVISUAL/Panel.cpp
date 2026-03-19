#include "Panel.h"
#include "BedTrack.h"
#include "BedFile.h"
#include "TrackManager.h"

#include <QMenu>
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

	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));

	setAcceptDrops(true);

	layout_->addStretch(1);
	layout_->setContentsMargins(0, 0, 0, height());
}

void Panel::trackAdded(QSharedPointer<Track> tr)
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
	qDebug() << "Track moved called" << Qt::endl;
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
		QString file_path = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Bed Files(*.bed);;Text Files (*.txt);;All Files (*)")); //

		if (file_path.isEmpty()) return;

		BedFile track;
		const QFileInfo info(file_path);
		if (info.isFile())
		{
			track.load(file_path);
			track.sort();

			/*
			 * TODO: send an error somehow
			 */
			if (track.chromosomes().count() != 1) return; // discard

			QSharedPointer<Track> tr = QSharedPointer<Track>(new Track(/*file path*/ file_path,
																	   /*filename*/  info.fileName(),
																	   /*BedFile*/   track));
			trackAdded(tr);
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


void Panel::dropEvent(QDropEvent* event)
{
	if (event->mimeData()->hasFormat("application/track-data"))
	{
		QByteArray track_data = event->mimeData()->data("application/track-data");
		QUuid track_id = QUuid(track_data);
		qDebug() << "Recevied track Id: " << track_id << Qt::endl;

		qDebug() << "DROPPED" << Qt::endl;

		if (TrackManager::hasTrackWidget(track_id))
		{

			BedTrack* source = qobject_cast<BedTrack*>(TrackManager::getTrackWidget(track_id));
			if (!source) return;
			qDebug() << "Source detected" << Qt::endl;
			QWidget* old_panel = qobject_cast<QWidget*>(source->parentWidget());
			if (!old_panel) qDebug() << "Old panel not detected" << Qt::endl;
			if (!source->parentWidget()) qDebug() << "Old panel has no parent" << Qt::endl;
			if (old_panel && old_panel != content_widget_)
			{
				qDebug() << "Old Panel != this" << Qt::endl;
				emit source->trackMoved();

				source->setParent(this);
				connect(source, SIGNAL(trackDeleted()), this, SLOT(trackDeleted()));
				connect(source, SIGNAL(trackMoved()), this, SLOT(trackMoved()));
				layout_->insertWidget(layout_->count() - 1, source);
				layout_->update();
			}
			else
			{
				qDebug() << "Old Panel is this" << Qt::endl;
			}
		}
	}
	event->acceptProposedAction();
}


