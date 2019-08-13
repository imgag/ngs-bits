#include "ReportDialog.h"
#include "GUIHelper.h"
#include <QTableWidgetItem>
#include <QPushButton>
#include <QMenu>


ReportDialog::ReportDialog(ReportSettings settings, const VariantList& variants, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, settings_(settings)
	, variants_(variants)
{
	ui_.setupUi(this);

	//context menu
	connect(ui_.vars, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

	//disable ok button when no outcome is set
	connect(ui_.diag_status, SIGNAL(outcomeChanged(QString)), this, SLOT(outcomeChanged()));

	//enable/disable low-coverage settings
	connect(ui_.details_cov, SIGNAL(stateChanged(int)), this, SLOT(updateCoverageSettings(int)));

	updateGUI();
}

void ReportDialog::updateGUI()
{
	//diagnostic status
	ui_.diag_status->setStatus(settings_.diag_status);
	outcomeChanged();

	//variants
	int geno_idx = variants_.getSampleHeader().infoByStatus(true).column_index;
	QList<int> selected_variants = settings_.variantIndices(VariantType::SNVS_INDELS, true);
	ui_.vars->setRowCount(selected_variants.count());
	int row = 0;
	foreach(int i, selected_variants)
	{
		const Variant& variant = variants_[i];
		ui_.vars->setItem(row, 0, new QTableWidgetItem(QString(variant.chr().str())));
		ui_.vars->setItem(row, 1, new QTableWidgetItem(QString::number(variant.start())));
		ui_.vars->setItem(row, 2, new QTableWidgetItem(QString::number(variant.end())));
		ui_.vars->setItem(row, 3, new QTableWidgetItem(variant.ref(), 0));
		ui_.vars->setItem(row, 4, new QTableWidgetItem(variant.obs(), 0));
		ui_.vars->setItem(row, 5, new QTableWidgetItem(variant.annotations().at(geno_idx), 0));

		for (int j=6; j<ui_.vars->horizontalHeader()->count(); ++j)
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

ReportSettings ReportDialog::settings()
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

	return settings_;
}

void ReportDialog::outcomeChanged()
{
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ui_.diag_status->status().outcome!="n/a");
}

void ReportDialog::showContextMenu(QPoint pos)
{
	int row = ui_.vars->rowAt(pos.y());
	if (row==-1) return;

	QMenu menu(ui_.vars);
	menu.addAction("Copy variant to diagnostic status");

	QAction* action = menu.exec(ui_.vars->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	QString text = action->text();
	if (text=="Copy variant to diagnostic status")
	{
		DiagnosticStatusData status = ui_.diag_status->status();
		status.genes_causal = ui_.vars->item(row, 8)->text();
		status.comments = ui_.vars->item(row, 9)->text();
		ui_.diag_status->setStatus(status);
	}
}

void ReportDialog::updateCoverageSettings(int state)
{
	bool enabled = (state==Qt::Checked);
	ui_.min_cov->setEnabled(enabled);
	ui_.min_cov_label->setEnabled(enabled);
	ui_.depth_calc->setEnabled(enabled);
	ui_.details_cov_roi->setEnabled(enabled);
}
