#ifndef TRACKGROUP_H
#define TRACKGROUP_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"

#include <QDomElement>
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>
#include <QXmlStreamWriter>

// a single panel (collection of TrackWidgets)
class CPPVISUALSHARED_EXPORT TrackGroup
	: public QScrollArea
{
	Q_OBJECT

public:
	explicit TrackGroup(QWidget* = nullptr);
	/* reloads the tracks (calls reloadTrack for each track), if the file is bad the corresponding
	   track is deleted */
	void reloadTracks();
	// writes properties of each track to the XML writer for persistence
	void writeToXml(QXmlStreamWriter&);
	// reads <Track> elements in the dom and creates Tracks based on the properties
	void loadFromXml(const QDomElement&);
	// opens a file dialog and creates a TrackGroup with Tracks if file is valid
	static TrackGroup* fromFile();
	// reads <Track> elements in the dom and creates Tracks based on the properties, if no Track elements were created, returns null ptr
	static TrackGroup* fromXml(const QDomElement&);

signals:
	void addPanelAbove();
	void addPanelBelow();

public slots:
	// sent by the track that was deleted, deletes TrackGroup if there are no tracks remaining inside
	void trackDeleted();
	// sent by the track that was moved, deletes TrackGroup if there are no tracks remaining inside
	void trackMoved();
	void contextMenu(QPoint);

protected:
	void dragEnterEvent(QDragEnterEvent*) override;
	// check which TrackWidget sent the event, moves that TrackWidget into this TrackGroup
	void dropEvent(QDropEvent*) override;
	// ignores event if it is modified (so that gvw can zoom/out)
	void wheelEvent(QWheelEvent* event) override;

private:
	QVBoxLayout* layout_;
	QWidget* content_widget_;
	QPointer<TrackWidget> cur_context_track_ = nullptr;

	// adds track widgets to TrackGroup, called by loadTracksFromFile or the static function fromFile
	void addTrackWidgets(TrackWidgetList widgets);
	// gives the index of the track on top of which the drop happend
	inline int getDropIndex(int y);
	// gives the TrackWidget which is at the specified pos, if none this returns nullptr
	TrackWidget* getTrackUnderMouse(QPoint pos);
	// opens a FileDialogue and creates TrackWidgets using FileLoader
	static TrackWidgetList loadTrackWidgetsFromFile();

private slots:
	// removes all tracks inside TrackGroup and deletes them
	void clearLayout();
	// delets all the tracks inside TrackGroup and deletes the TrackGroup
	void clearLayoutAndDelete();
	// opens the FileDialogue and loads tracks from it into the TrackGroup
	void loadTracksFromFile();
};


#endif // TRACKGROUP_H
