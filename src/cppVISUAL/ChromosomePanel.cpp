#include "ChromosomePanel.h"


ChromosomePanel::ChromosomePanel(QWidget* parent)
	: QWidget(parent)
{
}

void ChromosomePanel::setRegion(const BedLine& region)
{
	reg_ = region;
	update();
}

void ChromosomePanel::paintEvent(QPaintEvent* event)
{
}

void ChromosomePanel::mouseMoveEvent(QMouseEvent* event)
{
}
