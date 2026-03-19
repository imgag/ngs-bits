#ifndef PANELMANAGER_H
#define PANELMANAGER_H

#include "BedFile.h"
#include "Track.h"

#include <QSplitter>

class PanelManager : public QSplitter
{
	Q_OBJECT
public:
	PanelManager(QWidget* parent =nullptr);
	/*
	 * TODO handles addition of panels
	 * if bam panel is added it needs a split
	 * this also handles the movement using the mouse
	 */

	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;

	bool is_dragging_;
	int drag_start_x_;
	BedLine drag_start_region_;

public slots:
	void trackAdded(QSharedPointer<Track>);
};



#endif // PANELMANAGER_H
