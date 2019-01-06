#include "SampleDetailsDockWidget.h"
#include "DiagnosticStatusWidget.h"
#include "GUIHelper.h"
#include "BasicStatistics.h"
#include "Settings.h"
#include "DiseaseInfoDialog.h"
#include "SampleDiseaseInfoWidget.h"

#include <QMenu>
#include <QStringList>
#include <QFileInfo>
#include <QInputDialog>
#include <Log.h>

SampleDetailsDockWidget::SampleDetailsDockWidget(QWidget *parent)
	: QDockWidget(parent)
	, ui_()
	, processed_sample_name_()
{
	ui_.setupUi(this);
	connect(ui_.qc_af_dev_btn, SIGNAL(clicked(QPoint)), this, SLOT(showAlleleFrequencyDeviationMenu(QPoint)));

	//set up NGSD edit button
	QMenu* menu = new QMenu();
	menu->addAction("Edit disease group/status", this, SLOT(editDiseaseGroupAndInfo()));
	menu->addAction("Edit disease details", this, SLOT(editDiseaseDetails()));
	menu->addAction("Edit quality", this, SLOT(setQuality()));
	menu->addAction("Edit diagnostic status", this, SLOT(editDiagnosticStatus()));
	ui_.ngsd_edit->setMenu(menu);
	if (!Settings::boolean("NGSD_enabled", true))
	{
		ui_.ngsd_edit->setEnabled(false);
	}

	clear();
}

void SampleDetailsDockWidget::editDiagnosticStatus()
{
	NGSD db;
	QString processed_sample_id = db.processedSampleId(processed_sample_name_);

	DiagnosticStatusWidget* widget = new DiagnosticStatusWidget(this);
	widget->setStatus(db.getDiagnosticStatus(processed_sample_id));
	auto dlg = GUIHelper::createDialog(widget, "Diagnostic status of " + processed_sample_name_, "", true);
	if (dlg->exec()!=QDialog::Accepted) return;

	db.setDiagnosticStatus(processed_sample_id, widget->status());

	refresh(processed_sample_name_);
}

void SampleDetailsDockWidget::editDiseaseGroupAndInfo()
{
	QString sample_id = NGSD().sampleId(processed_sample_name_);
	DiseaseInfoDialog dlg(sample_id, this);

	if (dlg.exec()==QDialog::Accepted)
	{
		refresh(processed_sample_name_);
	}
}

void SampleDetailsDockWidget::editDiseaseDetails()
{
	NGSD db;
	QString sample_id = db.sampleId(processed_sample_name_);

	SampleDiseaseInfoWidget* widget = new SampleDiseaseInfoWidget(processed_sample_name_, this);
	widget->setDiseaseInfo(db.getSampleDiseaseInfo(sample_id));
	auto dlg = GUIHelper::createDialog(widget, "Sample disease details", "", true);
	if (dlg->exec() == QDialog::Accepted)
	{
		db.setSampleDiseaseInfo(sample_id, widget->diseaseInfo());
		refresh(processed_sample_name_);
	}
}

void SampleDetailsDockWidget::setQuality()
{
	QStringList qualities = NGSD().getEnum("processed_sample", "quality");

	bool ok;
	QString quality = QInputDialog::getItem(this, "Select processed sample quality", "quality:", qualities, qualities.indexOf(ui_.qc_quality->text()), false, &ok);
	if (!ok) return;

	NGSD db;
	QString processed_sample_id = db.processedSampleId(processed_sample_name_);
	db.setProcessedSampleQuality(processed_sample_id, quality);

	refresh(processed_sample_name_);
}

QGridLayout* SampleDetailsDockWidget::clearDiseaseDetails()
{
	QGridLayout* layout = qobject_cast<QGridLayout*>(ui_.disease_info_group->findChild<QGridLayout*>());

	QLayoutItem *child;
	while (layout->rowCount()>0 && ((child = layout->takeAt(0)) != 0))
	{
		delete child->widget();
		delete child;
	}

	return layout;
}

void SampleDetailsDockWidget::showAlleleFrequencyDeviationMenu(QPoint pos)
{
	QMenu* menu = new QMenu(this);
	menu->addAction("Show AF histogram", this, SIGNAL(showAlleleFrequencyHistogram()));
	menu->exec(pos);
}

void SampleDetailsDockWidget::refresh(QString processed_sample_name)
{
	processed_sample_name_ = processed_sample_name;
	ui_.name->setText(processed_sample_name_);

	if (!Settings::boolean("NGSD_enabled", true)) return;
	NGSD db;

	//data from NGSD
	try
	{
		//**** part 1: sample data
		QString sample_id = db.sampleId(processed_sample_name_);

		SampleData sample_data = db.getSampleData(sample_id);
		ui_.name_ext->setText(sample_data.name_external);
		ui_.tumor_ffpe->setText(QString(sample_data.is_tumor ? "yes" : "no") + " / " + (sample_data.is_ffpe ? "yes" : "no"));
		QString group = sample_data.disease_group.trimmed();
		if (group=="n/a") group = "<font color='red'>"+group+"</font>";
		ui_.disease_group->setText(group);
		QString status = sample_data.disease_status.trimmed();
		if (status=="n/a") status = "<font color='red'>"+status+"</font>";
		ui_.disease_status->setText(status);

		//disease details
		QGridLayout* disease_layout = clearDiseaseDetails();
		QList<SampleDiseaseInfo> disease_info = db.getSampleDiseaseInfo(sample_id);
		if (disease_info.isEmpty())
		{
			disease_layout->addWidget(new QLabel("<font color='red'>n/a</font>"), 0, 0);
		}
		else
		{
			foreach(const SampleDiseaseInfo& entry, disease_info)
			{
				int row = disease_layout->rowCount();

				//special handling of HPO (show term name)
				QString disease_info = entry.disease_info;
				if (entry.type=="HPO term id")
				{
					Phenotype pheno = db.phenotypeByAccession(disease_info.toLatin1(), false);
					disease_info +=  " (" + (pheno.name().isEmpty() ? "invalid" : pheno.name()) + ")";
				}
				disease_layout->addWidget(new QLabel(entry.type + ": " + disease_info), row, 0);
			}
		}

		//**** part 2: processed sample data
		QString processed_sample_id = db.processedSampleId(processed_sample_name_);

		//QC data
		ProcessedSampleData processed_sample_data = db.getProcessedSampleData(processed_sample_id);
		QCCollection qc = db.getQCData(processed_sample_id);
		ui_.qc_reads->setText(qc.value("QC:2000005", true).toString(0) + " (length: " + qc.value("QC:2000006", true).toString(0) + ")");

		statisticsLabel(ui_.qc_avg_depth, "QC:2000025", qc, true, false);
		statisticsLabel(ui_.qc_perc_20x, "QC:2000027", qc, true, false, 2, "%");
		statisticsLabel(ui_.qc_insert_size,"QC:2000023", qc, true, true);
		statisticsLabel(ui_.qc_vars,"QC:2000013", qc, true, true, 0);
		statisticsLabel(ui_.qc_vars_known,"QC:2000014", qc, true, false, 2, "%");
		statisticsLabel(ui_.qc_af_dev,"QC:2000051", qc, false, true, 4, "%");

		ui_.qc_kasp->setText(qc.value("kasp").asString());
		QString quality = processed_sample_data.quality;
		if (quality=="bad") quality = "<font color='red'>"+quality+"</font>";
		if (quality=="medium") quality = "<font color='orange'>"+quality+"</font>";
		ui_.qc_quality->setText(quality);
		ui_.comment->setText(processed_sample_data.comments);
		ui_.system->setText(processed_sample_data.processing_system);
		ui_.gender->setText(processed_sample_data.gender);

		//diagnostic status
		DiagnosticStatusData diag_status = db.getDiagnosticStatus(processed_sample_id);
		ui_.diag_status->setText(diag_status.dagnostic_status);
		ui_.diag_outcome->setText(diag_status.outcome);
		ui_.diag_genes->setText(diag_status.genes_causal);
		ui_.diag_inheritance->setText(diag_status.inheritance_mode);
		ui_.diag_comments->setText(diag_status.comments);

		//last analysis (BAM file date)
		QDateTime last_modified = QFileInfo(db.processedSamplePath(processed_sample_id, NGSD::BAM)).lastModified();
		QString date_text = last_modified.toString("yyyy-MM-dd");
		if (last_modified < QDateTime::currentDateTime().addMonths(-6))
		{
			date_text = " <font color='red'>" + date_text + "</font>";
		}
		ui_.date_bam->setText(date_text);

		ui_.ngsd_edit->setEnabled(true);

	}
	catch(Exception& e)
	{
		Log::warn("Error getting sample information from NGSD: " + e.message());
		ui_.ngsd_edit->setEnabled(false);
	}
}

void SampleDetailsDockWidget::clear()
{
	//labels
	ui_.name->clear();
	ui_.name_ext->clear();
	ui_.tumor_ffpe->clear();
	ui_.disease_group->clear();
	ui_.disease_status->clear();
	clearDiseaseDetails();
	ui_.qc_reads->clear();
	ui_.qc_kasp->clear();
	ui_.qc_quality->clear();
	ui_.qc_insert_size->clear();
	ui_.qc_avg_depth->clear();
	ui_.qc_perc_20x->clear();
	ui_.qc_vars->clear();
	ui_.qc_vars_known->clear();
	ui_.qc_af_dev->clear();
	ui_.comment->clear();
	ui_.system->clear();
	ui_.gender->clear();
	ui_.diag_status->clear();
	ui_.diag_outcome->clear();
	ui_.diag_genes->clear();
	ui_.diag_inheritance->clear();
	ui_.diag_comments->clear();
	ui_.date_bam->clear();

	//buttons
	ui_.ngsd_edit->setEnabled(false);
}

void SampleDetailsDockWidget::statisticsLabel(QLabel* label, QString accession, const QCCollection& qc, bool label_outlier_low, bool label_outlier_high, int decimal_places, QString suffix)
{
	try
	{
		//get QC value
		QString value_str = qc.value(accession, true).toString();
		label->setText(value_str + suffix);

		//get QC value statistics
		NGSD db;
		QVector<double> values = db.getQCValues(accession, db.processedSampleId(processed_sample_name_));
		std::sort(values.begin(), values.end());
		double median = BasicStatistics::median(values, false);
		double mad = 1.428 * BasicStatistics::mad(values, median);
		label->setToolTip("mean: " + QString::number(median, 'f', decimal_places) + "<br>stdev: " + QString::number(mad, 'f', decimal_places));

		//mark QC value red if it is an outlier
		bool ok = false;
		double value = value_str.toDouble(&ok);
		if (!ok || (label_outlier_low && value < median-2*mad) || (label_outlier_high && value > median+2*mad))
		{
			label->setText("<font color='red'>" + value_str + suffix + "</font>");
		}
	}
	catch (ArgumentException e) //in case the QC value does not exist in the DB
	{
		label->setText("<font color='red'>n/a</font>");
		label->setToolTip("NGSD error: " + e.message());
	}
	catch (StatisticsException e) //in case the QC values statistics cannot be calculated
	{
		label->setToolTip("Statistics error: " + e.message());
	}
}
