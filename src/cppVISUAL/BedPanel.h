#ifndef BEDPANEL_H
#define BEDPANEL_H

#include "cppVISUAL_global.h"
#include "Track.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QMouseEvent>

class CPPVISUALSHARED_EXPORT BedPanel
	: public QWidget
{
	Q_OBJECT

public:
	BedPanel(QWidget*);

public slots:
	void trackAdded(Track);
	void trackDeleted();

private:
	QVBoxLayout* layout_;
	void clearLayout();
};


#endif // BEDPANEL_H
