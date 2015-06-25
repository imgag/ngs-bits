#ifndef COLORSELECTOR_H
#define COLORSELECTOR_H

#include "cppGUI_global.h"
#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>

class CPPGUISHARED_EXPORT ColorSelector
		: public QWidget
{
	Q_OBJECT

public:
	ColorSelector(QWidget* parent = 0);

	QColor getColor();
	void setColor(QColor color);

	QSize sizeHint () const;

signals:
	void valueChanged(QColor);

protected:
	void paintEvent(QPaintEvent* e);
	void mousePressEvent(QMouseEvent* e);

	QColor color_;
};

#endif
