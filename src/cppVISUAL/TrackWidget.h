#ifndef TRACKWIDGET_H
#define TRACKWIDGET_H

#include "cppVISUAL_global.h"
#include "TrackData.h"
#include "TrackManager.h"

#include <QHash>
#include <QMouseEvent>
#include <QWidget>

class CPPVISUALSHARED_EXPORT TrackWidget
	: public QWidget
{
	Q_OBJECT

public:
	explicit TrackWidget(QWidget* parent, QSharedPointer<TrackData> track)
		:QWidget(parent), track_(track)
	{
		TrackManager::addTrackWidget(track_->id, this);
	}
	~TrackWidget()
	{
		TrackManager::removeTrackWidget(track_->id);
	}

signals:
	void trackDeleted();
	void trackMoved();
public slots:
	virtual void regionChanged();
	virtual void contextMenu(QPoint pos);

protected:
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void populateContextMenu(QMenu&);
	virtual void handleContextMenuAction(QAction*);

	enum DrawMode
	{
		COLLAPSED,
		EXPANDED
	};

	QSharedPointer<TrackData> track_;

	DrawMode draw_mode_ = COLLAPSED;
	QPoint drag_start_pos_;
	bool is_dragging_;
	QAction* opt1_;
	QAction* opt2_;
};



#endif // TRACKWIDGET_H
