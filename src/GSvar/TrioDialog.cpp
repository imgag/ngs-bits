#include "TrioDialog.h"
#include "Settings.h"
#include "Exceptions.h"

#include <QInputDialog>
#include <QMessageBox>

TrioDialog::TrioDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, samples_()
{
	ui_.setupUi(this);
	ui_.samples_table->resizeColumnsToContents();
	ui_.status_page_link->setText("<a href=\"" + Settings::string("SampleStatus") + "\"><span style=\"text-decoration: underline; color:#0000ff;\">[open status page]</span></a>");
}

QStringList TrioDialog::samples()
{
	QStringList output;

	if (ui_.samples_table->rowCount()==3)
	{
		output << ui_.samples_table->item(0,0)->text();
		output << ui_.samples_table->item(1,0)->text();
		output << ui_.samples_table->item(2,0)->text();
	}

	return output;
}

void TrioDialog::on_add_samples_clicked(bool)
{
	clearSampleData();

	addSample("child (index)");
	addSample("father");
	addSample("mother");
}

void TrioDialog::addSample(QString status)
{
	QString sample = QInputDialog::getText(this, "Add processed sample", status + ":");
	if (sample=="")
	{
		clearSampleData();
		return;
	}

	//check processing system
	QString sys;
	QString quality;
	try
	{
		ProcessedSampleData processed_sample_data = db_.getProcessedSampleData(db_.processedSampleId(sample));
		sys = processed_sample_data.processing_system;
		quality = processed_sample_data.quality;
	}
	catch (Exception&)
	{
		QMessageBox::warning(this, "Error adding sample", "Could not find processed sample '" + sample + "' in NGSD!");
		clearSampleData();
		return;
	}

	//add sample
	samples_.append(SampleInfo {sample, sys, status, quality});
	updateSampleTable();
}

void TrioDialog::clearSampleData()
{
	samples_.clear();
	updateSampleTable();
}

void TrioDialog::updateSampleTable()
{
	ui_.samples_table->clearContents();
	ui_.samples_table->setRowCount(samples_.count());
	for (int i=0; i<samples_.count(); ++i)
	{
		ui_.samples_table->setItem(i, 0, new QTableWidgetItem(samples_[i].name));
		ui_.samples_table->setItem(i, 1, new QTableWidgetItem(samples_[i].system));
		ui_.samples_table->setItem(i, 2, new QTableWidgetItem(samples_[i].status));
		ui_.samples_table->setItem(i, 3, new QTableWidgetItem(samples_[i].quality));
	}
	ui_.samples_table->resizeColumnsToContents();

	updateStartButton();
}

void TrioDialog::updateStartButton()
{
	ui_.start_button->setEnabled(samples_.count()==3);
}
