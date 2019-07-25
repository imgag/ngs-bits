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
#include <GUIHelper.h>

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
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	ui->name->setCompleter(completer);
}

void SubpanelDesignDialog::checkAndCreatePanel()
{
	//clear
	disableStoreButton();
	clearMessages();

	//name check name
	QString basename = ui->name->text().trimmed();
	if (basename.isEmpty() || !QRegExp("[0-9a-zA-Z_\\.]+").exactMatch(basename))
	{
		addMessage("Name '" + basename + "' is empty or contains invalid characters!", true, true);
		return;
	}

	//check output path does not exist
	QString bed_file = getBedFilename();
	if (QFile::exists(bed_file))
	{
		addMessage("Output file '" + bed_file + "' already exists!", true, true);
		return;
	}
	QString bed_file_archive = getBedFilenameArchive();
	if (QFile::exists(bed_file_archive))
	{
		addMessage("Output file '" + bed_file_archive + "' already exists!", true, true);
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
		addMessage("Genes are not set!", true, true);
		return;
	}
	bool ignore_gene_errors = ui->ignore_gene_errors->isChecked();
	foreach(QString gene, genes)
	{
		QPair<QString, QString> geneinfo = db.geneToApprovedWithMessage(gene);
		if (geneinfo.first!=gene || geneinfo.second.startsWith("ERROR"))
		{
			addMessage("Gene " + geneinfo.first + ": " + geneinfo.second, !ignore_gene_errors, false);
		}
	}

	//check that BED and genes file are writable
	roi_file = getBedFilename();
	if (!Helper::isWritable(roi_file))
	{
		addMessage("Region file '" + roi_file + "' is not writable!", true, true);
		return;
	}
	gene_file = roi_file.left(roi_file.size()-4) + "_genes.txt";
	if (!Helper::isWritable(gene_file))
	{
		addMessage("Genes file '" + gene_file + "' is not writable!", true, true);
		return;
	}

	//create target region
	QString mode = ui->mode->currentText();
	QString messages;
	QTextStream stream(&messages);
	regions = db.genesToRegions(genes, Transcript::ENSEMBL, mode, ui->fallback->isChecked(), false, &stream);
	if (messages!="")
	{
		foreach(QString message, messages.split('\n'))
		{
			addMessage(message.trimmed(), !ignore_gene_errors, false);
		}
	}

	//add flanking regions
	int flanking  = ui->flanking->currentText().toInt();
	if (flanking>0)
	{
		regions.extend(flanking);
	}

	//add special regions (gene symbol, region1, region2, ...)
	QStringList genes_special;
	QStringList special_regions = Settings::stringList("subpanel_special_regions");
	foreach(QString line, special_regions)
	{
		line = line.trimmed();
		QByteArrayList parts = line.toLatin1().split('\t');
		QByteArray gene_symbol = parts[0];
		if (genes.contains(gene_symbol))
		{
			genes_special << gene_symbol;
			for (int i=1; i<parts.count(); ++i)
			{
				regions.append(BedLine::fromString(parts[i]));
			}
		}
	}
	regions.merge();

	//show message
	addMessage("Sub-panel with " + QString::number(genes.count()) + " genes of size " + QString::number(regions.baseCount()) + " bp (" + mode + " plus " + ui->flanking->currentText() + " flanking bases) designed. You can store it now!", false, true);
	if (!genes_special.isEmpty())
	{
		addMessage("Added special regions for gene(s): " + genes_special.join(", "), false, true);
	}

	ui->store->setEnabled(!errorMessagesPresent());
}

void SubpanelDesignDialog::storePanel()
{
	regions.store(roi_file);
	genes.store(gene_file);

	clearMessages();
	addMessage("Sub-panel '" + QFileInfo(roi_file).baseName() + "' written successfully!", false, true);
	disableStoreButton();

	last_created_subpanel = roi_file;
}

void SubpanelDesignDialog::disableStoreButton()
{
	ui->store->setEnabled(false);
}

void SubpanelDesignDialog::importFromExistingSubpanel()
{
	//show selection dialog
	QLineEdit* panels = new QLineEdit(this);
	panels->setMinimumWidth(400);
	panels->setCompleter(completer);
	auto dlg = GUIHelper::createDialog(panels, "Import data from existing sub-panel", "source sub-panel:", true);
	if(dlg->exec()==QDialog::Accepted)
	{
		QString selected = panels->text();
		if (subpanelList().contains(selected))
		{
			//set base name (remove auto-suffix when present)
			QString basename = selected;
			if (basename.count('_')>=3)
			{
				QStringList parts = basename.split('_');
				QString first_suffix_part = parts[parts.count()-3];
				for (int i=0; i<ui->mode->count(); ++i)
				{
					if (QRegExp(ui->mode->itemText(i) + "[0-9]+").exactMatch(first_suffix_part))
					{
						basename = parts.mid(0, parts.count()-3).join("_");
						break;
					}
				}
			}

			ui->name->setText(basename);

			//set genes
			QString filename = NGSD::getTargetFilePath(true) + "/" + selected + "_genes.txt";
			setGenes(GeneSet::createFromFile(filename));
		}
		else
		{
			QMessageBox::warning(this, "Invalid sub-panel", "Please select an existing sub-panel!\n'" + selected + "' is not valid");
		}
	}

	delete panels;
}

QString SubpanelDesignDialog::getBedFilename() const
{
	return NGSD::getTargetFilePath(true) + ui->name->text() + getBedSuffix();
}

QString SubpanelDesignDialog::getBedFilenameArchive() const
{
	return NGSD::getTargetFilePath(true) + "/archive/" + ui->name->text() + getBedSuffix();
}

QString SubpanelDesignDialog::getBedSuffix() const
{
	return 	"_" + ui->mode->currentText() + ui->flanking->currentText() + "_" + Helper::userName() + "_" + QDate::currentDate().toString("yyyyMMdd") + ".bed";
}

void SubpanelDesignDialog::clearMessages()
{
	messages.clear();
	ui->messages->clear();
}

void SubpanelDesignDialog::addMessage(QString text, bool is_error, bool update_gui)
{
	messages << Message{text, is_error};

	if (update_gui)
	{
		QString text;
		foreach(const Message& m, messages)
		{
			if (!text.isEmpty()) text += "<br>";
			text += m.is_error ? "<font color='red'>" + m.text + "</font>" : m.text;
		}
		ui->messages->setHtml(text);
	}
}

bool SubpanelDesignDialog::errorMessagesPresent()
{
	foreach(const Message& m, messages)
	{
		if (m.is_error) return true;
	}

	return false;
}
