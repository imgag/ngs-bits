#include "ReportDialog.h"
#include "GUIHelper.h"
#include <QTableWidgetItem>
#include <QPushButton>
#include <QMenu>


ReportDialog::ReportDialog(ReportSettings& settings, const VariantList& variants, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, settings_(settings)
	, variants_(variants)
{
	ui_.setupUi(this);

	//disable ok button when no outcome is set
	connect(ui_.diag_status, SIGNAL(outcomeChanged(QString)), this, SLOT(activateOkButtonIfValid()));

	//enable/disable low-coverage settings
	connect(ui_.details_cov, SIGNAL(stateChanged(int)), this, SLOT(updateCoverageSettings(int)));

	//write settings if accepted
	connect(this, SIGNAL(accepted()), this, SLOT(writeBackSettings()));

	updateGUI();
}

void ReportDialog::updateGUI()
{
	//diagnostic status
	ui_.diag_status->setStatus(settings_.diag_status);

	//variants
	int geno_idx = variants_.getSampleHeader().infoByStatus(true).column_index;
	QList<int> selected_variants = settings_.variantIndices(VariantType::SNVS_INDELS, true);
	ui_.vars->setRowCount(selected_variants.count());
	int row = 0;
	foreach(int i, selected_variants)
	{
		const Variant& variant = variants_[i];
		const ReportVariantConfiguration& var_conf = settings_.getConfiguration(VariantType::SNVS_INDELS,i);

		ui_.vars->setItem(row, 0, new QTableWidgetItem(var_conf.type + (var_conf.causal ? " (causal)" : "")));
		QString tmp = variant.toString(false, 30) + " (" + variant.annotations().at(geno_idx) + ")";
		ui_.vars->setItem(row, 1, new QTableWidgetItem(tmp));

		for (int j=2; j<ui_.vars->horizontalHeader()->count(); ++j)
		{
			QString label = ui_.vars->horizontalHeaderItem(j)->text();
			int index = variants_.annotationIndexByName(label);
			ui_.vars->setItem(row, j, new QTableWidgetItem(variant.annotations().at(index), 0));
		}

		++row;
	}
	GUIHelper::resizeTableCells(ui_.vars);

	//settings
	ui_.details_cov->setChecked(settings_.show_coverage_details);
	ui_.min_cov->setValue(settings_.min_depth);
	ui_.details_cov_roi->setChecked(settings_.roi_low_cov);
	ui_.depth_calc->setChecked(settings_.recalculate_avg_depth);
	ui_.tool_details->setChecked(settings_.show_tool_details);
	ui_.omim_table->setChecked(settings_.show_omim_table);
	ui_.class_info->setChecked(settings_.show_class_details);
	ui_.language->setCurrentText(settings_.language);

	//buttons
	activateOkButtonIfValid();
}

void ReportDialog::setTargetRegionSelected(bool is_selected)
{
	if (!is_selected)
	{
		ui_.details_cov->setChecked(false);
		ui_.details_cov->setEnabled(false);

		ui_.details_cov_roi->setChecked(false);
		ui_.details_cov_roi->setEnabled(false);

		ui_.omim_table->setChecked(false);
		ui_.omim_table->setEnabled(false);
	}
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

	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void ReportDialog::updateCoverageSettings(int state)
{
	bool enabled = (state==Qt::Checked);
	ui_.min_cov->setEnabled(enabled);
	ui_.min_cov_label->setEnabled(enabled);
	ui_.depth_calc->setEnabled(enabled);
	ui_.details_cov_roi->setEnabled(enabled);
}
