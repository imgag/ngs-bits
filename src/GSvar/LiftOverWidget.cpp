#include "LiftOverWidget.h"
#include "ui_LiftOverWidget.h"
#include "BedFile.h"
#include "Exceptions.h"
#include "GSvarHelper.h"

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
		BedLine in = BedLine::fromString(ui->input->text());
		BedLine out = GSvarHelper::liftOver(in.chr(), in.start(), in.end(), ui->hg38_19->isChecked());
		QString mode = ui->hg38_19->isChecked() ? ui->hg38_19->text() : ui->hg19_38->text();
		ui->output->setPlainText("Lift-over " + mode + ":\n" + out.toString(true));
	}
	catch(Exception& e)
	{
		ui->output->setPlainText("Error:\n" + e.message());
	}
}
