#ifndef BEDPANEL_H
#define BEDPANEL_H

#include "cppVISUAL_global.h"
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
	void tracksChanged();

private:
	QVBoxLayout* layout_;
	void clearLayout();
};


#endif // BEDPANEL_H
