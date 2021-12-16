#include "FusionWidget.h"
#include "ui_FusionWidget.h"

FusionWidget::FusionWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::FusionWidget)
{
	ui->setupUi(this);
}

FusionWidget::~FusionWidget()
{
	delete ui;
}
