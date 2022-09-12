#include "CohortAnalysisWidget.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include "GlobalServiceProvider.h"
#include "LoginManager.h"
#include <QMenu>

CohortAnalysisWidget::CohortAnalysisWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.search_btn, SIGNAL(clicked(bool)), this, SLOT(updateOutputTable()));
	connect(ui_.output, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(outputContextMenu(QPoint)));
	connect(ui_.copy_btn, SIGNAL(clicked(bool)), this, SLOT(outputToClipboard()));
}

QString CohortAnalysisWidget::baseQuery()
{
	QString query_str = "SELECT v.id, v.chr, v.start, v.end, dv.genotype FROM variant v, detected_variant dv WHERE dv.variant_id=v.id";

	//AF
	QString max_af = QString::number(ui_.filter_af->value()/100.0);
	query_str += " AND (v.gnomad IS NULL OR v.gnomad<=" + max_af + ")";

	//impact
	query_str += " AND ( 0 ";
	if (ui_.filter_impact_high->isChecked()) query_str += " OR v.coding LIKE '%:HIGH:%'";
	if (ui_.filter_impact_moderate->isChecked())
	{
		if (ui_.filter_predicted_pathogenic->isChecked())
		{
			query_str += " OR (v.coding LIKE '%:MODERATE:%' AND (v.cadd>=20 || v.spliceai>=0.5))";
		}
		else
		{
			query_str += " OR v.coding LIKE '%:MODERATE:%'";
		}
	}
	if (ui_.filter_impact_low->isChecked())
	{
		if (ui_.filter_predicted_pathogenic->isChecked())
		{
			query_str += " OR (v.coding LIKE '%:LOW:%' AND (v.cadd>=20 || v.spliceai>=0.5))";
		}
		else
		{
			query_str += " OR v.coding LIKE '%:LOW:%'";
		}
	}
	if (ui_.filter_impact_modifier->isChecked())
	{
		if (ui_.filter_predicted_pathogenic->isChecked())
		{
			query_str += " OR (v.coding LIKE '%:MODIFIER:%' AND (v.cadd>=20 || v.spliceai>=0.5))";
		}
		else
		{
			query_str += " OR v.coding LIKE '%:MODIFIER:%'";
		}
	}
	query_str += ")";

	return query_str;
}

void CohortAnalysisWidget::updateOutputTable()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	try
	{
		//not for restricted users
		LoginManager::checkRoleNotIn(QStringList{"user_restricted"});

		//init
		NGSD db;
		QString query_str = baseQuery();
		bool iheritance_is_recessive = ui_.filter_inheritance->currentText()=="recessive";
		int max_ngsd = ui_.filter_ngsd_count->value();

		//determine processed sample IDs
		QStringList ps_ids;
		QStringList parts = ui_.samples->toPlainText().replace("\n", " ").replace(",", " ").replace(";", " ").split(" ");
		foreach(QString ps, parts)
		{
			ps = ps.trimmed();
			if (ps.isEmpty()) continue;

			ps_ids << db.processedSampleId(ps);
		}

		//determine matching variants for each sample
		QHash<QByteArray, QStringList> gene2ps_hits;
		foreach(QString ps_id, ps_ids)
		{
			QHash<QByteArray, int> hits_by_gene;
			SqlQuery query = db.getQuery();
			query.exec(query_str + " AND dv.processed_sample_id= " + ps_id);
			while(query.next())
			{
				if (max_ngsd>0)
				{
					int var_count_ngsd = db.getValue("SELECT COUNT(dv.processed_sample_id) FROM detected_variant dv WHERE dv.variant_id='" + query.value(0).toString() + "'").toInt();
					if (var_count_ngsd > max_ngsd) continue;
				}

				Chromosome chr = query.value(1).toString();
				int start = query.value(2).toInt();
				int end = query.value(3).toInt();
				GeneSet genes = db.genesOverlapping(chr, start, end);
				foreach(const QByteArray& gene, genes)
				{
					int hits = query.value(4).toByteArray()=="hom" ? 2 : 1;
					hits_by_gene[gene] += hits;
				}
			}
			for(auto it = hits_by_gene.begin(); it!=hits_by_gene.end(); ++it)
			{
				const QByteArray& gene = it.key();

				if (!iheritance_is_recessive && it.value()>=1)
				{
					gene2ps_hits[gene] << ps_id;
				}
				if (iheritance_is_recessive && it.value()>=2)
				{
					gene2ps_hits[gene] << ps_id;
				}
			}
		}

		//ouput
		ui_.output->clearContents();
		ui_.output->setRowCount(0);
		ui_.output->setSortingEnabled(false);
		for(auto it = gene2ps_hits.begin(); it!=gene2ps_hits.end(); ++it)
		{
			QStringList ps_ids = it.value();
			if (ps_ids.count()>1)
			{
				int r = ui_.output->rowCount();
				ui_.output->setRowCount(ui_.output->rowCount()+1);

				const QByteArray& gene = it.key();
				addTableItem(r, 0, QString(gene));
				addTableItem(r, 1, ps_ids.count());
				int base_count = db.bestTranscript(db.geneId(gene)).regions().baseCount();
				addTableItem(r, 2, base_count);
				GeneInfo gene_info = db.geneInfo(gene);
				addTableItem(r, 3, gene_info.oe_lof);
				addTableItem(r, 4, gene_info.oe_mis);
				QStringList ps_list;
				foreach(QString ps_id, ps_ids)
				{
					ps_list << db.processedSampleName(ps_id);
				}
				addTableItem(r, 5, ps_list.join(", "));
			}
		}
		ui_.output->setSortingEnabled(true);
		ui_.output->sortByColumn(1);
		GUIHelper::resizeTableCells(ui_.output, 400);

		QApplication::restoreOverrideCursor();
	}
	catch(Exception& e)
	{
		GUIHelper::showException(this, e, "Cohort analysis error");
	}
}

void CohortAnalysisWidget::outputContextMenu(QPoint pos)
{
	//get item
	QTableWidgetItem* item = ui_.output->itemAt(pos);
	if (!item) return;

	//create context menu
	QMenu menu(ui_.output);
	QAction* action_gene = menu.addAction(QIcon(":/Icons/NGSD_gene.png"), "Open gene tab");

	//execute context menu
	QAction* action = menu.exec(ui_.output->viewport()->mapToGlobal(pos));
	if (!action) return;

	if (action==action_gene)
	{
		QString gene = ui_.output->item(item->row(), 0)->text();
		GlobalServiceProvider::openGeneTab(gene);
	}
}

void CohortAnalysisWidget::outputToClipboard()
{
	GUIHelper::copyToClipboard(ui_.output);
}


QTableWidgetItem* CohortAnalysisWidget::addTableItem(int row, int col, QString text)
{
	QTableWidgetItem* item = new QTableWidgetItem();

	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
	item->setData(Qt::EditRole, text);

	ui_.output->setItem(row, col, item);

	return item;
}

QTableWidgetItem* CohortAnalysisWidget::addTableItem(int row, int col, int value)
{
	QTableWidgetItem* item = new QTableWidgetItem();

	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
	item->setData(Qt::EditRole, value);

	ui_.output->setItem(row, col, item);

	return item;
}
