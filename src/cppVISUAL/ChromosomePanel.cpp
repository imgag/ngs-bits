#include "ChromosomePanel.h"
#include "SharedData.h"
#include <QPainter>
#include <QMouseEvent>

ChromosomePanel::ChromosomePanel(QWidget* parent)
	: QWidget(parent)
{
	setMouseTracking(true);
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(updateRegion()));
}

void ChromosomePanel::updateRegion()
{
	update();
}

void ChromosomePanel::paintEvent(QPaintEvent* /*event*/)
{
	//init
	int w = width();
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();

	pixels_per_base_ = (double)(w-label_width-4) / (double)region.length();

	QPainter painter(this);
	painter.fillRect(rect(), Qt::red);

	//bands:
	//NGSHelper::cytoBands
	//TODO add band colors
}

void ChromosomePanel::mouseMoveEvent(QMouseEvent* event)
{
	//init
	int x = event->pos().x();
	int w = width();
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();

	//show
	if (x>label_width + 2 && x<w - 2)
	{
		int coordinate = region.start() + std::floor((double)(x-label_width - 2) / pixels_per_base_);
		emit mouseCoordinate(region.chr().strNormalized(true) + ":" + QString::number(coordinate));
	}
	else
	{
		emit mouseCoordinate("");
	}
}
