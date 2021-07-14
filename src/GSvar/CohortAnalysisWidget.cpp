#include "CohortAnalysisWidget.h"
#include "NGSD.h"
#include <QMessageBox>

CohortAnalysisWidget::CohortAnalysisWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.search_btn, SIGNAL(clicked(bool)), this, SLOT(updateOutputTable()));
}

QString CohortAnalysisWidget::baseQuery()
{
	QString query_str = "SELECT v.id, v.chr, v.start, v.end, dv.genotype FROM variant v, detected_variant dv WHERE dv.variant_id=v.id";

	//AF
	QString max_af = QString::number(ui_.filter_af->value()/100.0);
	query_str += " AND (v.1000g IS NULL OR v.1000g<=" + max_af + ") AND (v.gnomad IS NULL OR v.gnomad<=" + max_af + ")";

	//impact
	query_str += " AND ( 0 ";
	if (ui_.filter_impact_high->isChecked()) query_str += " OR v.coding LIKE '%:HIGH:%'";
	if (ui_.filter_impact_moderate->isChecked()) query_str += " OR v.coding LIKE '%:MODERATE:%'";
	if (ui_.filter_impact_low->isChecked()) query_str += " OR v.coding LIKE '%:LOW:%'";
	if (ui_.filter_impact_modifier->isChecked()) query_str += " OR v.coding LIKE '%:MODIFIER:%'";
	query_str += ")";

	return query_str;
}

void CohortAnalysisWidget::updateOutputTable()
{
	try
	{
		NGSD db;

		//init
		QString query_str = baseQuery();
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
		QHash<QByteArray, int> gene_single_hits;
		QHash<QByteArray, int> gene_double_hits;
		foreach(QString ps_id, ps_ids)
		{
			QHash<QByteArray, int> hits_by_gene;
			SqlQuery query = db.getQuery();
			query.exec(query_str + " AND dv.processed_sample_id= " + ps_id);
			while(query.next())
			{
				int var_count_ngsd = db.getValue("SELECT COUNT(dv.processed_sample_id) FROM detected_variant dv WHERE dv.variant_id='" + query.value(0).toString() + "'").toInt();
				if (max_ngsd>0 && var_count_ngsd < max_ngsd)
				{
					GeneSet genes = db.genesOverlapping(query.value(1).toString(), query.value(2).toInt(), query.value(3).toInt());
					foreach(const QByteArray& gene, genes)
					{
						int hits = query.value(4).toByteArray()=="hom" ? 2 : 1;
						hits_by_gene[gene] += hits;
					}
				}
			}
			for(auto it = hits_by_gene.begin(); it!=hits_by_gene.end(); ++it)
			{
				const QByteArray& gene = it.key();
				int hit_count = it.value();
				if (hit_count>=1)
				{
					gene_single_hits[gene] += 1;
				}
				if (hit_count>=2)
				{
					gene_double_hits[gene] += 1;
				}
			}
		}

		//TODO: try removing low-confidence region variants

		qDebug() << "double hits:";
		for(auto it = gene_double_hits.begin(); it!=gene_double_hits.end(); ++it)
		{
			const QByteArray& gene = it.key();
			int hit_count = it.value();
			if (hit_count>1)
			{
				qDebug() << gene << hit_count;
			}
		}

		qDebug() << "single hits:";
		for(auto it = gene_single_hits.begin(); it!=gene_single_hits.end(); ++it)
		{
			const QByteArray& gene = it.key();
			int hit_count = it.value();
			if (hit_count>1)
			{
				qDebug() << gene << hit_count;
			}
		}
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Error", "Could not perform cohort analysis:\n" + e.message());
	}
}
