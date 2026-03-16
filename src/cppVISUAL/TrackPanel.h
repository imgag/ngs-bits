#ifndef TRACKPANEL_H
#define TRACKPANEL_H


#include "cppVISUAL_global.h"
#include "Track.h"
#include "ChromosomalIndex.h"
#include <QWidget>


class CPPVISUALSHARED_EXPORT TrackPanel
	: public QWidget
{
	Q_OBJECT

public:
	TrackPanel(QWidget* parent, Track tack);

public slots:
	void regionChanged();

private:
	Track track;
	void paintEvent(QPaintEvent* event) override;
	ChromosomalIndex<BedFile> chrIdx;
};

#endif // TRACKPANEL_H
