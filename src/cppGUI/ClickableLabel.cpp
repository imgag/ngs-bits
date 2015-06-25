#include "ClickableLabel.h"

ClickableLabel::ClickableLabel(QWidget* parent)
	: QLabel(parent)
{
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent* event)
{
	emit clicked(event->pos());
}
