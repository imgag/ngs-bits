#include "PhenoToRoiDialog.h"
#include "ui_PhenoToRoiDialog.h"

PhenoToRoiDialog::PhenoToRoiDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PhenoToRoiDialog)
{
	ui->setupUi(this);
	connect(ui->pheno_sel, SIGNAL(phenotypeSelected(QString)), this, SLOT(copyPhenotype(QString)));
}

PhenoToRoiDialog::~PhenoToRoiDialog()
{
	delete ui;
}

void PhenoToRoiDialog::copyPhenotype(QString phenotype)
{
	//skip if already contained
	for (int i=0; i<ui->pheno->count(); ++i)
	{
		if (ui->pheno->item(i)->text()==phenotype) return;
	}

	ui->pheno->addItem(phenotype);
}
