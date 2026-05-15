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
	// writes properties in XML
	void writeToXml(QXmlStreamWriter&);
	// settings that should be written into XML
	virtual QMap<QString, QVariant> getSettings() {
		return QMap<QString, QVariant>();
	};
	// parses the DOM and loads corresponding settings
	void loadSettingsFromXml(const QDomNodeList&);
	// function that should be overriden by child classes (re reading the file)
	virtual void reloadTrack() {};
	// mathod for loading a setting from XML
	virtual void loadKeyValueFromXml(QString, const QDomElement&){}

	// creates TrackWidget by parsing XML
	static TrackWidget* fromXml(const QDomElement&, QWidget* parent);
	// creates a TrackWidget from a given type, e.g. "BED".
	static TrackWidget* fromType(QString type, QWidget* parent, QString file_path, QString display_name);
	virtual void populateContextMenu(QMenu&);
	virtual void handleContextMenuAction(QAction*);

signals:
	void trackDeleted();
	void trackMoved();

public slots:
	virtual void regionChanged();

protected:
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	// creates a pop up at global_pos and display info text on that
	virtual void showInfoPopup(QPointF global_pos, QString info);
	// this is same for all track widgets
	void drawLabel(QPainter&);
	virtual QString getType() = 0;

	QPoint drag_start_pos_;
	bool is_dragging_;
	QAction *opts_[2];


	QUuid id_;
	QString file_path_;
	QString name_;
};

using TrackWidgetList = QVector<TrackWidget*>;



#endif // TRACKWIDGET_H
