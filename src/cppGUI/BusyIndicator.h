#ifndef BUSYINDICATOR_H
#define BUSYINDICATOR_H

#include "cppGUI_global.h"
#include <QLabel>
#include <QMovie>

///Busy indicator label - a rotating circle.
class CPPGUISHARED_EXPORT BusyIndicator
		: public QLabel
{
	Q_OBJECT

public:
	BusyIndicator(QWidget* parent = 0);
	~BusyIndicator();

protected:
	QMovie* movie_;
};

#endif // BUSYINDICATOR_H
