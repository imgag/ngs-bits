#include "CnvSearchWidget.h"
#include "Exceptions.h"
#include "Chromosome.h"
#include "Helper.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>

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
}

void CnvSearchWidget::setCoordinates(Chromosome chr, int start, int end)
{
	ui_.coordinates->setText(chr.strNormalized(true) + ":" + QString::number(start) + "-" + QString::number(end));
}

void CnvSearchWidget::search()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	try
	{
		//(0) parse input
		QString coords = ui_.coordinates->text().replace("-", " ").replace(":", " ").replace(",", "");
		QStringList parts = coords.split(QRegularExpression("\\W+"), QString::SkipEmptyParts);
		if (parts.count()!=3) THROW(ArgumentException, "Could not split coordinates in three parts! " + QString::number(parts.count()) + " parts found.");

		Chromosome chr(parts[0]);
		if (!chr.isValid()) THROW(ArgumentException, "Invalid chromosome given: " + parts[0]);
		int start = Helper::toInt(parts[1], "Start cooridinate");
		int end = Helper::toInt(parts[2], "End cooridinate");

		//(1) search matching CNVs
		QString query_str = "SELECT c.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as sample, ps.quality as quality_sample, sys.name_manufacturer as system, s.disease_group, s.disease_status, cs.caller, cs.quality as quality_callset, cs.quality_metrics as callset_metrics, c.chr, c.start, c.end, c.cn, c.quality_metrics as cnv_metrics, rc.class "
							"FROM cnv_callset cs, processed_sample ps, processing_system sys, sample s, cnv c LEFT JOIN report_configuration_cnv rc ON rc.cnv_id=c.id "
							"WHERE s.id=ps.sample_id AND sys.id=ps.processing_system_id AND c.cnv_callset_id=cs.id AND ps.id=cs.processed_sample_id ";
		QString operation = ui_.operation->currentText();
		if (operation=="overlaps")
		{
			query_str += " AND chr='" + chr.strNormalized(true) + "' AND ((" + QString::number(start) + ">=start AND " + QString::number(start) + "<=end) OR (start>=" + QString::number(start) + " AND start<=" + QString::number(end) + "))";

		}
		else if (operation=="contains")
		{
			query_str += " AND chr='" + chr.strNormalized(true) + "' AND start<=" + QString::number(start) + " AND end>=" + QString::number(end);
		}
		else THROW(ProgrammingException, "Invalid operation: " + operation);
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
		int col_cnv_metrics = table.columnIndex("cnv_metrics");
		for (int r=table.rowCount()-1; r>=0; --r)
		{
			const DBRow& row = table.row(r);
			QString value = row.value(col_cnv_metrics);
			QJsonDocument json = QJsonDocument::fromJson(value.toLatin1());

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
