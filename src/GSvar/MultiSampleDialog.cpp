#include "MultiSampleDialog.h"
#include "Settings.h"
#include "Exceptions.h"

#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>

MultiSampleDialog::MultiSampleDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
	, samples_()
	, db_()
{
	ui_.setupUi(this);
	ui_.samples_table->resizeColumnsToContents();
	ui_.status_page_link->setText("<a href=\"" + Settings::string("SampleStatus") + "\"><span style=\"text-decoration: underline; color:#0000ff;\">[open status page]</span></a>");
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
		output << affected2str(info.affected);
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
	QString sys = name2sys(sample);
	if (sys=="unknown")
	{
		QMessageBox::warning(this, "Error adding sample", "Could not determine processing system of sample " + sample + " from NGSD!");
		return;
	}
	if (samples_.count()>0 && sys!=samples_[0].system)
	{
		QMessageBox::warning(this, "Error adding sample", "Processing system of sample " + sample + " is " + sys + ".\nExpected same processing system as other samples: " + samples_[0].system);
		return;
	}

	samples_.append(SampleInfo {sample, sys, affected});
	updateSampleTable();
}

QString MultiSampleDialog::name2sys(QString name)
{
	QString sys;
	try
	{
		sys = NGSD().getProcessingSystem(name.trimmed(), NGSD::LONG);
	}
	catch (Exception&)
	{
		sys = "unknown";
	}

	return sys;
}

QString MultiSampleDialog::affected2str(bool affected)
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
		ui_.samples_table->setItem(i, 2, new QTableWidgetItem(affected2str(samples_[i].affected)));
	}
	ui_.samples_table->resizeColumnsToContents();

	//start button
	ui_.start_button->setEnabled(samples_.count()>=2);
}
