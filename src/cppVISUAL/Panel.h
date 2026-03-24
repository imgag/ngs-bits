#ifndef PANEL_H
#define PANEL_H

#include "cppVISUAL_global.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMouseEvent>
#include <QPoint>

class CPPVISUALSHARED_EXPORT Panel
	: public QScrollArea
{
	Q_OBJECT

public:
	explicit Panel(QWidget* = nullptr);
	bool loadFile();

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


#endif // PANEL_H
