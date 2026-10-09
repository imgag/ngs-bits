#ifndef PANELMANAGER_H
#define PANELMANAGER_H

#include "BedFile.h"
#include "TrackGroup.h"
#include "cppVISUAL_global.h"
#include <QSplitter>
#include <QtXml/QDomElement>
#include <QXmlStreamWriter>

//class for handling the operations related to panels, holds every TrackGroup inside of it
// and creates a new split for every TrackGroup
class CPPVISUALSHARED_EXPORT PanelManager:
	public QSplitter
{
	Q_OBJECT
public:
	PanelManager(QWidget* parent =nullptr);
	// calls reloadTracks for all TrackGroups
	void reloadTracks();
	//removes all track groups
	void removeAll();
	//returns if this is a empty session: only track groups, but no tracks
	bool isEmptySession() const;
	//adds an empty panel
	void addEmptyPanel();
	//resizes the gene panel to the given height. All other panels maintain their relative heights.
	void resizeGenePanel(int height=80);

	// writes current session data to xml
	void writeToXml(QXmlStreamWriter&);
	//load TrackGroups from a session XML file.
	void loadFromXml(const QDomElement&, QStringList& errors);


	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;

	bool is_dragging_ = false;
	int drag_start_x_;
	BedLine drag_start_region_;

public slots:
	//creates a TrackGroup from a file
	void loadFile(QString filename);
	// creates empty panel above the panel that emitted this signal
	void addPanelAbove();
	// creates empty panel below the panel that emitted this signal
	void addPanelBelow();

private:
	// connects addPanelAbove and addPanelBelow signals and slots to the TrackGroup
	void connectSignals(TrackGroup*);
	void updateDragRegion(int x);
};



#endif // PANELMANAGER_H
