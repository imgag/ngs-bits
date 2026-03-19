#ifndef PANEL_H
#define PANEL_H

#include "cppVISUAL_global.h"
#include "Track.h"

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
	Panel(QWidget*);

public slots:
	void trackAdded(Track);
	void trackDeleted();
	void contextMenu(QPoint);

private:
	QVBoxLayout* layout_;
	QWidget* content_widget_;
	void clearLayout();
};


#endif // PANEL_H
