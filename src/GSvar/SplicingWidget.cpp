#include "SplicingWidget.h"
#include "ui_SplicingWidget.h"

SplicingWidget::SplicingWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SplicingWidget)
{
	ui->setupUi(this);
}

SplicingWidget::~SplicingWidget()
{
	delete ui;
}
