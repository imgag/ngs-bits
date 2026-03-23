#include "FileLoader.h"
#include "PanelManager.h"
#include "SharedData.h"
#include "Panel.h"

#include <QMouseEvent>


PanelManager::PanelManager(QWidget* parent)
	: QSplitter(Qt::Vertical, parent)
{
	setChildrenCollapsible(false);
	setHandleWidth(2);
	setMouseTracking(true);

	connect(SharedData::instance(), SIGNAL(addTracks(TrackList)), this, SLOT(addTracks(TrackList)));
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


void PanelManager::addTracks(TrackList tracks)
{
	if (tracks.isEmpty()) return;
	class Panel* new_panel = new class Panel(this);
	foreach (QSharedPointer<TrackData> track, tracks)
	{
		new_panel->addTrack(track);
	}
	insertWidget(0, new_panel);
}

void PanelManager::loadFile(QString file)
{
	TrackList tracks = FileLoader::load(file);
	addTracks(tracks);
}
