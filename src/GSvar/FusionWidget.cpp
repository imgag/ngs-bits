#include "FusionWidget.h"
#include "ui_FusionWidget.h"

#include <GUIHelper.h>
#include <Helper.h>
#include <TsvFile.h>
#include <VersatileFile.h>

FusionWidget::FusionWidget(const QString& filename, QWidget *parent) :
	QWidget(parent),
	filename_(filename),
	ui_(new Ui::FusionWidget)
{
	ui_->setupUi(this);

	loadFusionData();
}

FusionWidget::~FusionWidget()
{
	delete ui_;
}

void FusionWidget::loadFusionData()
{
	//load TSV file
	TsvFile fusion_data;
	fusion_data.load(filename_);

	//define columns
	//default columns
	column_names_.clear();
	numeric_columns_.clear();

	column_names_ << "gene1" << "gene2" << "strand1(gene/fusion)" << "strand2(gene/fusion)" << "breakpoint1" << "breakpoint2" << "site1" << "site2" << "type" << "split_reads1" << "split_reads2";
	numeric_columns_  << false << false << false << false << false << false << false << false << false << true << true;

	column_names_ << "discordant_mates" << "coverage1" << "coverage2" << "confidence" << "reading_frame" << "tags" << "retained_protein_domains";
	numeric_columns_  << true << true << true << false << false << false << false;

	column_names_ << "closest_genomic_breakpoint1" << "closest_genomic_breakpoint2" << "gene_id1" << "gene_id2" << "transcript_id1" << "transcript_id2" << "direction1" << "direction2";
	numeric_columns_  << false << false << false << false << false << false << false << false;

	column_names_ << "filters" << "fusion_transcript" << "peptide_sequence" << "read_identifiers";
	numeric_columns_  << false << false << false << false;

	int numeric_precision = 0;

	//determine col indices for table columns in tsv file
	QVector<int> column_indices;
	foreach (const QString& col_name, column_names_)
	{
		column_indices << fusion_data.columnIndex(col_name);
	}

	//create header
	ui_->fusions->setColumnCount(column_names_.size());
	for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
	{
		ui_->fusions->setHorizontalHeaderItem(col_idx, new QTableWidgetItem(column_names_.at(col_idx)));
	}

	//fill table widget with expression data
	ui_->fusions->setRowCount(fusion_data.rowCount());
	for(int row_idx=0; row_idx<fusion_data.rowCount(); ++row_idx)
	{
		QStringList row = fusion_data.row(row_idx);
		for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
		{
			if(numeric_columns_.at(col_idx))
			{
				// add numeric QTableWidgetItem
				QString value = row.at(column_indices.at(col_idx));
				if (value != "n/a" && !value.isEmpty())
				{
					ui_->fusions->setItem(row_idx, col_idx, GUIHelper::createTableItem(Helper::toDouble(value, "TSV column " + QString::number(col_idx), QString::number(row_idx)), numeric_precision));
				}
				else
				{
					ui_->fusions->setItem(row_idx, col_idx, GUIHelper::createTableItem(""));
				}
			}
			else
			{
				// add standard QTableWidgetItem
				ui_->fusions->setItem(row_idx, col_idx, GUIHelper::createTableItem(row.at(column_indices.at(col_idx))));
			}
		}
	}

	//hide vertical header
	ui_->fusions->verticalHeader()->setVisible(false);

	//enable sorting
	ui_->fusions->setSortingEnabled(true);

	//optimize table view
	GUIHelper::resizeTableCellWidths(ui_->fusions, 200);
	ui_->fusions->resizeRowsToContents();
}
