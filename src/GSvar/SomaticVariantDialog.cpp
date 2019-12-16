#include "SomaticVariantDialog.h"
#include "ui_SomaticVariantDialog.h"

SomaticVariantDialog::SomaticVariantDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SomaticVariantDialog)
{
	ui->setupUi(this);
}

SomaticVariantDialog::~SomaticVariantDialog()
{
	delete ui;
}
