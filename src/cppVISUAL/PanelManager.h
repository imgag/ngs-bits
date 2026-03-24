#ifndef PANELMANAGER_H
#define PANELMANAGER_H

#include "BedFile.h"
#include "TrackData.h"

#include <QSplitter>

class PanelManager : public QSplitter
{
	Q_OBJECT
public:
	PanelManager(QWidget* parent =nullptr);

	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;

	bool is_dragging_;
	int drag_start_x_;
	BedLine drag_start_region_;

public slots:
	void loadFile();
};



#endif // PANELMANAGER_H
