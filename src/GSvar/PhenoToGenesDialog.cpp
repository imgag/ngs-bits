#include "PhenoToGenesDialog.h"
#include "ui_PhenoToGenesDialog.h"
#include <QDebug>

PhenoToGenesDialog::PhenoToGenesDialog(QWidget *parent) :
	QDialog(parent),
        ui(new Ui::PhenoToGenesDialog)
{
	ui->setupUi(this);
	connect(ui->pheno_sel, SIGNAL(phenotypeSelected(QString)), this, SLOT(copyPhenotype(QString)));
	connect(ui->pheno, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(deletePhenotype(QListWidgetItem*)));
	connect(ui->tabs, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
}

PhenoToGenesDialog::~PhenoToGenesDialog()
{
	delete ui;
}

void PhenoToGenesDialog::copyPhenotype(QString phenotype)
{
	//skip if already contained
	for (int i=0; i<ui->pheno->count(); ++i)
	{
		if (ui->pheno->item(i)->text()==phenotype) return;
	}

	ui->pheno->addItem(phenotype);
}

void PhenoToGenesDialog::deletePhenotype(QListWidgetItem* item)
{
	ui->pheno->takeItem(ui->pheno->row(item));
}

void PhenoToGenesDialog::tabChanged(int num)
{
	//update genes
	if (num==1)
	{
		//get gene list
		QMap<QString, QStringList> gene2pheno;
		for (int i=0; i<ui->pheno->count(); ++i)
		{
			QString pheno = ui->pheno->item(i)->text();
			QStringList genes = db_.phenotypeToGenes(pheno, true);
			foreach(QString gene, genes)
			{
				gene2pheno[gene].append(pheno);
			}
		}

		//update view
		ui->genes->clear();
		auto it = gene2pheno.begin();
		while(it!=gene2pheno.end())
		{
			ui->genes->append(it.key() + "\t" + it.value().join(", "));
			++it;
		}
	}
}
