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
	foreach (TrackGroup* track_group, findChildren<TrackGroup*>())
	{
		if (track_group) track_group->reloadTracks();
	}
}

void PanelManager::addEmptyPanel()
{
	TrackGroup* new_panel = new TrackGroup();
	insertWidget(0, new_panel);
	connectSignals(new_panel);
}

void PanelManager::resizeGenePanel(int height)
{
	QList<int> sizes_old = sizes();
	if (sizes_old.count()<2) return;

	int height_all = std::accumulate(sizes_old.begin(), sizes_old.end(), 0);
	double trackgroups_height = height_all - sizes_old.last(); //subtract chromome and gene panel
	if (trackgroups_height<=0) return;

	QList<int> sizes_new;
	for (int i=0; i<sizes_old.count()-1; ++i)
	{
		double relative = sizes_old[i]/trackgroups_height;
		sizes_new << (relative*(height_all-height));
	}
	sizes_new << height;

	setSizes(sizes_new);
}

void PanelManager::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		int x = event->pos().x();
		//Only start dragging inside the genomic content area.
		if (x < SharedData::settings().label_width + 2 || x > width() - 2) return;

		is_dragging_ = true;
		drag_start_x_ = x;
		drag_start_region_ = SharedData::region();
	}
}

void PanelManager::mouseMoveEvent(QMouseEvent* event)
{
	if (is_dragging_) updateDragRegion(event->pos().x());
	update();
}

void PanelManager::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && is_dragging_)
	{
		updateDragRegion(event->pos().x());
	}

	is_dragging_ = false;

	update();
}

void PanelManager::updateDragRegion(int mouse_x)
{
	int w = width();
	int label_width = SharedData::settings().label_width;
	double total_width = width() - label_width - 4;
	if (total_width <= 0) return;

	int x_clamped = std::clamp(mouse_x, label_width + 2, w - 2);
	double bases_per_pxiel = (double)drag_start_region_.length() / total_width;
	double diff = (x_clamped - drag_start_x_) * bases_per_pxiel;

	int new_start = drag_start_region_.start() - std::lround(diff);
	int new_end = drag_start_region_.end() - std::lround(diff);

	SharedData::setRegion(drag_start_region_.chr(), new_start, new_end);
}

void PanelManager::loadFile(QString filename)
{
	//empty session => remove all track groups
	bool empty_session = isEmptySession();
	if (empty_session) removeAll();

	//add new track group with file contents
	TrackGroup* new_panel = TrackGroup::fromFile(filename);
	if (new_panel)
	{
		connectSignals(new_panel);
		insertWidget(0, new_panel);
	}

	//resize gene panel
	if (empty_session) resizeGenePanel();
}

void PanelManager::addPanelAbove()
{
	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
	if (senderWidget)
	{
		int idx = indexOf(senderWidget);
		if (idx >= 0)
		{
			TrackGroup* new_panel = new TrackGroup();
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
			TrackGroup* new_panel = new TrackGroup();
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
	for (int i = 0; i < count(); ++i)
	{
		QWidget* widget = this->widget(i);
		TrackGroup* track_group = qobject_cast<TrackGroup*>(widget);

		if (track_group)
		{
			track_group->writeToXml(writer);
		}
	}
}

void PanelManager::loadFromXml(const QDomElement& dom_element, QStringList& errors)
{
	QDomNodeList elements = dom_element.elementsByTagName("TrackGroup");
	for (int i =0; i < elements.count(); ++i)
	{
		TrackGroup* panel = new TrackGroup();
		panel->loadFromXml(elements.at(i).toElement(), errors);
		connectSignals(panel);
		insertWidget(count() - 1, panel);
	}
}

void PanelManager::removeAll()
{
	foreach (TrackGroup* track_group, findChildren<TrackGroup*>())
	{
		//hide and delete
		track_group->hide();
		track_group->setParent(nullptr);
		track_group->deleteLater();
	}
}

bool PanelManager::isEmptySession() const
{
	foreach (TrackGroup* track_group, findChildren<TrackGroup*>())
	{
		if (track_group->trackCount()>0) return false;
	}

	return true;
}
