#include "PRSView.h"
#include "ui_PRSView.h"

PRSView::PRSView(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::PRSView)
{
	ui->setupUi(this);
}

PRSView::~PRSView()
{
	delete ui;
}
