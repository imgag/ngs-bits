#include "SomaticReportDialog.h"
#include "SomaticReportHelper.h"

SomaticReportDialog::SomaticReportDialog(SomaticReportSettings &settings, const VariantList &variants, const CnvList &cnvs, QWidget *parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, settings_(settings)
	, variants_(variants)
	, cnvs_(cnvs)
	, target_region_(settings.target_bed_file)
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




	//Update GUI
	if(!std::isnan( tum_cont_snps_))
	{
		ui_.include_max_tum_freq->setChecked(true);
		ui_.include_max_tum_freq->setText(ui_.include_max_tum_freq->text() + " ("  + QString::number(tum_cont_snps_, 'f', 1) +"%)");
	}
	else
	{
		ui_.include_max_tum_freq->setEnabled(false);
	}

	if(!std::isnan( tum_cont_max_clonality_))
	{
		ui_.include_max_clonality->setChecked(true);
		ui_.include_max_clonality->setText(ui_.include_max_clonality->text() + " ("  + QString::number(tum_cont_max_clonality_ * 100., 'f', 1) +"%)");
	}
	else
	{
		ui_.include_max_clonality->setCheckable(false);
	}

	if(!std::isnan( tum_cont_histological_) && tum_cont_histological_ > 0.)
	{
		ui_.include_tum_content_histological->setChecked(true);
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

}

void SomaticReportDialog::writeBackSettings()
{
	settings_.include_cov_statistics = ui_.include_low_cov->isChecked();
	settings_.include_tum_content_snp_af = ui_.include_max_tum_freq->isChecked();
	settings_.include_tum_content_clonality = ui_.include_max_clonality->isChecked();
	settings_.include_tum_content_histological = ui_.include_tum_content_histological->isChecked();
}
