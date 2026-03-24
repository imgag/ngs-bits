#ifndef TRACKWIDGET_H
#define TRACKWIDGET_H

#include "cppVISUAL_global.h"
#include "TrackManager.h"

#include <QVector>

#include <QHash>
#include <QMouseEvent>
#include <QWidget>

class CPPVISUALSHARED_EXPORT TrackWidget
	: public QWidget
{
	Q_OBJECT

public:
	explicit TrackWidget(QWidget* parent, QString file_path, QString name)
		:QWidget(parent), id_(QUuid::createUuid()), file_path_(file_path), name_(name)
	{
		TrackManager::addTrackWidget(id_, this);
	}
	~TrackWidget()
	{
		TrackManager::removeTrackWidget(id_);
	}

	QUuid id() {return id_;}

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

	DrawMode draw_mode_ = COLLAPSED;
	QPoint drag_start_pos_;
	bool is_dragging_;
	QAction* opt1_;
	QAction* opt2_;


	QUuid id_;
	QString file_path_;
	QString name_;
};

using TrackWidgetList = QVector<TrackWidget*>;



#endif // TRACKWIDGET_H
