#include "SubpanelDesignDialog.h"
#include "ui_SubpanelDesignDialog.h"
#include "NGSD.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Helper.h"
#include "NGSHelper.h"
#include <QPushButton>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>

SubpanelDesignDialog::SubpanelDesignDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::SubpanelDesignDialog)
	, last_created_subpanel("")
{
	ui->setupUi(this);
	createSubpanelCompleter();

	connect(ui->check, SIGNAL(clicked()), this, SLOT(checkAndCreatePanel()));
	connect(ui->store, SIGNAL(clicked()), this, SLOT(storePanel()));
	connect(ui->import_btn, SIGNAL(clicked()), this, SLOT(importFromExistingSubpanel()));

	connect(ui->name, SIGNAL(textEdited(QString)), this, SLOT(disableStoreButton()));
	connect(ui->genes, SIGNAL(textChanged()), this, SLOT(disableStoreButton()));
}

SubpanelDesignDialog::~SubpanelDesignDialog()
{
	delete ui;
}

void SubpanelDesignDialog::setGenes(const GeneSet& genes)
{
	ui->genes->clear();
	ui->genes->setPlainText(genes.join("\n"));
}

QString SubpanelDesignDialog::lastCreatedSubPanel()
{
	return last_created_subpanel;
}


QStringList SubpanelDesignDialog::subpanelList()
{
	QStringList names;

	QStringList tmp = Helper::findFiles(NGSD::getTargetFilePath(true), "*.bed", false);
	foreach(QString t, tmp)
	{
		if(t.endsWith("_amplicons.bed")) continue;

		names.append(QFileInfo(t).fileName().replace(".bed", ""));
	}

	return names;
}

void SubpanelDesignDialog::createSubpanelCompleter()
{
	completer = new QCompleter(subpanelList());
	ui->name->setCompleter(completer);
}

void SubpanelDesignDialog::checkAndCreatePanel()
{
	//clear
	ui->store->setEnabled(false);
	ui->messages->clear();

	//name check name
	QString name = ui->name->text().trimmed();
	if (!QRegExp("[0-9a-zA-Z_\\.]+").exactMatch(name))
	{
		showMessage("Name '" + name + "' is empty or contains invalid characters!", true);
		return;
	}

	//check output path does not exist
	QString bed_file = getBedFilename();
	if (QFile::exists(bed_file))
	{
		showMessage("Output file '" + bed_file + "' already exists!", true);
		return;
	}
	QString bed_file_archive = getBedFilenameArchive();
	if (QFile::exists(bed_file_archive))
	{
		showMessage("Output file '" + bed_file_archive + "' already exists!", true);
		return;
	}

	//check gene names
	NGSD db;
	genes.clear();
	QByteArrayList lines = ui->genes->toPlainText().toLatin1().split('\n');
	foreach(QByteArray line, lines)
	{
		int tab_idx = line.indexOf("\t");
		if (tab_idx==-1)
		{
			genes.insert(line);
		}
		else
		{
			genes.insert(line.left(tab_idx));
		}
	}
	if (genes.count()==0)
	{
		showMessage("Genes are not set!", true);
		return;
	}
	foreach(QString gene, genes)
	{
		QPair<QString, QString> geneinfo = db.geneToApprovedWithMessage(gene);
		if (geneinfo.first!=gene || geneinfo.second.startsWith("ERROR"))
		{
			showMessage("Gene " + geneinfo.first + ": " + geneinfo.second, true);
			return;
		}
	}

	//check that BED and genes file are writable
	roi_file = getBedFilename();
	if (!Helper::isWritable(roi_file))
	{
		showMessage("Region file '" + roi_file + "' is not writable!", true);
		return;
	}
	gene_file = roi_file.left(roi_file.size()-4) + "_genes.txt";
	if (!Helper::isWritable(gene_file))
	{
		showMessage("Genes file '" + gene_file + "' is not writable!", true);
		return;
	}

	//create target region
	QString messages;
	QTextStream stream(&messages);
	regions = db.genesToRegions(genes, Transcript::CCDS, "exon", ui->fallback->isChecked(), false, &stream);
	if (messages!="")
	{
		showMessage(messages, true);
		return;
	}
	int flanking  = ui->flanking->currentText().toInt();
	if (flanking>0)
	{
		regions.extend(flanking);
	}
	regions.merge();
	showMessage("Sub-panel with " + QString::number(genes.count()) + " genes of size " + QString::number(regions.baseCount()) + " bp (exons plus " + ui->flanking->currentText() + " flanking bases) designed. You can store it now!", false);

	ui->store->setEnabled(true);
}

void SubpanelDesignDialog::storePanel()
{
	regions.store(roi_file);
	genes.store(gene_file);

	showMessage("Sub-panel '" + ui->name->text().trimmed() +"' written successfully!", false);
	ui->store->setEnabled(false);

	last_created_subpanel = roi_file;
}

void SubpanelDesignDialog::disableStoreButton()
{
	ui->store->setEnabled(false);
}

void SubpanelDesignDialog::importFromExistingSubpanel()
{
	bool ok;
	QString selected = QInputDialog::getItem(this, "Import data from existing sub-panel", "source sub-panel:", subpanelList(), 0, false, &ok);
	if (!ok) return;

	//set name
	ui->name->setText(selected);

	//set genes
	QString filename = NGSD::getTargetFilePath(true) + "/" + selected + "_genes.txt";
	setGenes(GeneSet::createFromFile(filename));
}

QString SubpanelDesignDialog::getBedFilename()
{
	return NGSD::getTargetFilePath(true) + ui->name->text() + ".bed";
}

QString SubpanelDesignDialog::getBedFilenameArchive()
{
	return NGSD::getTargetFilePath(true) + "/archive/" + ui->name->text() + ".bed";
}

void SubpanelDesignDialog::showMessage(QString message, bool error)
{
	if (error)
	{
		message = "<font color='red'>" + message + "</font>";
	}

	ui->messages->setText(message);
}
