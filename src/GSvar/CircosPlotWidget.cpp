#include "CircosPlotWidget.h"
#include "ui_CircosPlotWidget.h"
#include <QFileInfo>
#include "Helper.h"
#include "GlobalServiceProvider.h"


CircosPlotWidget::CircosPlotWidget(QString filename, QWidget *parent)
	: QWidget(parent)
	, ui_(new Ui::CircosPlotWidget)
{
	ui_->setupUi(this);

	loadCircosPlot(filename);
}

CircosPlotWidget::~CircosPlotWidget()
{
	delete ui_;
}

void CircosPlotWidget::resizeEvent(QResizeEvent*)
{
	// resize image
	ui_->imageLabel->setPixmap(image_.scaled(ui_->imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void CircosPlotWidget::loadCircosPlot(QString filename)
{
	// load plot file
	image_= QPixmap(filename);

	// display plot
	ui_->imageLabel->setPixmap(image_);
}

