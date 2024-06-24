#include "CircosPlotWidget.h"
#include "ui_CircosPlotWidget.h"
#include "VersatileFile.h"
#include <QMessageBox>
#include <QMenu>
#include <QClipboard>

CircosPlotWidget::CircosPlotWidget(QString filename, QWidget *parent)
	: QWidget(parent)
	, ui_(new Ui::CircosPlotWidget)
{
	ui_->setupUi(this);
	connect(ui_->imageLabel, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

	loadCircosPlot(filename);
}

CircosPlotWidget::~CircosPlotWidget()
{
	delete ui_;
}

void CircosPlotWidget::showContextMenu(QPoint pos)
{
	QMenu menu;
	QAction* a_copy = menu.addAction(QIcon(":/Icons/Clipboard.png"), "copy to clipboard");

	QAction* action = menu.exec(ui_->imageLabel->mapToGlobal(pos));
	if (action==nullptr) return;

	if (action==a_copy)
	{
		QApplication::clipboard()->setPixmap(image_);
	}
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

