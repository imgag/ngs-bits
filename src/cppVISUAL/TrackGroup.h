#ifndef TRACKGROUP_H
#define TRACKGROUP_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include <QtXml/QDomElement>
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>
#include <QXmlStreamWriter>

// A TrackGroup containing TrackWidgets.
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
	void writeToXml(QXmlStreamWriter& writer);
	//Reads Track elements from session XML
	void loadFromXml(const QDomElement& dom_element, QStringList& errors);
	//Creates a track group from a file
	static TrackGroup* fromFile(QString filename);
	// reads <Track> elements in the dom and creates Tracks based on the properties, if no Track elements were created, returns null ptr
	static TrackGroup* fromXml(const QDomElement& dom_element, QStringList& errors);
	//returns the number of tracks
	int trackCount();

signals:
	void addTrackGroupAbove();
	void addTrackGroupBelow();
	void trackSelected(TrackWidget* track);

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
	void addTrackWidgets(QVector<TrackWidget*> widgets);
	void connectTrackSignals(TrackWidget* track);
	// gives the index of the track on top of which the drop happend
	inline int getDropIndex(int y);
	// gives the TrackWidget which is at the specified pos, if none this returns nullptr
	TrackWidget* getTrackUnderMouse(QPoint pos);

private slots:
	// removes all tracks inside TrackGroup and deletes them
	void clearLayout();
	// delets all the tracks inside TrackGroup and deletes the TrackGroup
	void clearLayoutAndDelete();
	// opens the FileDialogue and loads tracks from it into the TrackGroup
	void loadTracksFromFile();
	//Asks for a URL and loads its tracks into this group.
	void loadTracksFromUrl();
};


#endif // TRACKGROUP_H
