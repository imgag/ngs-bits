#include "ReportDialog.h"
#include "GUIHelper.h"
#include <QTableWidgetItem>
#include <QPushButton>
#include <QMenu>


ReportDialog::ReportDialog(ReportSettings& settings, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs, QString target_region, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, settings_(settings)
	, variants_(variants)
	, cnvs_(cnvs)
	, svs_(svs)
	, roi_file_(target_region.trimmed())
	, roi_()
{
	ui_.setupUi(this);
	initGUI();

	//variant types
	connect(ui_.report_type, SIGNAL(currentTextChanged(QString)), this, SLOT(updateGUI()));

	//disable ok button when no outcome is set
	connect(ui_.diag_status, SIGNAL(outcomeChanged(QString)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.report_type, SIGNAL(currentTextChanged(QString)), this, SLOT(activateOkButtonIfValid()));

	//enable/disable low-coverage settings
	connect(ui_.details_cov, SIGNAL(stateChanged(int)), this, SLOT(updateGUI()));

	//write settings if accepted
	connect(this, SIGNAL(accepted()), this, SLOT(writeBackSettings()));

	//handle ROI
	if (roi_file_!="")
	{
		roi_.load(roi_file_);
	}

	updateGUI();
}


void ReportDialog::initGUI()
{
	//report types
	ui_.report_type->addItem("");
	ui_.report_type->addItems(ReportVariantConfiguration::getTypeOptions());

	//diagnostic status
	ui_.diag_status->setStatus(settings_.diag_status);

	//settings
	ui_.details_cov->setChecked(settings_.show_coverage_details);
	ui_.min_cov->setValue(settings_.min_depth);
	ui_.details_cov_roi->setChecked(settings_.roi_low_cov);
	ui_.depth_calc->setChecked(settings_.recalculate_avg_depth);
	ui_.tool_details->setChecked(settings_.show_tool_details);
	ui_.omim_table->setChecked(settings_.show_omim_table);
	ui_.class_info->setChecked(settings_.show_class_details);
	ui_.language->setCurrentText(settings_.language);

	//no ROI > no roi options
	if (roi_file_=="")
	{
		ui_.details_cov->setChecked(false);
		ui_.details_cov->setEnabled(false);

		ui_.depth_calc->setChecked(false);
		ui_.depth_calc->setEnabled(false);

		ui_.details_cov_roi->setChecked(false);
		ui_.details_cov_roi->setEnabled(false);

		ui_.min_cov_label->setEnabled(false);
		ui_.min_cov->setEnabled(false);

		ui_.omim_table->setChecked(false);
		ui_.omim_table->setEnabled(false);
	}
}

void ReportDialog::updateGUI()
{
	//init
	ui_.vars->setRowCount(0);
	int row = 0;

	//add small variants
	int geno_idx = variants_.getSampleHeader().infoByStatus(true).column_index;
	int gene_idx = variants_.annotationIndexByName("gene");
	int class_idx = variants_.annotationIndexByName("classification");
	foreach(int i, settings_.report_config.variantIndices(VariantType::SNVS_INDELS, true, type()))
	{
		const Variant& variant = variants_[i];
		if (roi_file_!="" && !roi_.overlapsWith(variant.chr(), variant.start(), variant.end())) continue;
		const ReportVariantConfiguration& var_conf = settings_.report_config.get(VariantType::SNVS_INDELS,i);

		ui_.vars->setRowCount(ui_.vars->rowCount()+1);
		ui_.vars->setItem(row, 0, new QTableWidgetItem(var_conf.report_type + (var_conf.causal ? " (causal)" : "")));
		QString tmp = variant.toString(false, 30) + " (" + variant.annotations().at(geno_idx) + ")";
		ui_.vars->setItem(row, 1, new QTableWidgetItem(tmp));
		ui_.vars->setItem(row, 2, new QTableWidgetItem(variant.annotations().at(gene_idx), QTableWidgetItem::Type));
		ui_.vars->setItem(row, 3, new QTableWidgetItem(variant.annotations().at(class_idx), QTableWidgetItem::Type));
		++row;
	}


	//add CNVs
	foreach(int i, settings_.report_config.variantIndices(VariantType::CNVS, true, type()))
	{
		const CopyNumberVariant& cnv = cnvs_[i];
		if (roi_file_!="" && !roi_.overlapsWith(cnv.chr(), cnv.start(), cnv.end())) continue;
		const ReportVariantConfiguration& var_conf = settings_.report_config.get(VariantType::CNVS,i);

		ui_.vars->setRowCount(ui_.vars->rowCount()+1);
		ui_.vars->setItem(row, 0, new QTableWidgetItem(var_conf.report_type + (var_conf.causal ? " (causal)" : "")));
		ui_.vars->setItem(row, 1, new QTableWidgetItem("CNV " + cnv.toStringWithMetaData() + " cn=" + QString::number(cnv.copyNumber(cnvs_.annotationHeaders()))));
		ui_.vars->setItem(row, 2, new QTableWidgetItem(cnv.genes().join(", "), QTableWidgetItem::Type));
		ui_.vars->setItem(row, 3, new QTableWidgetItem(var_conf.classification));
		++row;
	}

	//add Svs
	foreach(int i, settings_.report_config.variantIndices(VariantType::SVS, true, type()))
	{
		const BedpeLine& sv = svs_[i];
		BedFile affected_region = sv.affectedRegion();
		if (roi_file_!="")
		{
			if (sv.type() != StructuralVariantType::BND)
			{
				if (!roi_.overlapsWith(affected_region[0].chr(), affected_region[0].start(), affected_region[0].end())) continue;
			}
			else
			{
				if (!roi_.overlapsWith(affected_region[0].chr(), affected_region[0].start(), affected_region[0].end())
					&& !roi_.overlapsWith(affected_region[1].chr(), affected_region[1].start(), affected_region[1].end())) continue;
			}
		}
		const ReportVariantConfiguration& var_conf = settings_.report_config.get(VariantType::SVS,i);

		QString sv_string = "SV " + affected_region[0].toString(true);
		if (sv.type()==StructuralVariantType::BND) sv_string += " <-> " + affected_region[1].toString(true);
		sv_string += " type=" + BedpeFile::typeToString(sv.type());

		ui_.vars->setRowCount(ui_.vars->rowCount()+1);
		ui_.vars->setItem(row, 0, new QTableWidgetItem(var_conf.report_type + (var_conf.causal ? " (causal)" : "")));
		ui_.vars->setItem(row, 1, new QTableWidgetItem(sv_string));
		ui_.vars->setItem(row, 2, new QTableWidgetItem(sv.genes(svs_.annotationHeaders()).join(", "), QTableWidgetItem::Type));
		ui_.vars->setItem(row, 3, new QTableWidgetItem(var_conf.classification));
		++row;
	}


	//resize table cells
	GUIHelper::resizeTableCells(ui_.vars);

	//enable coverage detail settings only if necessary
	if (roi_file_!="")
	{
		bool add_cov_details = ui_.details_cov->isChecked();
		ui_.min_cov->setEnabled(add_cov_details);
		ui_.min_cov_label->setEnabled(add_cov_details);
		ui_.depth_calc->setEnabled(add_cov_details);
		if (!add_cov_details) ui_.depth_calc->setChecked(false);
		ui_.details_cov_roi->setEnabled(add_cov_details);
		if (!add_cov_details) ui_.details_cov_roi->setChecked(false);
	}

	//buttons
	activateOkButtonIfValid();
}

void ReportDialog::writeBackSettings()
{
	//diag status
	settings_.diag_status = ui_.diag_status->status();

	//settings
	settings_.show_coverage_details = ui_.details_cov->isChecked();
	settings_.min_depth = ui_.min_cov->value();
	settings_.roi_low_cov = ui_.details_cov_roi->isChecked();
	settings_.recalculate_avg_depth = ui_.depth_calc->isChecked();
	settings_.show_tool_details = ui_.tool_details->isChecked();
	settings_.show_omim_table = ui_.omim_table->isChecked();
	settings_.show_class_details = ui_.class_info->isChecked();
	settings_.language = ui_.language->currentText();
}

void ReportDialog::activateOkButtonIfValid()
{
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	if (ui_.diag_status->status().outcome=="n/a") return;
	if (ui_.report_type->currentIndex()==0) return;

	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}
