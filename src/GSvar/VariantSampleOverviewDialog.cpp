#include "VariantSampleOverviewDialog.h"

#include "NGSD.h"
#include "GUIHelper.h"

VariantSampleOverviewDialog::VariantSampleOverviewDialog(const Variant& variant, QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.similarity, SIGNAL(clicked(bool)), this, SLOT(calculateSimilarity()));

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

void VariantSampleOverviewDialog::calculateSimilarity()
{
	NGSD db;
	SqlQuery query = db.getQuery();

	//get sample names and variant lists
	QStringList ps_names;
	QList<QSet<QString>> ps_vars;
	for(int i=0; i<ui_.table->rowCount(); ++i)
	{
		QString ps = ui_.table->item(i, 0)->text();
		ps_names << ps;

		QString ps_id = db.processedSampleId(ps);
		ps_vars << db.getValues("SELECT variant_id FROM detected_variant WHERE processed_sample_id=" + ps_id).toSet();
	}

	//calcualte and show overlap
	QTableWidget* table = new QTableWidget(this);
	table->setMinimumSize(700, 550);
	table->setEditTriggers(QTableWidget::NoEditTriggers);
	table->setSortingEnabled(true);
	table->setColumnCount(6);
	table->setHorizontalHeaderLabels(QStringList() << "s1" << "#variants s1" << "s2" << "#variants s2" << "variant overlap" << "variant overlap %");
	int row = 0;
	for (int i=0; i<ps_vars.count(); ++i)
	{
		for (int j=i+1; j<ps_vars.count(); ++j)
		{
			table->setRowCount(table->rowCount()+1);
			table->setItem(row, 0, new QTableWidgetItem(ps_names[i]));
			int c_s1 = ps_vars[i].count();
			table->setItem(row, 1, new QTableWidgetItem(QString::number(c_s1)));
			table->setItem(row, 2, new QTableWidgetItem(ps_names[j]));
			int c_s2 = ps_vars[j].count();
			table->setItem(row, 3, new QTableWidgetItem(QString::number(c_s2)));
			int overlap = QSet<QString>(ps_vars[i]).intersect(ps_vars[j]).count();
			table->setItem(row, 4, new QTableWidgetItem(QString::number(overlap)));
			double overlap_perc = 100.0 * overlap / (double)std::min(c_s1, c_s2);
			auto item = new QTableWidgetItem();
			item->setData(Qt::DisplayRole, overlap_perc);
			table->setItem(row, 5, item);

			++row;
		}
	}

	table->sortByColumn(5, Qt::DescendingOrder);

	//show results
	GUIHelper::showWidgetAsDialog(table, "Sample correlation based on rare variants from NGSD", false);
}
