#include "VariantSampleOverviewDialog.h"

#include "NGSD.h"
#include "GUIHelper.h"

VariantSampleOverviewDialog::VariantSampleOverviewDialog(const Variant& variant, QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);

	//get variant id
	NGSD db;
	QString variant_id = db.variantId(variant);

	//set variant text
	ui_.variant->setText(variant.toString(true));

	//populate table
	SqlQuery query = db.getQuery();
	query.exec("SELECT processed_sample_id, genotype FROM detected_variant WHERE variant_id=" + variant_id);

	//resize table
	ui_.table->setRowCount(query.size());

	//fill table
	int row = 0;
	while(query.next())
	{
		QString ps_id = query.value(0).toString();

		SampleData s_data = db.getSampleData(db.getValue("SELECT sample_id FROM processed_sample WHERE id=" + ps_id).toString());
		ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);
		DiagnosticStatusData diag_data = db.getDiagnosticStatus(ps_id);
		addItem(row, 0,  ps_data.name);
		addItem(row, 1,  s_data.name_external);
		addItem(row, 2,  s_data.quality + " / " + ps_data.quality);
		addItem(row, 3,  query.value(1).toString());
		addItem(row, 4, ps_data.processing_system);
		addItem(row, 5, ps_data.project_name);
		addItem(row, 6, s_data.disease_group);
		addItem(row, 7, s_data.disease_status);
		addItem(row, 8, diag_data.dagnostic_status);
		addItem(row, 9, diag_data.genes_causal);
		addItem(row, 10, diag_data.user);
		addItem(row, 11, s_data.comments);

		++row;
	}

	//sort by processed sample name
	ui_.table->sortByColumn(0);

	//resize table cols
	GUIHelper::resizeTableCells(ui_.table, 200);
}


void VariantSampleOverviewDialog::addItem(int r, int c, QString text)
{
	QTableWidgetItem* item = new QTableWidgetItem(text);
	ui_.table->setItem(r, c, item);
}

void VariantSampleOverviewDialog::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_.table);
}

//TODO: matrix overlap
