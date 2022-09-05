#include "GenesToRegionsDialog.h"
#include "ui_GenesToRegionsDialog.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "NGSHelper.h"
#include <QClipboard>
#include <QFileDialog>

GenesToRegionsDialog::GenesToRegionsDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::GenesToRegionsDialog)
{
	ui->setupUi(this);
	connect(ui->convert_btn, SIGNAL(pressed()), this, SLOT(convertGenesToRegions()));
	connect(ui->clip_btn, SIGNAL(pressed()), this, SLOT(copyRegionsToClipboard()));
	connect(ui->store_btn, SIGNAL(pressed()), this, SLOT(storeRegionsAsBED()));
	connect(ui->mode, SIGNAL(currentIndexChanged(int)), this, SLOT(setExpandByMode()));
}

GenesToRegionsDialog::~GenesToRegionsDialog()
{
	delete ui;
}

void GenesToRegionsDialog::convertGenesToRegions()
{
	ui->regions->clear();

	//convert input to gene list (text before first tab in each line)
	GeneSet genes;
	QByteArrayList lines = ui->genes->toPlainText().toUtf8().split('\n');
	foreach(const QByteArray& line, lines)
	{
		if (line.startsWith("#")) continue;
		QByteArrayList parts = line.split('\t');
		genes.insert(parts[0]);
	}

	//convert gene list to regions (BED)
	Transcript::SOURCE source = Transcript::stringToSource(ui->source->currentText());
	QString mode = ui->mode->currentText().toLower();
	NGSD db;
	QString messages;
	QTextStream stream(&messages);
	regions = db.genesToRegions(genes, source, mode, false, false, &stream);
	int extend_by = ui->expand->value();
	if(extend_by>0)
	{
		regions.extend(ui->expand->value());
	}

	//merge
	if (mode=="gene")
	{
		regions.merge(true, true, true);
	}

	//set output
	ui->regions->setPlainText(messages);
	ui->regions->appendPlainText(regions.toText());

	//scroll to top
	ui->regions->moveCursor(QTextCursor::Start);
	ui->regions->ensureCursorVisible();
}

void GenesToRegionsDialog::copyRegionsToClipboard()
{
	QApplication::clipboard()->setText(ui->regions->toPlainText());
}

void GenesToRegionsDialog::storeRegionsAsBED()
{
	QString filename = QFileDialog::getSaveFileName(this, "Store regions as BED file", "", "BED files (*.bed);;All files (*.*)");
	if (filename.isEmpty()) return;

	regions.store(filename);
}

void GenesToRegionsDialog::setExpandByMode()
{
	QString mode = ui->mode->currentText();
	if(mode=="gene")
	{
		ui->expand->setValue(5000);
	}
	else if(mode=="exon")
	{
		ui->expand->setValue(20);
	}
}
