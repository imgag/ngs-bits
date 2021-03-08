#include "CircosPlotWidget.h"
#include "ui_CircosPlotWidget.h"
#include <QFileInfo>
#include "Helper.h"
#include "GlobalServiceProvider.h"


CircosPlotWidget::CircosPlotWidget(QWidget *parent)
	: QWidget(parent)
	, ui_(new Ui::CircosPlotWidget)
{
	ui_->setupUi(this);

	//load plot file
	QList<FileLocation> plot_files = GlobalServiceProvider::instance().fileLocationProvider()->getCircosPlotFiles();
	if (plot_files.count()==1)
	{
		loadCircosPlot(plot_files[0].filename);
	}	
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

