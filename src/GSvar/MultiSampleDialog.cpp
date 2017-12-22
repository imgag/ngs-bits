#include "MultiSampleDialog.h"
#include "Settings.h"
#include "Exceptions.h"

#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>

MultiSampleDialog::MultiSampleDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, samples_()
{
	ui_.setupUi(this);
	ui_.samples_table->resizeColumnsToContents();
	ui_.status_page_link->setText("<a href=\"" + Settings::string("SampleStatus") + "\"><span style=\"text-decoration: underline; color:#0000ff;\">[open status page]</span></a>");

	connect(ui_.target_region, SIGNAL(currentIndexChanged(int)), this, SLOT(updateStartButton()));
	connect(ui_.start_button, SIGNAL(clicked(bool)), this, SLOT(startAnalysis()));
}

QStringList MultiSampleDialog::samples()
{
	QStringList output;

	foreach(const SampleInfo& info, samples_)
	{
		output << info.name;
	}

	return output;
}

QStringList MultiSampleDialog::status()
{
	QStringList output;

	foreach(const SampleInfo& info, samples_)
	{
		output << formatAffected(info.affected);
	}

	return output;
}

void MultiSampleDialog::on_add_affected_clicked(bool)
{
	addSample(true);
}

void MultiSampleDialog::on_add_control_clicked(bool)
{
	addSample(false);
}

void MultiSampleDialog::addSample(bool affected)
{
	QString sample = QInputDialog::getText(this, "Add sample", "processed sample:");
	if (sample=="") return;

	//check processing system
	QString sys;
	QString quality;
	try
	{
		sys = db_.getProcessingSystem(sample, NGSD::LONG);
		quality = db_.getProcessedSampleQuality(sample, false);
	}
	catch (Exception&)
	{
		QMessageBox::warning(this, "Error adding sample", "Could not determine processing system or quality of sample " + sample + " from NGSD!");
		return;
	}


	//fill target region combobox
	bool contained = false;
	for (int i=0; i<ui_.target_region->count(); ++i)
	{
		if (ui_.target_region->itemText(i)==sys)
		{
			contained = true;
		}
	}
	if (!contained)
	{
		ui_.target_region->addItem(sys);
		if (ui_.target_region->count()==2)
		{
			ui_.target_region->setCurrentText(sys);
		}
		if (ui_.target_region->count()>2)
		{
			ui_.target_region->setCurrentText("n/a");
		}
	}

	//add sample
	samples_.append(SampleInfo {sample, sys, affected, quality});
	updateSampleTable();
}

QString MultiSampleDialog::formatAffected(bool affected)
{
	return affected ? "affected" : "control";
}

void MultiSampleDialog::updateSampleTable()
{
	ui_.samples_table->clearContents();
	ui_.samples_table->setRowCount(samples_.count());
	for (int i=0; i<samples_.count(); ++i)
	{
		ui_.samples_table->setItem(i, 0, new QTableWidgetItem(samples_[i].name));
		ui_.samples_table->setItem(i, 1, new QTableWidgetItem(samples_[i].system));
		ui_.samples_table->setItem(i, 2, new QTableWidgetItem(formatAffected(samples_[i].affected)));
		ui_.samples_table->setItem(i, 3, new QTableWidgetItem(samples_[i].quality));
	}
	ui_.samples_table->resizeColumnsToContents();

	updateStartButton();
}

void MultiSampleDialog::updateStartButton()
{
	ui_.start_button->setEnabled(samples_.count()>=2 && ui_.target_region->currentText()!="n/a");
}

void MultiSampleDialog::startAnalysis()
{
	//reorder samples (if more then one processing system is available)
	QString system = ui_.target_region->currentText();
	if (system!="n/a")
	{
		for (int i=0; i<samples_.count(); ++i)
		{
			if (samples_[i].system==system)
			{
				samples_.swap(0, i);
				break;
			}
		}
	}

	//accept dialog
	accept();
}
