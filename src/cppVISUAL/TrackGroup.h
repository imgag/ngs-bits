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

signals:
	void addPanelAbove();
	void addPanelBelow();

public slots:
	// sent by the track that was deleted, deletes TrackGroup if there are no tracks remaining inside
	void trackDeleted();
	// sent by the track that was moved, deletes TrackGroup if there are no tracks remaining inside
	void trackMoved();
	void contextMenu(QPoint);

private:
	QVBoxLayout* layout_;
	QWidget* content_widget_;
	QPointer<TrackWidget> cur_context_track_ = nullptr;

	// removes all tracks inside TrackGroup and deletes them
	void clearLayout();
	// delets all the tracks inside TrackGroup and deletes the TrackGroup
	void clearLayoutAndDelete();
	// opens the FileDialogue and loads tracks from it into the TrackGroup
	void loadTracksFromFile();
	// adds track widgets to TrackGroup, called by loadTracksFromFile or the static function fromFile
	void addTrackWidgets(TrackWidgetList widgets);

	void dragEnterEvent(QDragEnterEvent*) override;
	void dropEvent(QDropEvent*) override;

	// gives the index of the track on top of which the drop happend
	inline int getDropIndex(int y);
	// gives the TrackWidget which is at the specified pos, if none this returns nullptr
	TrackWidget* getTrackUnderMouse(QPoint pos);

	// opens a FileDialogue and creates TrackWidgets using FileLoader
	static TrackWidgetList loadTrackWidgetsFromFile();
};


#endif // TRACKGROUP_H
