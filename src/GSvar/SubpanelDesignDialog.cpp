#include "SubpanelDesignDialog.h"
#include "ui_SubpanelDesignDialog.h"
#include "NGSD.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QDebug>
#include <QPushButton>
#include <QDesktopServices>
#include <QFileInfo>
#include <QMessageBox>

SubpanelDesignDialog::SubpanelDesignDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::SubpanelDesignDialog)
	, added_subpanel(true)
{
	ui->setupUi(this);
	loadProcessingSystems();
	createSubpanelCompleter();

	connect(ui->check, SIGNAL(pressed()), this, SLOT(checkAndCreatePanel()));
	connect(ui->store, SIGNAL(pressed()), this, SLOT(storePanel()));
	connect(ui->open_folder, SIGNAL(pressed()), this, SLOT(openSubpanelFolder()));
}

SubpanelDesignDialog::~SubpanelDesignDialog()
{
	delete ui;
}

bool SubpanelDesignDialog::addedSubpanel()
{
	return added_subpanel;
}


void SubpanelDesignDialog::loadProcessingSystems()
{
	ui->base_panel->addItem("<select>");

	QMap<QString, QString> systems = NGSD().getProcessingSystems(true, true);
	auto it = systems.constBegin();
	while (it != systems.constEnd())
	{
		ui->base_panel->addItem(it.key(), it.value());
		++it;
	}
}

void SubpanelDesignDialog::createSubpanelCompleter()
{
	QStringList tmp;
	Helper::findFiles(NGSD::getTargetFilePath(true), "*.bed", tmp);
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
	if (name.isEmpty())
	{
		showMessage("Name is not set!", true);
		return;
	}
	if (name.contains(' '))
	{
		showMessage("Name '" + name + "' contains spaces!", true);
		return;
	}

	//check output path does not exist
	QString bed_file = getBedFilename();
	if (QFile::exists(bed_file))
	{
		showMessage("Output file '" + bed_file + "' already exists!", true);
		return;
	}

	//check gene names
	NGSD db;
	genes = geneList();
	if (genes.size()==0)
	{
		showMessage("Genes are not set!", true);
		return;
	}
	foreach(QString gene, genes)
	{
		QPair<QByteArray, QByteArray> geneinfo = db.geneToApproved(gene.toLatin1());
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
	if (messages!="")
	{
		showMessage(messages, true);
		return;
	}
	regions = db.genesToRegions(genes, "ccds", "exon", &stream);
	regions.extend(20);
	regions.merge();
	showMessage("Sub-panel with " + QString::number(genes.count()) + " genes of size " + QString::number(regions.baseCount()) + " bp (CCDS exons with 20 flanking bases) designed. You can store it now!", false);

	ui->store->setEnabled(true);
}

void SubpanelDesignDialog::storePanel()
{
	regions.store(roi_file);
	Helper::storeTextFile(gene_file, genes);

	showMessage("Sub-panel '" + ui->name->text().trimmed() +"' written successfully!", false);
	ui->store->setEnabled(false);

	added_subpanel = true;
}

void SubpanelDesignDialog::openSubpanelFolder()
{
	QDesktopServices::openUrl(QUrl(NGSD::getTargetFilePath(true)));
}

QStringList SubpanelDesignDialog::geneList()
{
	QStringList output;

	QStringList tmp = ui->genes->toPlainText().split('\n');
	foreach (QString t, tmp)
	{
		t = t.trimmed();
		if (!t.isEmpty()) output.append(t);
	}

	return output;
}

QString SubpanelDesignDialog::getBedFilename()
{
	return NGSD::getTargetFilePath(true) + ui->name->text() + ".bed";
}

void SubpanelDesignDialog::showMessage(QString message, bool error)
{
	if (error)
	{
		message = "<font color='red'>" + message + "</font>";
	}

	ui->messages->setText(message);
}
