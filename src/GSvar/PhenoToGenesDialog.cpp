#include "PhenoToGenesDialog.h"
#include "Helper.h"
#include "PhenotypeSettingsDialog.h"
#include <QClipboard>
#include <QFileDialog>
#include <QMenu>

PhenoToGenesDialog::PhenoToGenesDialog(QWidget *parent)
	: QDialog(parent)
	, ui()
	, phenotype_settings_()
{
	ui.setupUi(this);

	connect(ui.tabs, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
	connect(ui.store_btn, SIGNAL(pressed()), this, SLOT(storeGenesAsTSV()));
	connect(ui.settings_btn, SIGNAL(pressed()), this, SLOT(showSettings()));

	QMenu* menu = new QMenu();
	menu->addAction("tab-separated table", this, SLOT(copyGenesToClipboardAsTable()));
	menu->addAction("comma-separated gene list", this, SLOT(copyGenesToClipboardAsList()));
	ui.clip_btn->setMenu(menu);
}

void PhenoToGenesDialog::copyGenesToClipboardAsTable()
{
	QApplication::clipboard()->setText(ui.genes->toPlainText());
}

void PhenoToGenesDialog::copyGenesToClipboardAsList()
{
	QStringList output;

	QStringList lines = ui.genes->toPlainText().split("\n");
	foreach(QString line, lines)
	{
		line = line.trimmed();
		if (line.isEmpty()) continue;

		QStringList parts = line.split("\t");
		output.append(parts[0]);
	}

	QApplication::clipboard()->setText(output.join(", "));
}

void PhenoToGenesDialog::storeGenesAsTSV()
{
	QString filename = QFileDialog::getSaveFileName(this, "Store genes as TSV file", "", "TSV files (*.tsv);;TXT files (*.txt);;All files (*.*)");
	if (filename.isEmpty()) return;

	auto file_prt = Helper::openFileForWriting(filename);
	file_prt->write(ui.genes->toPlainText().toUtf8());
}

void PhenoToGenesDialog::showSettings()
{
	PhenotypeSettingsDialog dlg(this);
	dlg.setCombinationModeEnabled(false);
	dlg.set(phenotype_settings_);

	//update
	if (dlg.exec()==QDialog::Accepted)
	{
		phenotype_settings_ = dlg.get();
		emit tabChanged(1);
	}
}

void PhenoToGenesDialog::tabChanged(int num)
{
	//update genes
	if (num==1)
	{
		NGSD db;
		//get gene list
		int max_phenotypes = 0;
		QMap<QByteArray, QStringList> gene2pheno;
		PhenotypeList phenos = ui.pheno_selector->selectedPhenotypes();
		for (int i=0; i<phenos.count(); ++i)
		{
			GeneSet genes = db.phenotypeToGenesbySourceAndEvidence(db.phenotypeIdByAccession(phenos[i].accession()), phenotype_settings_.sources, phenotype_settings_.evidence_levels, true, false);
            for (const QByteArray& gene : genes)
			{
				gene2pheno[gene].append(phenos[i].name());
                max_phenotypes = std::max(SIZE_TO_INT(max_phenotypes), SIZE_TO_INT(gene2pheno[gene].count()));
			}
		}

		//update view
		ui.genes->clear();
		for (int hits = max_phenotypes; hits>0; --hits)
		{
			auto it = gene2pheno.begin();
			while(it!=gene2pheno.end())
			{
				int count_phenos = it.value().count();
				if (count_phenos==hits)
				{
					ui.genes->append(it.key() + "\t" + it.value().join(", ") + "\t" + QString::number(count_phenos));
				}
				++it;
			}
		}
		ui.genes->moveCursor(QTextCursor::Start);
	}

	//update buttons
	ui.clip_btn->setEnabled(num==1);
	ui.store_btn->setEnabled(num==1);
}
