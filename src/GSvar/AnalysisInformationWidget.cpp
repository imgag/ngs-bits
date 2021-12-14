#include "AnalysisInformationWidget.h"
#include "NGSD.h"
#include "GlobalServiceProvider.h"
#include "GUIHelper.h"
#include <QFileInfo>
#include <QMessageBox>

AnalysisInformationWidget::AnalysisInformationWidget(QString ps_id, QWidget* parent)
	: QWidget(parent)
	, ui_()
	, ps_id_(ps_id)
{
	ui_.setupUi(this);

	updateGUI();
}

void AnalysisInformationWidget::updateGUI()
{
	try
	{
		NGSD db;
		QString ps = db.processedSampleName(ps_id_);
		QString sample_id = db.sampleId(ps);

		//analysis infos
		int last_job_id = db.lastAnalysisOf(ps_id_);
		if (last_job_id>0)
		{
			AnalysisJob job = db.analysisInfo(last_job_id);
			ui_.analysis->setText(job.type + " (" + job.args +")");

			if (!job.history.isEmpty())
			{
				QString text = job.history.first().timeAsString() + " - ";
				if (job.isRunning())
				{
					text += "<font color=red>[analysis running]</font>";
				}
				else
				{
					text += job.history.last().timeAsString();
				}
				ui_.analysis_date->setText(text);
			}
		}

		//file details
		QString sample_type = db.getSampleData(sample_id).type;
		ui_.table->setRowCount(4);
		if (sample_type.startsWith("DNA"))
		{
			ImportStatusGermline import_status = db.importStatus(ps_id_);

			//BAM
			FileLocation file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::BAM);
			ui_.table->setItem(0, 0, GUIHelper::createTableItem(QFileInfo(file.filename).fileName()));
			ui_.table->setItem(0, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			ui_.table->setItem(0, 2, GUIHelper::createTableItem(QString::number(import_status.qc_terms) + " QC terms"));

			//small variants
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::GSVAR);
			ui_.table->setItem(1, 0, GUIHelper::createTableItem(QFileInfo(file.filename).fileName()));
			ui_.table->setItem(1, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(1,1)->setTextColor(QColor(Qt::red));
			ui_.table->setItem(1, 2, GUIHelper::createTableItem(QString::number(import_status.small_variants) + " variants"));

			//CNVs
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::COPY_NUMBER_CALLS);
			ui_.table->setItem(2, 0, GUIHelper::createTableItem(QFileInfo(file.filename).fileName()));
			ui_.table->setItem(2, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(2,1)->setTextColor(QColor(Qt::red));
			ui_.table->setItem(2, 2, GUIHelper::createTableItem(QString::number(import_status.cnvs) + " variants"));

			//SVs
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::STRUCTURAL_VARIANTS);
			ui_.table->setItem(3, 0, GUIHelper::createTableItem(QFileInfo(file.filename).fileName()));
			ui_.table->setItem(3, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(3,1)->setTextColor(QColor(Qt::red));
			ui_.table->setItem(3, 2, GUIHelper::createTableItem(QString::number(import_status.svs) + " variants"));

			GUIHelper::resizeTableCells(ui_.table);
		}
		else
		{
			THROW(ProgrammingException, "Sample type '"+sample_type+"' not handled in anaysis information widget!");
		}
	}
	catch (Exception e)
	{
		QMessageBox::warning(this, "Analysis information", "Error:\n" + e.message());
	}
}
