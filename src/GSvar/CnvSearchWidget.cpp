#include "CnvSearchWidget.h"
#include "Exceptions.h"
#include "Chromosome.h"
#include "Helper.h"
#include "NGSHelper.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QClipboard>
#include <QAction>

CnvSearchWidget::CnvSearchWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, init_timer_(this, true)
	, db_()
{
	ui_.setupUi(this);
	QStringList callers;
	callers << "all";
	callers << db_.getEnum("cnv_callset", "caller");
	ui_.caller->addItems(callers);
	ui_.caller->setCurrentText("ClinCNV");
	connect(ui_.search_btn, SIGNAL(clicked(bool)), this, SLOT(search()));
	QAction* action = new QAction("Copy coordinates");
	connect(action, SIGNAL(triggered(bool)), this, SLOT(copyCoodinatesToClipboard()));
	connect(ui_.rb_chr_pos->group(), SIGNAL(buttonToggled(int,bool)), this, SLOT(changeSearchType()));
	ui_.table->addAction(action);
}

void CnvSearchWidget::setCoordinates(Chromosome chr, int start, int end)
{
	ui_.coordinates->setText(chr.strNormalized(true) + ":" + QString::number(start) + "-" + QString::number(end));

	ui_.rb_chr_pos->setChecked(true);
}

void CnvSearchWidget::search()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	// clear table
	ui_.table->clear();
	ui_.table->setColumnCount(0);

	try
	{
		//prepared SQL query
		QString query_str = "SELECT c.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as sample, ps.quality as quality_sample, sys.name_manufacturer as system, s.disease_group, s.disease_status, cs.caller, cs.quality as quality_callset, cs.quality_metrics as callset_metrics, c.chr, c.start, c.end, c.cn, (c.end-c.start)/1000.0 as size_kb, c.quality_metrics as cnv_metrics, rc.class "
							"FROM cnv_callset cs, processed_sample ps, processing_system sys, sample s, cnv c LEFT JOIN report_configuration_cnv rc ON rc.cnv_id=c.id "
							"WHERE s.id=ps.sample_id AND sys.id=ps.processing_system_id AND c.cnv_callset_id=cs.id AND ps.id=cs.processed_sample_id ";

		//(0) parse input and prepare query

		if (ui_.rb_chr_pos->isChecked())
		{
			// parse position
			Chromosome chr;
			int start, end;
			NGSHelper::parseRegion(ui_.coordinates->text(), chr, start, end);

			QString operation = ui_.operation->currentText();
			if (operation=="overlaps")
			{
				query_str += " AND chr='" + chr.strNormalized(true) + "' AND " + QString::number(start) + "<=end AND " + QString::number(end) + ">=start";
			}
			else if (operation=="contains")
			{
				query_str += " AND chr='" + chr.strNormalized(true) + "' AND start<=" + QString::number(start) + " AND end>=" + QString::number(end);
			}
			else THROW(ProgrammingException, "Invalid operation: " + operation);
		}
		else
		{
			// parse genes
			GeneSet genes;
			foreach (const QString& gene, ui_.le_genes->text().replace(";", " ").replace(",", "").split(QRegularExpression("\\W+"), QString::SkipEmptyParts))
			{
				QByteArray approved_gene_name = db_.geneToApproved(gene.toUtf8());
				if (approved_gene_name == "") THROW(ArgumentException, "Invalid gene name '" + gene + "' given!");
				genes.insert(approved_gene_name);
			}
			if (genes.count() == 0) THROW(ArgumentException, "No valid gene names provided!");

			// convert GeneSet to region
			BedFile region = db_.genesToRegions(genes, Transcript::ENSEMBL, "gene");
			region.extend(5000);
			region.sort();
			region.merge();

			QByteArrayList query_pos_overlap;
			for (int i = 0; i < region.count(); ++i)
			{
				QByteArray query_single_region;
				query_single_region += "(c.chr = \"" + region[i].chr().strNormalized(true) + "\" AND c.start <= " + QByteArray::number(region[i].end()) + " AND ";
				query_single_region += QByteArray::number(region[i].start()) + " <= c.end) ";
				query_pos_overlap.append(query_single_region);
			}

			//concatinate single pos querie
			query_str += " AND (" + query_pos_overlap.join("OR ") + ") ";
		}



		// parse copy number
		if (ui_.cn_0->isChecked() || ui_.cn_1->isChecked() || ui_.cn_2->isChecked() || ui_.cn_3->isChecked() || ui_.cn_4_plus->isChecked())
		{
			QStringList tmp;
			if (ui_.cn_0->isChecked()) tmp << "c.cn=0";
			if (ui_.cn_1->isChecked()) tmp << "c.cn=1";
			if (ui_.cn_2->isChecked()) tmp << "c.cn=2";
			if (ui_.cn_3->isChecked()) tmp << "c.cn=3";
			if (ui_.cn_4_plus->isChecked()) tmp << "c.cn>=4";
			query_str += " AND (" + tmp.join(" OR ") + ")";
		}
		if (ui_.q_ps_good->isChecked() || ui_.q_ps_medium->isChecked() || ui_.q_ps_bad->isChecked() || ui_.q_ps_na->isChecked())
		{
			QStringList tmp;
			if (ui_.q_ps_good->isChecked()) tmp << "ps.quality='good'";
			if (ui_.q_ps_medium->isChecked()) tmp << "ps.quality='medium'";
			if (ui_.q_ps_bad->isChecked()) tmp << "ps.quality='bad'";
			if (ui_.q_ps_na->isChecked()) tmp << "ps.quality='n/a'";
			query_str += " AND (" + tmp.join(" OR ") + ")";
		}
		if (ui_.q_cs_good->isChecked() || ui_.q_cs_medium->isChecked() || ui_.q_cs_bad->isChecked() || ui_.q_cs_na->isChecked())
		{
			QStringList tmp;
			if (ui_.q_cs_good->isChecked()) tmp << "cs.quality='good'";
			if (ui_.q_cs_medium->isChecked()) tmp << "cs.quality='medium'";
			if (ui_.q_cs_bad->isChecked()) tmp << "cs.quality='bad'";
			if (ui_.q_cs_na->isChecked()) tmp << "cs.quality='n/a'";
			query_str += " AND (" + tmp.join(" OR ") + ")";
		}
		if (ui_.caller->currentText()!="all")
		{
			query_str += " AND cs.caller='" + ui_.caller->currentText() + "'";
		}
		if (ui_.class_3_to_5->isChecked())
		{
			query_str += " AND (rc.class='3' OR rc.class='4' OR rc.class='5')";
		}
		query_str += " ORDER BY ps.id";


		//(1) search matching CNVs
		DBTable table = db_.createTable("cnv", query_str);

		//(2) process cnv callset metrics
		int max_iterations = ui_.iterations->value();
		int col_cs_metrics = table.columnIndex("callset_metrics");
		int col_caller = table.columnIndex("caller");
		for (int r=table.rowCount()-1; r>=0; --r)
		{
			const DBRow& row = table.row(r);
			QString value = row.value(col_cs_metrics);
			QJsonDocument json = QJsonDocument::fromJson(value.toLatin1());

			//filter by iterations
			if (row.value(col_caller)=="ClinCNV")
			{
				if (json.object().value("number of iterations").toString().toInt()>max_iterations)
				{
					table.removeRow(r);
					continue;
				}
			}

			//format value
			if (row.value(col_caller)=="ClinCNV")
			{
				QStringList values;
				values << "number of iterations: " + json.object().value("number of iterations").toString();
				values << "high-quality cnvs: " + json.object().value("high-quality cnvs").toString();
				values << "correlation to reference samples: " + json.object().value("mean correlation to reference samples").toString();
				table.setValue(r, col_cs_metrics, values.join(" "));
			}
			else if (row.value(col_caller)=="CnvHunter")
			{
				QStringList values;
				values << "cnvs: " + json.object().value("cnvs").toString();
				values << "correlation to reference samples: " + json.object().value("ref_correl").toString();
				table.setValue(r, col_cs_metrics, values.join(" "));
			}
		}

		//(3) process cnv metrics
		int min_regions = ui_.regions->value();
		int min_ll = ui_.ll->value();
		double min_size = ui_.size->value();
		int col_cnv_metrics = table.columnIndex("cnv_metrics");
		int col_size = table.columnIndex("size_kb");
		for (int r=table.rowCount()-1; r>=0; --r)
		{
			const DBRow& row = table.row(r);
			QString value = row.value(col_cnv_metrics);
			QJsonDocument json = QJsonDocument::fromJson(value.toLatin1());

			//filter by size
			if (min_size>0.0)
			{
				if (row.value(col_size).toDouble()<min_size)
				{
					table.removeRow(r);
					continue;
				}
			}

			//filter by regions
			if (json.object().value("regions").toString().toInt()<min_regions)
			{
				table.removeRow(r);
				continue;
			}

			//filter by log-likelihood
			if (row.value(col_caller)=="ClinCNV")
			{
				if (json.object().value("loglikelihood").toString().toInt()<min_ll)
				{
					table.removeRow(r);
					continue;
				}
			}

			//format value
			if (row.value(col_caller)=="ClinCNV")
			{
				QStringList values;
				values << "regions: " + json.object().value("regions").toString();
				values << "log-likelihood: " + json.object().value("loglikelihood").toString();
				table.setValue(r, col_cnv_metrics, values.join(" "));
			}
			else if (row.value(col_caller)=="CnvHunter")
			{
				QStringList values;
				values << "regions: " + json.object().value("regions").toString();
				values << "z-scores: " + json.object().value("region_zscores").toString();
				table.setValue(r, col_cnv_metrics, values.join(" "));
			}
		}

		//(4) show samples with CNVs in table
		ui_.table->setData(table);
		ui_.message->setText("Found " + QString::number(table.rowCount()) + " matching CNVs in NGSD.");
	}
	catch(Exception& e)
	{
		ui_.message->setText("Error: Search could not be performed:\t" + e.message());
		QMessageBox::warning(this, "CNV search", "Error: Search could not be performed:\n" + e.message());
	}

	QApplication::restoreOverrideCursor();
}

void CnvSearchWidget::delayedInitialization()
{
	if (ui_.coordinates->text().trimmed()!="")
	{
		search();
	}
}

void CnvSearchWidget::copyCoodinatesToClipboard()
{
	int c_chr = ui_.table->columnIndex("chr");
	int c_start = ui_.table->columnIndex("start");
	int c_end = ui_.table->columnIndex("end");

	QStringList coords;
	foreach(int row, ui_.table->selectedRows())
	{
		coords << ui_.table->item(row, c_chr)->text() + ":" + ui_.table->item(row, c_start)->text() + "-" + ui_.table->item(row, c_end)->text();
	}

	QApplication::clipboard()->setText(coords.join("\n"));
}

void CnvSearchWidget::changeSearchType()
{
	ui_.operation->setEnabled(ui_.rb_chr_pos->isChecked());
	ui_.coordinates->setEnabled(ui_.rb_chr_pos->isChecked());
	ui_.le_genes->setEnabled(ui_.rb_genes->isChecked());
}
