#ifndef TRACKWIDGET_H
#define TRACKWIDGET_H

#include "cppVISUAL_global.h"
#include "TrackManager.h"

#include <QVector>
#include <QDomElement>
#include <QHash>
#include <QMouseEvent>
#include <QWidget>
#include <QXmlStreamWriter>

class CPPVISUALSHARED_EXPORT TrackWidget
	: public QWidget
{
	Q_OBJECT

public:
	explicit TrackWidget(QWidget* parent, QString file_path, QString name);
	~TrackWidget();

	const QUuid& id() {return id_;}
	virtual void writeToXml(QXmlStreamWriter&);
	virtual QMap<QString, QVariant> getSettings();
	virtual void loadSettingsFromXml(const QDomNodeList&);
	static TrackWidget* loadFromXml(const QDomElement&, QWidget* parent);
	virtual void reloadTrack(){}

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
	virtual QString getType() = 0;

	enum DrawMode
	{
		COLLAPSED,
		EXPANDED
	};

	DrawMode draw_mode_ = COLLAPSED;
	QPoint drag_start_pos_;
	bool is_dragging_;
	QAction *opts_[3];


	QUuid id_;
	QString file_path_;
	QString name_;
};

using TrackWidgetList = QVector<TrackWidget*>;



#endif // TRACKWIDGET_H
