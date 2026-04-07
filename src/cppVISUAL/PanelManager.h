#ifndef PANELMANAGER_H
#define PANELMANAGER_H

#include "BedFile.h"
#include "TrackGroup.h"
#include "cppVISUAL_global.h"

#include <QSplitter>
#include <QDomElement>
#include <QXmlStreamWriter>

class CPPVISUALSHARED_EXPORT PanelManager:
	public QSplitter
{
	Q_OBJECT
public:
	PanelManager(QWidget* parent =nullptr);
	// calls reloadTracks for all TrackGroups
	void reloadTracks();
	// deletes all TrackGroups
	void newSession();
	// writes current session data to xml
	void writeToXml(QXmlStreamWriter&);
	// loads data from Xml (Track Groups)
	void loadFromXml(const QDomElement&);

	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;

	bool is_dragging_;
	int drag_start_x_;
	BedLine drag_start_region_;

public slots:
	// opens a file dialog and creates a TrackGroup if file is valid
	void loadFile();
	// creates empty panel above the panel that emitted this signal
	void addPanelAbove();
	// creates empty panel below the panel that emitted this signal
	void addPanelBelow();

private:
	// connects addPanelAbove and addPanelBelow signals and slots to the TrackGroup
	void connectSignals(TrackGroup*);
};



#endif // PANELMANAGER_H
