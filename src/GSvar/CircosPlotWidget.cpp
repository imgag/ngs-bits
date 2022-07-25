#include "CircosPlotWidget.h"
#include "ui_CircosPlotWidget.h"
#include "VersatileFile.h"
#include <QMessageBox>

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
	VersatileFile file(filename);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(this, "Read error", "Could not open a circos plot image file: '" + filename);
		return;
	}
	image_.loadFromData(file.readAll());

	// display plot
	ui_->imageLabel->setPixmap(image_);
}

