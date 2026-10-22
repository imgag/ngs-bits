#include "FileLoader.h"
#include "BamAlignmentTrack.h"
#include "QtXml/qdom.h"
#include "TrackGroup.h"
#include "Helper.h"
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>


TrackGroup::TrackGroup(QWidget* parent)
	: QScrollArea(parent)
	, layout_(new QVBoxLayout(this))
	, content_widget_(new QWidget(this))
	, empty_message_(new QLabel(viewport()))
{
	//Override QScrollArea's default minimum height for splitter resizing.
	setMinimumHeight(25);
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
	empty_message_->setObjectName("empty_track_group_message");
	empty_message_->setText("Empty track group. Drag/drop a track or load a track using the context menu.");
	empty_message_->setAlignment(Qt::AlignCenter);
	empty_message_->setWordWrap(true);
	empty_message_->setAttribute(Qt::WA_TransparentForMouseEvents);
	updateEmptyState();
}

void TrackGroup::updateEmptyState()
{
	empty_message_->setGeometry(viewport()->rect().adjusted(12, 8, -12, -8));
	empty_message_->setVisible(layout_->count() == 1);
	empty_message_->raise();
}

void TrackGroup::trackDeleted()
{
	TrackWidget* senderWidget = qobject_cast<TrackWidget*>(sender());
	if (senderWidget) {
		layout_->removeWidget(senderWidget);
		senderWidget->deleteLater();
		layout_->update();
	}

	updateEmptyState();
	if (layout_->count() == 1) deleteLater();
}

void TrackGroup::trackMoved()
{
	TrackWidget* senderWidget = qobject_cast<TrackWidget*>(sender());
	if (senderWidget){
		disconnect(senderWidget, SIGNAL(trackDeleted()), this, SLOT(trackDeleted()));
		disconnect(senderWidget, SIGNAL(trackMoved()), this, SLOT(trackMoved()));
		disconnect(senderWidget, &TrackWidget::trackSelected, this, &TrackGroup::trackSelected);
		disconnect(senderWidget, &TrackWidget::settingsRequested, this, &TrackGroup::settingsRequested);
		layout_->removeWidget(senderWidget);
		layout_->update();

		updateEmptyState();
		if (layout_->count() == 1) deleteLater();
	}
}

void TrackGroup::connectTrackSignals(TrackWidget* track)
{
	connect(track, &TrackWidget::trackDeleted, this, &TrackGroup::trackDeleted);
	connect(track, &TrackWidget::trackMoved, this, &TrackGroup::trackMoved);
	connect(track, &TrackWidget::trackSelected, this, &TrackGroup::trackSelected);
	connect(track, &TrackWidget::settingsRequested, this, &TrackGroup::settingsRequested);
}

void TrackGroup::addTrackWidgets(QVector<TrackWidget*> widgets)
{
	foreach (TrackWidget* widget, widgets)
	{
		connectTrackSignals(widget);
		layout_->insertWidget(layout_->count() - 1, widget);
		layout_->update();
	}
	updateEmptyState();
}

void TrackGroup::loadTracksFromFile()
{
	QString file_path =  QFileDialog::getOpenFileName(QApplication::activeWindow(), "Open file(s)", "", "NGS files(*.bam *.cram *.bed *.bed.gz *.bedpe *.bedpe.gz *.vcf *.vcf.gz *.igv);;All files(*.*)");
	if (file_path.isEmpty()) return;

	QVector<TrackWidget*> widgets = FileLoader::loadTracks(file_path, nullptr);
	addTrackWidgets(widgets);
}

void TrackGroup::loadTracksFromUrl()
{
	const QString title = "Add track(s) from URL";
	const QString url = QInputDialog::getText(this, title, "URL").trimmed();
	if (url.isEmpty()) return;
	if (!Helper::isHttpUrl(url))
	{
		QMessageBox::warning(this, title, "This is not a URL:\n" + url);
		return;
	}
	try
	{
		addTrackWidgets(FileLoader::loadTracks(url, nullptr));
	}
	catch (const Exception& e)
	{
		QMessageBox::warning(this, title, e.message());
	}
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

	//Track-specific element submenus precede the shared Track submenu.
	TrackWidget* track = getTrackUnderMouse(pos);
	if (track)
	{
		emit trackSelected(track);
		track->populateContextMenu(menu, track->mapFrom(viewport(), pos));
	}

	//add TrackGroup sub-menu
	QMenu* track_group_menu = new QMenu("TrackGroup", this);
	QAction* load_file = track_group_menu->addAction("Add track(s) from file");
	connect(load_file, &QAction::triggered, this, &TrackGroup::loadTracksFromFile);
	QAction* load_url = track_group_menu->addAction("Add track(s) from URL");
	connect(load_url, &QAction::triggered, this, &TrackGroup::loadTracksFromUrl);
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
	//Keep the trailing stretch so tracks can be added again after clearing.
	while (layout_->count() > 1)
	{
		QLayoutItem* item = layout_->takeAt(0);
		if (QWidget* widget = item->widget()) widget->deleteLater();
		delete item;
	}
	updateEmptyState();
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
	updateEmptyState();

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
	writer.writeAttribute("height", QString::number(height()));
	for (int i = 0; i < layout_->count(); ++i)
	{
		if (auto* track = qobject_cast<TrackWidget*>(layout_->itemAt(i)->widget())) track->writeToXml(writer);
	}
	writer.writeEndElement(); // TrackGroup
}

void TrackGroup::loadFromXml(const QDomElement& dom_element, QStringList& errors)
{
	session_height_ = dom_element.attribute("height").toInt();
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
	restoreAlignmentHeights();
}

int TrackGroup::alignmentSessionHeight() const
{
	int height = 0;
	for (BamAlignmentTrack* track : findChildren<BamAlignmentTrack*>()) height += track->sessionHeight();
	return height;
}

void TrackGroup::resizeEvent(QResizeEvent* event)
{
	QScrollArea::resizeEvent(event);
	updateEmptyState();
	restoreAlignmentHeights();
}

void TrackGroup::restoreAlignmentHeights()
{
	const int total = alignmentSessionHeight();
	if (session_height_ <= 0 || total <= 0) return;
	const int difference = height() - session_height_;
	int assigned = 0;
	int cumulative = 0;
	for (BamAlignmentTrack* track : findChildren<BamAlignmentTrack*>())
	{
		if (track->sessionHeight() <= 0) continue;
		cumulative += track->sessionHeight();
		const int share = qRound((double)difference * cumulative / total);
		track->setFixedHeight(std::max(1, track->sessionHeight() + share - assigned));
		assigned = share;
	}
}

int TrackGroup::trackCount()
{
	return findChildren<TrackWidget*>().count();
}
