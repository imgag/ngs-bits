#include <QDebug>
#include <QMenu>
#include "TumorOnlyReportDialog.h"
#include "ui_TumorOnlyReportDialog.h"
#include "SomaticReportHelper.h"
#include "GUIHelper.h"


TumorOnlyReportDialog::TumorOnlyReportDialog(const VariantList& variants, TumorOnlyReportWorkerConfig& config, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::TumorOnlyReportDialog)
	, config_(config)
	, variants_(variants)
{
	ui->setupUi(this);

	connect( this, SIGNAL(accepted()), this, SLOT(writeBackSettings()) );
	connect( ui->snvs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(rightClickMenu(QPoint)) );


	if(config_.roi.isValid())
	{
		ui->include_cov_per_gap->setEnabled(true);
		ui->include_cov_per_gap->setChecked(true);

		ui->include_exon_number->setEnabled(true);
		ui->include_exon_number->setChecked(true);
	}

	//fill variants widget
	int i_gene = variants_.annotationIndexByName("gene");
	int i_var_type = variants_.annotationIndexByName("variant_type");
	int i_tum_af = variants_.annotationIndexByName("tumor_af");
	int i_tum_dp = variants_.annotationIndexByName("tumor_dp");
	ui->snvs->setRowCount( variants_.count() );
	for(int i=0;i< variants_.count(); ++i)
	{
		if(!config.filter_result.passing(i)) ui->snvs->hideRow(i);

		const Variant& var = variants_[i];

		QTableWidgetItem* chr_item = GUIHelper::createTableItem(var.chr().strNormalized("chr"));
		QTableWidgetItem* start = GUIHelper::createTableItem(QString::number(var.start()));
		QTableWidgetItem* end = GUIHelper::createTableItem(QString::number(var.end()));
		QTableWidgetItem* ref= GUIHelper::createTableItem(var.ref());
		QTableWidgetItem* obs = GUIHelper::createTableItem(var.obs());
		QTableWidgetItem* tumor_af = GUIHelper::createTableItem(var.annotations()[i_tum_af]);
		QTableWidgetItem* tumor_depth = GUIHelper::createTableItem(var.annotations()[i_tum_dp]);
		QTableWidgetItem* gene = GUIHelper::createTableItem(var.annotations()[i_gene]);
		QTableWidgetItem* var_type = GUIHelper::createTableItem(var.annotations()[i_var_type]);


		QTableWidgetItem* checkbox = new QTableWidgetItem("");
		checkbox->setCheckState(Qt::Unchecked);
		checkbox->setFlags(checkbox->flags() & (~Qt::ItemIsEditable) );

		ui->snvs->setItem(i, 0, checkbox);
		ui->snvs->setItem(i, 1, chr_item);
		ui->snvs->setItem(i, 2, start);
		ui->snvs->setItem(i, 3, end);
		ui->snvs->setItem(i, 4, ref);
		ui->snvs->setItem(i, 5, obs);
		ui->snvs->setItem(i, 6, tumor_af);
		ui->snvs->setItem(i, 7, tumor_depth);

		ui->snvs->setItem(i, 8, gene);
		ui->snvs->setItem(i, 9, var_type);


	}
	GUIHelper::resizeTableCellWidths(ui->snvs);
	GUIHelper::resizeTableCellHeightsToFirst(ui->snvs);
}

TumorOnlyReportDialog::~TumorOnlyReportDialog()
{
	delete ui;
}

void TumorOnlyReportDialog::writeBackSettings()
{
	config_.include_coverage_per_gap = ui->include_cov_per_gap->isChecked();
	config_.include_exon_number_per_gap = ui->include_exon_number->isChecked();

	for(int i=0; i<ui->snvs->rowCount(); ++i)
	{
		if(ui->snvs->isRowHidden(i)) continue;

		if(ui->snvs->item(i,0)->checkState() == Qt::CheckState::Unchecked)
		{
			config_.filter_result.flags()[i] = false;
		}
	}
}


void TumorOnlyReportDialog::rightClickMenu(QPoint p)
{
	//make sure a row was clicked
	int row = ui->snvs->indexAt(p).row();
	if (row==-1) return;

	QMenu menu(this);
	menu.addAction(QIcon(":/Icons/box_checked.png") , "select all");
	menu.addAction(QIcon(":/Icons/box_unchecked.png") , "deselect all");
	menu.addAction(QIcon(":/Icons/box_checked.png") , "select highlighted");
	menu.addAction(QIcon(":/Icons/box_unchecked.png") , "deselect higlighted");

	//exec menu
	QAction* action = menu.exec(ui->snvs->viewport()->mapToGlobal(p));

	if(action==nullptr) return;

	QString action_text = action->text();

	for(const auto index : ui->snvs->selectionModel()->selectedRows())
	{
		if(action_text == "deselect higlighted")
		{
			ui->snvs->item(index.row(), 0)->setCheckState(Qt::CheckState::Unchecked);
		}
		else if(action_text == "select highlighted")
		{
			ui->snvs->item(index.row(), 0)->setCheckState(Qt::CheckState::Checked);
		}
		else if(action_text == "select all")
		{
			for (int i = 0; i < ui->snvs->rowCount(); ++i)
			{
				ui->snvs->item(i, 0)->setCheckState( Qt::Checked );
			}
		}
		else if(action_text == "deselect all")
		{
			for (int i = 0; i < ui->snvs->rowCount(); ++i)
			{
				ui->snvs->item(i, 0)->setCheckState( Qt::Unchecked );
			}
		}

	}
}

