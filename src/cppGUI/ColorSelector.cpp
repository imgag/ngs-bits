#include <QPainter>
#include <QColorDialog>

#include "ColorSelector.h"

ColorSelector::ColorSelector(QWidget* parent)
	: QWidget(parent)
	, color_(255,255,255)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize ColorSelector::sizeHint() const
{
	return QSize(15,15);
}

void ColorSelector::paintEvent(QPaintEvent*)
{
	int size = std::min(width(),height());
	QPainter painter(this);
	painter.setPen(QColor(0,0,0));
	painter.drawRect(0, 0, size-1, size-1);
	painter.setPen(QColor(255,255,255));
	painter.drawRect(1, 1, size-3, size-3);

	painter.fillRect(2, 2, size-4, size-4 ,color_);
}

void ColorSelector::mousePressEvent(QMouseEvent* e)
{
	if (e->button()!=Qt::LeftButton)
	{
		e->ignore();
		return;
	}

	QColor tmp = QColorDialog::getColor(color_,this);
	if (tmp.isValid())
	{
		setColor(tmp);
	}
}

QColor ColorSelector::getColor()
{
	return color_;
}

void ColorSelector::setColor(QColor color)
{
	color_ = color;
	emit valueChanged(color_);
	repaint();
}
