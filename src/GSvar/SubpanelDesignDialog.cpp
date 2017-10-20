#include "SubpanelDesignDialog.h"
#include "ui_SubpanelDesignDialog.h"
#include "NGSD.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Helper.h"
#include "NGSHelper.h"
#include <QDebug>
#include <QPushButton>
#include <QFileInfo>
#include <QMessageBox>

SubpanelDesignDialog::SubpanelDesignDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::SubpanelDesignDialog)
	, last_created_subpanel("")
{
	ui->setupUi(this);
	createSubpanelCompleter();

	connect(ui->check, SIGNAL(pressed()), this, SLOT(checkAndCreatePanel()));
	connect(ui->store, SIGNAL(pressed()), this, SLOT(storePanel()));

	connect(ui->name, SIGNAL(textEdited(QString)), this, SLOT(disableStoreButton()));
	connect(ui->genes, SIGNAL(textChanged()), this, SLOT(disableStoreButton()));
}

SubpanelDesignDialog::~SubpanelDesignDialog()
{
	delete ui;
}

void SubpanelDesignDialog::setGenes(QStringList genes)
{
	ui->genes->clear();
	ui->genes->setPlainText(genes.join("\n"));
}

QString SubpanelDesignDialog::lastCreatedSubPanel()
{
	return last_created_subpanel;
}

void SubpanelDesignDialog::createSubpanelCompleter()
{
	QStringList tmp = Helper::findFiles(NGSD::getTargetFilePath(true), "*.bed", false);
	QStringList names;
	foreach(QString t, tmp)
	{
		if(t.endsWith("_amplicons.bed")) continue;

		QString name = QFileInfo(t).fileName().replace(".bed", "");
		names.append(name);
	}

	completer = new QCompleter(names);
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
	genes = GeneSet::createFromText(ui->genes->toPlainText().toLatin1());
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
