#include "PhenoToRoiDialog.h"
#include "ui_PhenotoRoiDialog.h"

PhenoToRoiDialog::PhenoToRoiDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PhenotoRoiDialog)
{
	ui->setupUi(this);
}

PhenoToRoiDialog::~PhenoToRoiDialog()
{
	delete ui;
}
