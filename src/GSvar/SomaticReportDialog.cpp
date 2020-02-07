#include "SomaticReportDialog.h"
#include "SomaticReportHelper.h"

SomaticReportDialog::SomaticReportDialog(SomaticReportSettings &settings, const VariantList &variants, const CnvList &cnvs, QWidget *parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, settings_(settings)
	, variants_(variants)
	, cnvs_(cnvs)
	, target_region_(settings.report_config.targetFile())
	, tum_cont_snps_(std::numeric_limits<double>::quiet_NaN())
	, tum_cont_max_clonality_(std::numeric_limits<double>::quiet_NaN())
	, tum_cont_histological_(std::numeric_limits<double>::quiet_NaN())
{
	ui_.setupUi(this);

	//Resolve tumor content estimate from NGSD
	QCCollection res = db_.getQCData(db_.processedSampleId(settings.tumor_ps, true));
	try
	{
		tum_cont_snps_ = Helper::toDouble(res.value("QC:2000054", true).asString());
	}
	catch(ArgumentException){} //nothing to do
	catch(TypeConversionException){} //nothing to do

	//Resolve histological tumor content (if available in NGSD)
	QList<SampleDiseaseInfo> disease_infos = db_.getSampleDiseaseInfo(db_.sampleId(settings.tumor_ps), "tumor fraction");
	for(const auto& entry : disease_infos)
	{
		if(entry.type == "tumor fraction")
		{
			try
			{
				tum_cont_histological_ = Helper::toDouble(entry.disease_info);
			}
			catch(ArgumentException)
			{
				;//Nothing to do here
			}
			break;
		}
	}

	tum_cont_max_clonality_ = SomaticReportHelper::getCnvMaxTumorClonality(cnvs);

	//Load HPO term from database
	QString hpo_ngsd;
	QList<SampleDiseaseInfo> details = db_.getSampleDiseaseInfo(db_.sampleId(settings_.tumor_ps));
	for(const auto& info : details)
	{
		if(info.type == "HPO term id")
		{
			hpo_ngsd = info.disease_info;
		}
	}

	QList<tmb_info> hpo_tmbs = tmb_info::load("://Resources/hpoterms_tmb.tsv");


	//Set Reference value proposals
	for(const auto& hpo_tmb : hpo_tmbs)
	{
		if(hpo_tmb.hpoterm != hpo_ngsd) continue;

		QTableWidgetItem *disease = new QTableWidgetItem(QString(hpo_tmb.tumor_entity));
		QTableWidgetItem *tmb_text = new QTableWidgetItem("Median: " + QString::number(hpo_tmb.tmb_median,'f', 2) + " Var/Mbp, Maximum: " + QString::number(hpo_tmb.tmb_max,'f',2) + " Var/Mbp");

		ui_.tmb_reference->insertRow(ui_.tmb_reference->rowCount());
		ui_.tmb_reference->setItem(ui_.tmb_reference->rowCount()-1, 0, disease);
		ui_.tmb_reference->setItem(ui_.tmb_reference->rowCount()-1, 1, tmb_text);
	}

	//load former TMB ref entry (stored in somatic settings from NGSD)
	int old_entry_row = -1;
	for(int i=0; i<ui_.tmb_reference->rowCount(); ++i)
	{
		if(settings_.report_config.tmbReferenceText() == ui_.tmb_reference->item(i, 1)->text())
		{
			old_entry_row = i;
			break;
		}
	}

	if(old_entry_row == -1) //Insert ref text from former report if not  in table
	{
		ui_.tmb_reference->insertRow(ui_.tmb_reference->rowCount());
		ui_.tmb_reference->setItem(ui_.tmb_reference->rowCount()-1, 0, new QTableWidgetItem("former report"));
		ui_.tmb_reference->setItem(ui_.tmb_reference->rowCount()-1, 1, new QTableWidgetItem( settings_.report_config.tmbReferenceText()) );
		ui_.tmb_reference->selectRow(ui_.tmb_reference->rowCount()-1);
	}
	else //set selection to former entry if already in table
	{
		if(settings_.report_config.tmbReferenceText() != "") ui_.tmb_reference->selectRow(old_entry_row);
		else if(ui_.tmb_reference->rowCount() >= 3) ui_.tmb_reference->selectRow(2); //First entry that referes to a suggested ref value
	}

	ui_.tmb_reference->item(0,0)->setFlags(ui_.tmb_reference->item(0,0)->flags() & ~Qt::ItemIsEditable);//set "none" item non-editable
	ui_.tmb_reference->item(0,1)->setFlags(ui_.tmb_reference->item(0,0)->flags() & ~Qt::ItemIsEditable);//set "no value text" item non-editable
	ui_.tmb_reference->item(1,0)->setFlags(ui_.tmb_reference->item(0,0)->flags() & ~Qt::ItemIsEditable);
	ui_.tmb_reference->resizeRowsToContents();
	ui_.tmb_reference->setColumnWidth(1,500);
	ui_.tmb_reference->setRowCount(ui_.tmb_reference->rowCount());
	ui_.tmb_reference->setEditTriggers(QAbstractItemView::DoubleClicked);

	updateGUI();
}

void SomaticReportDialog::updateGUI()
{
	//Update GUI
	if(!std::isnan(tum_cont_snps_))
	{
		ui_.include_max_tum_freq->setChecked(settings_.report_config.tumContentByMaxSNV());
		ui_.include_max_tum_freq->setText(ui_.include_max_tum_freq->text() + " ("  + QString::number(tum_cont_snps_, 'f', 1) +"%)");
	}
	else
	{
		ui_.include_max_tum_freq->setEnabled(false);
	}

	if(!std::isnan( tum_cont_max_clonality_))
	{
		ui_.include_max_clonality->setChecked(settings_.report_config.tumContentByClonality());
		ui_.include_max_clonality->setText(ui_.include_max_clonality->text() + " ("  + QString::number(tum_cont_max_clonality_ * 100., 'f', 1) +"%)");
	}
	else
	{
		ui_.include_max_clonality->setCheckable(false);
	}

	if(!std::isnan( tum_cont_histological_) && tum_cont_histological_ > 0.)
	{
		ui_.include_tum_content_histological->setChecked(settings_.report_config.tumContentByHistological());
		ui_.include_tum_content_histological->setText(ui_.include_tum_content_histological->text() + " (" + QString::number(tum_cont_histological_, 'f', 1)+"%)");
	}
	else
	{
		ui_.include_tum_content_histological->setCheckable(false);
	}

	if(target_region_ != "")
	{
		ui_.target_bed_path->setText("Target region: " + target_region_);
	}
	else
	{
		ui_.target_bed_path->setText("Target region: not set");
	}

	if(SomaticReportHelper::cnvBurden(cnvs_) > 0.01)
	{
		ui_.include_cnv_burden->setChecked(settings_.report_config.cnvBurden());
		ui_.include_cnv_burden->setText(ui_.include_cnv_burden->text() + " (" + QString::number(SomaticReportHelper::cnvBurden(cnvs_), 'f', 1)  + "%)");
	}
	else
	{
		ui_.include_cnv_burden->setCheckable(false);
	}

	//Preselect remaining options
	ui_.include_msi_status->setChecked(settings_.report_config.msiStatus());
	ui_.include_cin_hint->setChecked(settings_.report_config.cinHint());

	//index of hrd_score is equal to actual score value
	ui_.hrd_score->setCurrentIndex(settings_.report_config.hrdScore());


	//Load possible quality settings
	QStringList quality_entries = db_.getEnum("somatic_report_configuration", "quality");
	for(const auto& entry: quality_entries)
	{
		ui_.quality->addItem(entry);
	}

	//Set selected entry to old setting
	for(int i=0; i<ui_.quality->count();++i)
	{
		if(ui_.quality->itemText(i) == settings_.report_config.quality())
		{
			ui_.quality->setCurrentIndex(i);
			break;
		}
	}
}

void SomaticReportDialog::writeBackSettings()
{
	settings_.include_gap_statistics = ui_.include_low_cov->isChecked();
	settings_.report_config.setTumContentByMaxSNV(ui_.include_max_tum_freq->isChecked());
	settings_.report_config.setTumContentByClonality(ui_.include_max_clonality->isChecked());
	settings_.report_config.setTumContentByHistological(ui_.include_tum_content_histological->isChecked());
	settings_.report_config.setMsiStatus(ui_.include_msi_status->isChecked());
	settings_.report_config.setCnvBurden(ui_.include_cnv_burden->isChecked());
	settings_.report_config.setCinHint(ui_.include_cin_hint->isChecked());

	//current index of hrd_score is identical to value!
	settings_.report_config.setHrdScore(ui_.hrd_score->currentIndex());

	settings_.report_config.setQuality(ui_.quality->currentText());

	if(ui_.tmb_reference->selectionModel()->selectedRows().count() == 1)
	{
		QString ref_text = ui_.tmb_reference->item(ui_.tmb_reference->selectionModel()->selectedRows()[0].row(), 1)->text();
		settings_.report_config.setTmbReferenceText(ref_text);
	}


}

SomaticReportDialog::report_type SomaticReportDialog::getReportType()
{
	if(ui_.report_type_dna->isChecked()) return report_type::DNA;
	else return report_type::RNA;
}

void SomaticReportDialog::enableChoiceReportType(bool enabled)
{
	ui_.report_type_label->setEnabled(enabled);
	ui_.report_type_dna->setEnabled(enabled);
	ui_.report_type_rna->setEnabled(enabled);
}

void SomaticReportDialog::enableGapStatistics(bool enabled)
{
	ui_.include_low_cov->setEnabled(enabled);
}
