#include "VariantWidget.h"

#include "NGSD.h"
#include "GUIHelper.h"
#include <QDialog>
#include <QMessageBox>
#include <QAction>

VariantWidget::VariantWidget(const Variant& variant, QWidget *parent)
	: QWidget(parent)
	, ui_()
	, init_timer_(this, true)
	, variant_(variant)
{
	ui_.setupUi(this);
	connect(ui_.similarity, SIGNAL(clicked(bool)), this, SLOT(calculateSimilarity()));
	connect(ui_.copy_btn, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
	connect(ui_.update_btn, SIGNAL(clicked(bool)), this, SLOT(updateGUI()));
	connect(ui_.transcripts, SIGNAL(linkActivated(QString)), this, SIGNAL(openGeneTab(QString)));

	//add sample table context menu entries
	QAction* action = new QAction(QIcon(":/Icons/NGSD_sample.png"), "Open processed sample tab", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openProcessedSampleTab()));
}

void VariantWidget::updateGUI()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//get variant id
	NGSD db;
	QString variant_id = db.variantId(variant_);

	//variant base info
	ui_.variant->setText(variant_.toString());

	SqlQuery query = db.getQuery();
	query.exec("SELECT * FROM variant WHERE id=" + variant_id);
	query.next();
	ui_.af_tg->setText(query.value("1000g").toString());
	ui_.af_gnomad->setText(query.value("gnomad").toString());
	ui_.comments->setText(query.value("comment").toString());

	//transcripts
	QStringList lines;
	QList<VariantTranscript> transcripts = Variant::parseTranscriptString(query.value("coding").toByteArray(), true);
	foreach(const VariantTranscript& trans, transcripts)
	{
		lines << "<a href=\"" + trans.gene + "\">" + trans.gene + "</a> " + trans.id + ": " + trans.type + " " + trans.hgvs_c + " " + trans.hgvs_p;
	}
	ui_.transcripts->setText(lines.join("<br>"));
	//classification
	ClassificationInfo class_info = db.getClassification(variant_);
	ui_.classification->setText(class_info.classification);
	ui_.classification_comment->setText(class_info.comments);

	//samples table
	{
		SqlQuery query = db.getQuery();
		query.exec("SELECT processed_sample_id, genotype FROM detected_variant WHERE variant_id=" + variant_id);

		//resize table
		ui_.table->setRowCount(query.size());

		//fill samples table
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
			QStringList pho_list;
			foreach(const Phenotype& pheno, s_data.phenotypes)
			{
				pho_list << pheno.toString();
			}
			addItem(row, 8, pho_list.join("; "));
			addItem(row, 9, diag_data.dagnostic_status);
			addItem(row, 10, diag_data.user);
			addItem(row, 11, s_data.comments);

			//get causal genes from report config
			GeneSet genes_causal;
			SqlQuery query3 = db.getQuery();
			query3.exec("SELECT v.gene FROM variant v, report_configuration rc, report_configuration_variant rcv WHERE v.id=rcv.variant_id AND rcv.report_configuration_id=rc.id AND rcv.type='diagnostic variant' AND rcv.causal=1 AND rc.processed_sample_id=" + ps_id);
			while(query3.next())
			{
				genes_causal << query3.value(0).toByteArray().split(',');
			}
			addItem(row, 12, genes_causal.join(','));

			//get candidate genes from report config
			GeneSet genes_candidate;
			SqlQuery query4 = db.getQuery();
			query4.exec("SELECT v.gene FROM variant v, report_configuration rc, report_configuration_variant rcv WHERE v.id=rcv.variant_id AND rcv.report_configuration_id=rc.id AND rcv.type='candidate variant' AND rc.processed_sample_id=" + ps_id);
			while(query4.next())
			{
				genes_candidate << query4.value(0).toByteArray().split(',');
			}
			addItem(row, 13, genes_candidate.join(','));

			++row;
		}

		//sort by processed sample name
		ui_.table->sortByColumn(0);

		//resize table cols
		GUIHelper::resizeTableCells(ui_.table, 200);
	}

	QApplication::restoreOverrideCursor();
}

void VariantWidget::delayedInitialization()
{
	updateGUI();
}


void VariantWidget::addItem(int r, int c, QString text)
{
	QTableWidgetItem* item = new QTableWidgetItem(text);
	ui_.table->setItem(r, c, item);
}

void VariantWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_.table);
}

void VariantWidget::calculateSimilarity()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	NGSD db;

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

	QApplication::restoreOverrideCursor();

	//show results
	auto dlg = GUIHelper::createDialog(table, "Sample correlation based on rare variants from NGSD");
	dlg->exec();
}

QList<int> VariantWidget::selectedRows() const
{
	QSet<int> set;

	foreach(QTableWidgetItem* item, ui_.table->selectedItems())
	{
		set << item->row();
	}

	return set.toList();
}

void VariantWidget::openProcessedSampleTab()
{
	QList<int> rows = selectedRows();
	foreach(int row, rows)
	{
		QString ps = ui_.table->item(row, 0)->text();
		emit openProcessedSampleTab(ps);
	}
}

