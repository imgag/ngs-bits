#include "Exceptions.h"
#include "ReportDialog.h"
#include "NGSD.h"
#include "Log.h"
#include "GUIHelper.h"
#include <QTableWidgetItem>
#include <QBitArray>
#include <QPushButton>
#include <QMenu>


ReportDialog::ReportDialog(QString filename, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, filename_(filename)
	, labels_()
{
	ui_.setupUi(this);

	//variant header
	labels_ << "" << "chr" << "start" << "end" << "ref" << "obs" << "NGSD_hom" << "NGSD_het" << "genotype" << "gene" << "variant_type" << "coding_and_splicing";
	ui_.vars->setColumnCount(labels_.count());
	ui_.vars->setHorizontalHeaderLabels(labels_);

	//contect menu
	ui_.vars->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_.vars, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

	//disable ok button when no outcome is set
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	connect(ui_.diag_status, SIGNAL(outcomeChanged(QString)), this, SLOT(outcomeChanged(QString)));

	//set diagnostic status
	NGSD db;
	QString processed_sample_id = db.processedSampleId(filename_);
	DiagnosticStatusData diag_status = db.getDiagnosticStatus(processed_sample_id);
	ui_.diag_status->setStatus(diag_status);

	//enable/disable low-coverage settings
	connect(ui_.details_cov, SIGNAL(stateChanged(int)), this, SLOT(updateCoverageSettings(int)));
}

void ReportDialog::addVariants(const VariantList& variants, const QBitArray& visible)
{
	int class_idx = variants.annotationIndexByName("classification", true, false);

	ui_.vars->setRowCount(visible.count(true));
	int row = 0;
	for (int i=0; i<variants.count(); ++i)
	{
		if (!visible[i]) continue;

		const Variant& variant = variants[i];
		ui_.vars->setItem(row, 0, new QTableWidgetItem(""));
		ui_.vars->item(row, 0)->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		ui_.vars->item(row, 0)->setCheckState(Qt::Checked);
		if (class_idx!=-1 && variant.annotations()[class_idx]=="1")
		{
			ui_.vars->item(row, 0)->setCheckState(Qt::Unchecked);
			ui_.vars->item(row, 0)->setToolTip("Unchecked because of classification '1'.");
		}
		ui_.vars->item(row, 0)->setData(Qt::UserRole, i);
		ui_.vars->setItem(row, 1, new QTableWidgetItem(QString(variant.chr().str())));
		ui_.vars->setItem(row, 2, new QTableWidgetItem(QString::number(variant.start())));
		ui_.vars->setItem(row, 3, new QTableWidgetItem(QString::number(variant.end())));
		ui_.vars->setItem(row, 4, new QTableWidgetItem(variant.ref(), 0));
		ui_.vars->setItem(row, 5, new QTableWidgetItem(variant.obs(), 0));
		for (int j=6; j<labels_.count(); ++j)
		{
			int index = variants.annotationIndexByName(labels_[j], true, false);
			if (index==-1 && labels_[j]!="genotype")
			{
				THROW(ArgumentException, "Report dialog: Could not find variant annotation '" + labels_[j] + "'!");
			}
			else if (index==-1 && labels_[j]=="genotype")
			{
				ui_.vars->setItem(row, j, new QTableWidgetItem("n/a"));
			}
			else
			{
				ui_.vars->setItem(row, j, new QTableWidgetItem(variant.annotations().at(index), 0));
			}
		}

		++row;
	}

	GUIHelper::resizeTableCells(ui_.vars);
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

ReportSettings ReportDialog::settings() const
{
	ReportSettings output;

	//diag status
	output.diag_status = ui_.diag_status->status();

	//variant indices
	for (int i=0; i<ui_.vars->rowCount(); ++i)
	{
		QTableWidgetItem* item = ui_.vars->item(i, 0);
		if (item->checkState()==Qt::Checked)
		{
			output.variants_selected.append(item->data(Qt::UserRole).toInt());
		}
	}

	//settings
	output.show_coverage_details = ui_.details_cov->isChecked();
	output.min_depth = ui_.min_cov->value();
	output.roi_low_cov = ui_.details_cov_roi->isChecked();
	output.recalculate_avg_depth = ui_.depth_calc->isChecked();
	output.show_tool_details = ui_.tool_details->isChecked();
	output.show_omim_table = ui_.omim_table->isChecked();
	output.show_class_details = ui_.class_info->isChecked();

	return output;
}

void ReportDialog::outcomeChanged(QString text)
{
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(text!="n/a");
}

void ReportDialog::showContextMenu(QPoint pos)
{
	int row = ui_.vars->rowAt(pos.y());

	QMenu menu(ui_.vars);
	if (row!=-1)
	{
		menu.addAction("Copy variant to diagnostic status");
		menu.addSeparator();
	}
	menu.addAction(QIcon(":/Icons/box_checked.png"), "select all");
	menu.addAction(QIcon(":/Icons/box_unchecked.png"), "unselect all");

	QAction* action = menu.exec(ui_.vars->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	QByteArray text = action->text().toLatin1();
	if (text=="select all")
	{
		for (int i=0; i<ui_.vars->rowCount(); ++i)
		{
			ui_.vars->item(i, 0)->setCheckState(Qt::Checked);
		}
	}
	if (text=="unselect all")
	{
		for (int i=0; i<ui_.vars->rowCount(); ++i)
		{
			ui_.vars->item(i, 0)->setCheckState(Qt::Unchecked);
		}
	}
	if (text=="Copy variant to diagnostic status")
	{
		DiagnosticStatusData status = ui_.diag_status->status();
		status.genes_causal = ui_.vars->item(row, 9)->text();
		status.comments = ui_.vars->item(row, 10)->text();
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
