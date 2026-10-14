#include "TrackGroupManager.h"
#include "SharedData.h"
#include "TrackGroup.h"
#include <QMouseEvent>
#include <QTimer>

TrackGroupManager::TrackGroupManager(QWidget* parent)
	: QSplitter(Qt::Vertical, parent)
{
	setChildrenCollapsible(false);
	setHandleWidth(2);
	setMouseTracking(true);
	connect(this, &QSplitter::splitterMoved, this, [this]()
	{
		if (session_gene_height_ <= 0) return;
		session_group_heights_ = sizes();
		session_gene_height_ = session_group_heights_.takeLast();
	});
}


void TrackGroupManager::reloadTracks()
{
	foreach (TrackGroup* track_group, findChildren<TrackGroup*>())
	{
		if (track_group) track_group->reloadTracks();
	}
}

void TrackGroupManager::addEmptyTrackGroup()
{
	TrackGroup* new_track_group = new TrackGroup();
	insertWidget(0, new_track_group);
	connectSignals(new_track_group);
}

void TrackGroupManager::addTrack(TrackWidget* track)
{
	const bool empty_session = isEmptySession();
	if (empty_session) removeAll();
	auto* group = new TrackGroup();
	group->addTrackWidgets({track});
	connectSignals(group);
	insertWidget(0, group);
	if (empty_session) resizeGenePanel();
}

void TrackGroupManager::resizeGenePanel(int height)
{
	if (session_gene_height_ > 0) return; //Do not overwrite a restored session during delayed initialization.
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

void TrackGroupManager::mousePressEvent(QMouseEvent* event)
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

void TrackGroupManager::mouseMoveEvent(QMouseEvent* event)
{
	if (is_dragging_) updateDragRegion(event->pos().x());
	update();
}

void TrackGroupManager::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && is_dragging_)
	{
		updateDragRegion(event->pos().x());
	}

	is_dragging_ = false;

	update();
}

void TrackGroupManager::updateDragRegion(int mouse_x)
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

void TrackGroupManager::loadFile(QString filename)
{
	//empty session => remove all track groups
	bool empty_session = isEmptySession();
	if (empty_session) removeAll();

	//add new track group with file contents
	TrackGroup* new_track_group = TrackGroup::fromFile(filename);
	if (new_track_group)
	{
		connectSignals(new_track_group);
		insertWidget(0, new_track_group);
	}

	//resize gene panel
	if (empty_session) resizeGenePanel();
}

void TrackGroupManager::addTrackGroupAbove()
{
	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
	if (senderWidget)
	{
		int idx = indexOf(senderWidget);
		if (idx >= 0)
		{
			TrackGroup* new_track_group = new TrackGroup();
			insertWidget(idx, new_track_group);
			connectSignals(new_track_group);
		}
	}
}

void TrackGroupManager::addTrackGroupBelow()
{
	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
	if (senderWidget)
	{
		int idx = indexOf(senderWidget);
		if (idx >= 0)
		{
			TrackGroup* new_track_group = new TrackGroup();
			insertWidget(std::min(idx + 1, count() - 1), new_track_group);
			connectSignals(new_track_group);
		}
	}
}

void TrackGroupManager::connectSignals(TrackGroup* track_group)
{
	connect(track_group, &TrackGroup::addTrackGroupAbove, this, &TrackGroupManager::addTrackGroupAbove);
	connect(track_group, &TrackGroup::addTrackGroupBelow, this, &TrackGroupManager::addTrackGroupBelow);
	connect(track_group, &TrackGroup::trackSelected, this, &TrackGroupManager::trackSelected);
}

void TrackGroupManager::writeToXml(QXmlStreamWriter& writer)
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

void TrackGroupManager::loadFromXml(const QDomElement& dom_element, QStringList& errors)
{
	QList<int> heights;
	QDomNodeList elements = dom_element.elementsByTagName("TrackGroup");
	for (int i =0; i < elements.count(); ++i)
	{
		TrackGroup* track_group = new TrackGroup();
		track_group->loadFromXml(elements.at(i).toElement(), errors);
		connectSignals(track_group);
		insertWidget(count() - 1, track_group);
		heights.append(elements.at(i).toElement().attribute("height").toInt());
	}
	session_group_heights_ = heights;
	session_gene_height_ = dom_element.firstChildElement("General").attribute("gene_panel_height").toInt();
	//Wait until loaded widgets have been included in the splitter layout.
	QTimer::singleShot(0, this, &TrackGroupManager::restoreSessionHeights);
}

void TrackGroupManager::resizeEvent(QResizeEvent* event)
{
	QSplitter::resizeEvent(event);
	restoreSessionHeights();
}

void TrackGroupManager::restoreSessionHeights()
{
	const QList<int>& group_heights = session_group_heights_;
	const int gene_panel_height = session_gene_height_;
	if (gene_panel_height <= 0 || group_heights.size() != count() - 1) return;
	int total = gene_panel_height;
	int alignment_total = 0;
	for (int i = 0; i < group_heights.size(); ++i)
	{
		if (group_heights[i] <= 0) return;
		total += group_heights[i];
		auto* group = qobject_cast<TrackGroup*>(widget(i));
		if (group) alignment_total += group->alignmentSessionHeight();
	}
	const int difference = contentsRect().height() - handleWidth() * (count() - 1) - total;
	QList<int> heights;
	int cumulative = 0;
	int assigned = 0;
	for (int i = 0; i < group_heights.size(); ++i)
	{
		auto* group = qobject_cast<TrackGroup*>(widget(i));
		const int weight = alignment_total > 0 ? (group ? group->alignmentSessionHeight() : 0) : group_heights[i];
		cumulative += weight;
		const int share = qRound((double)difference * cumulative / (alignment_total > 0 ? alignment_total : total - gene_panel_height));
		heights.append(std::max(25, group_heights[i] + share - assigned));
		assigned = share;
	}
	heights.append(gene_panel_height);
	setSizes(heights);
}

void TrackGroupManager::removeAll()
{
	session_group_heights_.clear();
	session_gene_height_ = 0;
	foreach (TrackGroup* track_group, findChildren<TrackGroup*>())
	{
		//hide and delete
		track_group->hide();
		track_group->setParent(nullptr);
		track_group->deleteLater();
	}
}

bool TrackGroupManager::isEmptySession() const
{
	foreach (TrackGroup* track_group, findChildren<TrackGroup*>())
	{
		if (track_group->trackCount()>0) return false;
	}

	return true;
}
