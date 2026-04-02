#include "PanelManager.h"
#include "SharedData.h"
#include "TrackGroup.h"

#include <QMouseEvent>


PanelManager::PanelManager(QWidget* parent)
	: QSplitter(Qt::Vertical, parent)
{
	setChildrenCollapsible(false);
	setHandleWidth(2);
	setMouseTracking(true);
}


void PanelManager::reloadTracks()
{
	QList<TrackGroup*> track_groups = findChildren<TrackGroup*>();
	foreach (TrackGroup* track_group, track_groups)
	{
		if (track_group) track_group->reloadTracks();
	}
}

void PanelManager::newSession()
{
	QList<TrackGroup*> track_groups = findChildren<TrackGroup*>();
	foreach (TrackGroup* track_group, track_groups)
	{
		if (track_group) track_group->deleteLater();
	}
}

void PanelManager::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		int x = event->pos().x();
		is_dragging_ = true;
		drag_start_x_ = x;
		drag_start_region_ = SharedData::region();
	}
}



void PanelManager::mouseMoveEvent(QMouseEvent* event)
{
	int x = event->pos().x();

	if (is_dragging_)
	{
		int w = width();
		int label_width = SharedData::settings().label_width;
		float total_width = width() - label_width - 4;

		if (x > label_width + 2 && x < w - 2)
		{
			if (abs(x - drag_start_x_) >= 5)
			{
				float end = x;
				float start = drag_start_x_;
				float diff = ((end - start)/total_width) * (.4 * drag_start_region_.length());

				SharedData::setRegion(drag_start_region_.chr(), drag_start_region_.start() - diff, drag_start_region_.end() - diff);
			}
		}

		update();
	}
}


void PanelManager::mouseReleaseEvent(QMouseEvent* event)
{
	int x = event->pos().x();
	int w = width();
	int label_width = SharedData::settings().label_width;
	float total_width = width() - label_width - 4;

	if (event->button() == Qt::LeftButton && is_dragging_)
	{
		if (x > label_width + 2 && x < w - 2)
		{
			if (abs(x - drag_start_x_) >= 5)
			{
				float end = x;
				float start = drag_start_x_;
				float diff = ((end - start)/total_width) * (.4 * drag_start_region_.length());

				SharedData::setRegion(drag_start_region_.chr(), drag_start_region_.start() - diff, drag_start_region_.end() - diff);
			}
		}
	}

	is_dragging_ = false;
	update();
}


void PanelManager::loadFile()
{
	TrackGroup* new_panel = new TrackGroup();
	if (new_panel->loadFile())
	{
		connectSignals(new_panel);
		insertWidget(0, new_panel);
	}
	else new_panel->deleteLater();
}

void PanelManager::addPanelAbove()
{
	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
	if (senderWidget)
	{
		int idx = indexOf(senderWidget);
		if (idx >= 0)
		{
			TrackGroup* new_panel = new TrackGroup;
			insertWidget(idx, new_panel);
			connectSignals(new_panel);
		}
	}
}

void PanelManager::addPanelBelow()
{
	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
	if (senderWidget)
	{
		int idx = indexOf(senderWidget);
		if (idx >= 0)
		{
			TrackGroup* new_panel = new TrackGroup;
			insertWidget(std::min(idx + 1, count() - 1), new_panel);
			connectSignals(new_panel);
		}
	}
}

void PanelManager::connectSignals(TrackGroup* panel)
{
	connect(panel, SIGNAL(addPanelAbove()), this, SLOT(addPanelAbove()));
	connect(panel, SIGNAL(addPanelBelow()), this, SLOT(addPanelBelow()));
}


void PanelManager::writeToXml(QXmlStreamWriter& writer)
{
	QList<TrackGroup*> track_groups = findChildren<TrackGroup*>();
	foreach (TrackGroup* track_group, track_groups)
	{
		if (track_group) track_group->writeToXml(writer);
	}
}

void PanelManager::loadFromXml(QDomElement& dom_element)
{
	QDomNodeList elements = dom_element.elementsByTagName("TrackGroup");
	for (int i =0; i < elements.count(); ++i)
	{
		TrackGroup* panel = new TrackGroup;
		panel->loadFromXml(elements.at(i).toElement());
		connectSignals(panel);
		insertWidget(count() - 1, panel);
	}
}
