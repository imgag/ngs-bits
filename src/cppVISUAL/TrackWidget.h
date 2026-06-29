#ifndef TRACKWIDGET_H
#define TRACKWIDGET_H

#include "cppVISUAL_global.h"
#include "TrackManager.h"
#include "BedFile.h"

#include <QUuid>
#include <QVector>
#include <QDomElement>
#include <QHash>
#include <QMouseEvent>
#include <QWidget>
#include <QXmlStreamWriter>

struct CPPVISUALSHARED_EXPORT Viewport
{
	const BedLine& region;
	int total_width;
	int x0;
	float pixels_per_base;

	float genomePosToScreen(int genome_pos) const;
	float genomeWidthToScreen(int genome_width) const;
	int screenXToGenomePos(int x_pos) const;
	bool isOutOfDrawRegion(int x_pos) const;
};

// A generic Track, all tracks must inherit from this and implement the getType() function
class CPPVISUALSHARED_EXPORT TrackWidget
	: public QWidget
{
	Q_OBJECT

public:
	explicit TrackWidget(QWidget* parent, QString file_path, QString name);
	// ~TrackWidget();

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

signals:
	void trackDeleted();
	void trackMoved();

public slots:
	virtual void regionChanged();
	void handleTrackRename();

protected:
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	// creates a pop up at global_pos and displays the info text on that
	virtual void showInfoPopup(QPointF global_pos, QString info);
	// draws the name of the widget on the left side
	void drawLabel(QPainter&);
	// called when rename is clicked
	// returns the current viewport
	virtual Viewport getViewport();

	virtual QString getType() = 0;

	QPoint drag_start_pos_;
	bool is_dragging_;

	QUuid id_;
	QString file_path_;
	QString name_;
};

using TrackWidgetList = QVector<TrackWidget*>;



#endif // TRACKWIDGET_H
