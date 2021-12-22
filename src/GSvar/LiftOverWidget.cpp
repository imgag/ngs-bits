#include "LiftOverWidget.h"
#include "ui_LiftOverWidget.h"
#include "BedFile.h"
#include "Exceptions.h"
#include "GSvarHelper.h"
#include "NGSHelper.h"

LiftOverWidget::LiftOverWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::LiftOverWidget)
{
	ui->setupUi(this);
	connect(ui->input, SIGNAL(editingFinished()), this, SLOT(performLiftover()));
	connect(ui->btn, SIGNAL(clicked()), this, SLOT(performLiftover()));
}

LiftOverWidget::~LiftOverWidget()
{
	delete ui;
}

void LiftOverWidget::performLiftover()
{
	//clear
	ui->output->clear();

	//lift-over
	try
	{
		Chromosome chr;
		int start;
		int end;
		NGSHelper::parseRegion(ui->input->text(), chr, start, end);

		BedLine out = GSvarHelper::liftOver(chr, start, end, ui->hg19_38->isChecked());
		QString mode = ui->hg38_19->isChecked() ? ui->hg38_19->text() : ui->hg19_38->text();
		ui->output->setPlainText("Lift-over " + mode + ":\n" + out.toString(true));
	}
	catch(Exception& e)
	{
		ui->output->setPlainText("Error:\n" + e.message());
	}
}
