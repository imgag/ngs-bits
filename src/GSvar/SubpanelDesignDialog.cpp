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
	loadProcessingSystems();
	createSubpanelCompleter();

	connect(ui->check, SIGNAL(pressed()), this, SLOT(checkAndCreatePanel()));
	connect(ui->store, SIGNAL(pressed()), this, SLOT(storePanel()));

	connect(ui->name, SIGNAL(textEdited(QString)), this, SLOT(disableStoreButton()));
	connect(ui->base_panel, SIGNAL(currentTextChanged(QString)), this, SLOT(disableStoreButton()));
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

void SubpanelDesignDialog::loadProcessingSystems()
{
	ui->base_panel->addItem("<select>");

	QMap<QString, QString> systems = NGSD().getProcessingSystems(true, true);
	auto it = systems.constBegin();
	while (it != systems.constEnd())
	{
		ui->base_panel->addItem(it.key(), Helper::canonicalPath(it.value()));
		++it;
	}
}

void SubpanelDesignDialog::createSubpanelCompleter()
{
	QStringList tmp = Helper::findFiles(NGSD::getTargetFilePath(true), "*.bed", false);
	QStringList names;
	foreach(QString t, tmp)
	{
		QString name = QFileInfo(t).fileName();
		name = name.left(name.size()-4);
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
	genes = NGSHelper::textToGenes(ui->genes->toPlainText());
	if (genes.size()==0)
	{
		showMessage("Genes are not set!", true);
		return;
	}
	foreach(QString gene, genes)
	{
		QPair<QString, QString> geneinfo = db.geneToApproved(gene);
		if (geneinfo.first!=gene || geneinfo.second.contains("ERROR"))
		{
			showMessage(geneinfo.second, true);
			return;
		}
	}

	//check that genes are contained on base panel
	QString base_panel_file = ui->base_panel->currentData().toString();
	if (base_panel_file!="")
	{
		base_panel_file = base_panel_file.left(base_panel_file.size()-4) + "_genes.txt";
		if (!QFile::exists(base_panel_file))
		{
			showMessage("Base panel gene file " + base_panel_file + " does not exist!", true);
			return;
		}
		QStringList base_genes = Helper::loadTextFile(base_panel_file, true, '#', true);
		foreach (QString g, genes)
		{
			if (!base_genes.contains(g))
			{
				showMessage("Gene '" + g + "' is not part of the base panel!", true);
				return;
			}
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
	regions = db.genesToRegions(genes, Transcript::CCDS, "exon", ui->fallback->isChecked(), &stream);
	if (messages!="")
	{
		showMessage(messages, true);
		return;
	}
	regions.extend(ui->flanking->currentText().toInt());
	regions.merge();
	showMessage("Sub-panel with " + QString::number(genes.count()) + " genes of size " + QString::number(regions.baseCount()) + " bp (exons plus " + ui->flanking->currentText() + " flanking bases) designed. You can store it now!", false);

	ui->store->setEnabled(true);
}

void SubpanelDesignDialog::storePanel()
{
	regions.store(roi_file);
	Helper::storeTextFile(gene_file, genes);

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
