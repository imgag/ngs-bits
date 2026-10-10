#ifndef TRACKWIDGET_H
#define TRACKWIDGET_H

#include "cppVISUAL_global.h"
#include "BedFile.h"
#include "ParameterList.h"
#include <QMouseEvent>
#include <QWidget>
#include <QXmlStreamWriter>
#include <QDomElement>

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

// Track base class - all tracks inherit from this
class CPPVISUALSHARED_EXPORT TrackWidget
	: public QWidget
{
	Q_OBJECT

public:
	TrackWidget(QWidget* parent, QString file_path, QString display_name, QString type);
	QSize minimumSizeHint() const override;

	//re-loads a the track from file/URL
	virtual void reloadTrack() {};
	//adds general track context menu entries
	virtual void populateContextMenu(QMenu&, const QPoint&);

	//writes properties in XML
	void writeToXml(QXmlStreamWriter&);
	//creates TrackWidget based on the given XML specification
	static TrackWidget* fromXml(const QDomElement&, QWidget* parent, QStringList &errors);

	//determines a display name from file path or URL
	static QString getDisplayNameFromFilePath(QString file_path);

signals:
	void trackDeleted();
	void trackMoved();

public slots:
	virtual void regionChanged();
	void handleTrackRename();

protected:
	virtual QList<Parameter> getParameters() const { return {}; }
	virtual QHash<QByteArray, QVariant> getParameterDefaults() const { return {}; }
	//Call in the derived constructor body, where virtual dispatch reaches its overrides.
	void initializeSettings();

	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	// creates a pop up at global_pos and displays the info text on that
	virtual void showInfoPopup(QPointF global_pos, QString info);
	// draws the name of the widget on the left side
	void drawLabel(QPainter&);
	// called when rename is clicked. Returns the current viewport
	virtual Viewport getViewport();

	QPoint drag_start_pos_;
	bool is_dragging_;

	QString file_path_; //path or URL of the source file
	QString display_name_; //display name
	QString type_; //track type - used to serialize the track to XML
	ParameterList settings_; //settings
};

#endif // TRACKWIDGET_H
