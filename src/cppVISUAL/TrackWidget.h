#ifndef TRACKWIDGET_H
#define TRACKWIDGET_H

#include "cppVISUAL_global.h"
#include "GenomeVisualizationWidget.h"
#include "ParameterList.h"
#include "BedFile.h"
#include <QMouseEvent>
#include <QWidget>
#include <QXmlStreamWriter>
#include <QDomElement>

//A structure for loading track states from various different sources
struct LoadContext {
	QString track_type;
	QString file_path;
	QString display_name;
	QStringList& errors;
	int height;
};

//struct for pixel x coordiante <> genome position calculations
struct CPPVISUALSHARED_EXPORT Viewport
{
	int usable_width; //usable width of painting (widget width - label region width - 4px margin)
	int x0; //leftmost x-coodinate
	float pixels_per_base; //pixels per base
	int genome_start; //one-based coordinate at x0

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
	//The parent must belong to the viewer that owns this track.
	TrackWidget(QWidget* parent, QString file_path, QString display_name, QString type);
	QSize minimumSizeHint() const override;
	ParameterList& parameters() { return settings_; }
	QString displayName() const { return display_name_; }
	int sessionHeight() const { return session_height_; }

	//re-loads a the track from file/URL
	virtual void reloadTrack() = 0;
	//Adds the Track submenu to the root context menu. Derived classes add element submenus first.
	virtual void populateContextMenu(QMenu&, const QPoint&);
	//Returns the track type as a string
	LoadContext loadContext(QStringList& errors) const;

	//writes properties in XML
	void writeToXml(QXmlStreamWriter&);
	//creates TrackWidget based on the given XML specification
	static TrackWidget* fromXml(const QDomElement&, QWidget* parent, QStringList &errors);
	static TrackWidget* load(const LoadContext cx, QWidget* parent);

	//determines a display name from file path or URL
	static QString getDisplayNameFromFilePath(QString file_path);

signals:
	void trackDeleted();
	void trackMoved();
	void trackSelected(TrackWidget* track);
	void settingsRequested(TrackWidget* track);

public slots:
	void regionChanged();

protected:
	GenomeVisualizationWidget* viewer_;
	virtual QList<Parameter> getParameters() const { return {}; }
	virtual QHash<QByteArray, QVariant> getParameterDefaults() const { return {}; }
	//Call in the derived constructor body, where virtual dispatch reaches its overrides.
	void initializeSettings(QObject* fast_access_struct);

	bool event(QEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	// creates a pop up at global_pos and displays the info text on that
	void showInfoPopup(QPointF global_pos, QString info);
	// draws the name of the widget on the left side
	void drawLabel(QPainter&);
	//Draws the overview placeholder and returns true if no single region is selected.
	bool drawOverviewMessage(QPainter&);
	//Returns the current viewport for drawing and coordinate conversion.
	Viewport getViewport() const;
	//One plot for a selected region, or proportional plots for all canonical chromosomes.
	QVector<QPair<BedLine, Viewport>> plotViewports() const;

	QString file_path_; //path or URL of the source file
	ParameterList settings_; //settings

private:
	QPoint drag_start_pos_;
	bool is_dragging_ = false;
	QString display_name_; //display name
	QString type_; //track type - used to serialize the track to XML
	int session_height_ = 0;

private slots:
	void handleTrackRename();
};

#endif // TRACKWIDGET_H
