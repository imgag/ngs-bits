#include "ClinVarUploadDialog.h"
#include "ui_ClinVarUploadDialog.h"

ClinvarUploadDialog::ClinvarUploadDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ClinvarUploadDialog)
{
	ui->setupUi(this);
}

ClinvarUploadDialog::~ClinvarUploadDialog()
{
	delete ui;
}
