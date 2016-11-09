#include "PhenoToGenesDialog.h"
#include "Helper.h"
#include "ui_PhenoToGenesDialog.h"
#include <QDebug>
#include <QClipboard>
#include <QFileDialog>

PhenoToGenesDialog::PhenoToGenesDialog(QWidget *parent) :
	QDialog(parent),
        ui(new Ui::PhenoToGenesDialog)
{
	ui->setupUi(this);
	ui->pheno_sel->setDetailsWidget(ui->pheno_details);
	connect(ui->pheno_sel, SIGNAL(phenotypeActivated(QString)), this, SLOT(copyPhenotype(QString)));
	connect(ui->pheno, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(deletePhenotype(QListWidgetItem*)));
	connect(ui->tabs, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

	connect(ui->clip_btn, SIGNAL(pressed()), this, SLOT(copyGenesToClipboard()));
	connect(ui->store_btn, SIGNAL(pressed()), this, SLOT(storeGenesAsTSV()));
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

void PhenoToGenesDialog::copyGenesToClipboard()
{
	QApplication::clipboard()->setText(ui->genes->toPlainText());
}

void PhenoToGenesDialog::storeGenesAsTSV()
{
	QString filename = QFileDialog::getSaveFileName(this, "Store genes as TSV file", "", "TSV files (*.tsv);;TXT files (*.txt);;All files (*.*)");
	if (filename.isEmpty()) return;

	auto file_prt = Helper::openFileForWriting(filename);
	file_prt->write(ui->genes->toPlainText().toLatin1());
}

void PhenoToGenesDialog::tabChanged(int num)
{
	//update genes
	if (num==1)
	{
		NGSD db;
		//get gene list
		int max_phenotypes = 0;
		QMap<QString, QStringList> gene2pheno;
		for (int i=0; i<ui->pheno->count(); ++i)
		{
			QString pheno = ui->pheno->item(i)->text();
			QStringList genes = db.phenotypeToGenes(pheno, true);
			foreach(QString gene, genes)
			{
				gene2pheno[gene].append(pheno);
				max_phenotypes = std::max(max_phenotypes, gene2pheno[gene].count());
			}
		}

		//update view
		ui->genes->clear();
		for (int hits = max_phenotypes; hits>0; --hits)
		{
			auto it = gene2pheno.begin();
			while(it!=gene2pheno.end())
			{
				int count_phenos = it.value().count();
				if (count_phenos==hits)
				{
					ui->genes->append(it.key() + "\t" + it.value().join(", ") + "\t" + QString::number(count_phenos));
				}
				++it;
			}
		}
		ui->genes->moveCursor(QTextCursor::Start);
	}

	//update buttons
	ui->clip_btn->setEnabled(num==1);
	ui->store_btn->setEnabled(num==1);
}
