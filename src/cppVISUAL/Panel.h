#ifndef PANEL_H
#define PANEL_H

#include "cppVISUAL_global.h"
#include "Track.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QMouseEvent>

class CPPVISUALSHARED_EXPORT Panel
	: public QWidget
{
	Q_OBJECT

public:
	Panel(QWidget*);

public slots:
	void trackAdded(Track);
	void trackDeleted();

private:
	QVBoxLayout* layout_;
	void clearLayout();
};


#endif // PANEL_H
