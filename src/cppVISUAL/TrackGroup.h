#ifndef TRACKGROUP_H
#define TRACKGROUP_H

#include "cppVISUAL_global.h"

#include <QDomElement>
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMouseEvent>
#include <QPoint>
#include <QXmlStreamWriter>

class CPPVISUALSHARED_EXPORT TrackGroup
	: public QScrollArea
{
	Q_OBJECT

public:
	explicit TrackGroup(QWidget* = nullptr);
	bool loadFile();
	void reloadTracks();
	void writeToXml(QXmlStreamWriter&);
	void loadFromXml(const QDomElement&);

signals:
	void addPanelAbove();
	void addPanelBelow();

public slots:
	void trackDeleted();
	void trackMoved();
	void contextMenu(QPoint);

private:
	QVBoxLayout* layout_;
	QWidget* content_widget_;

	void clearLayout();
	void clearLayoutAndDelete();
	void removeTrack(QWidget*);

	void dragEnterEvent(QDragEnterEvent*) override;
	void dropEvent(QDropEvent*) override;

	inline int getDropIndex(int y);
};


#endif // TRACKGROUP_H
