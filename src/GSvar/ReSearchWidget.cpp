#include "ReSearchWidget.h"
#include "LoginManager.h"
#include "GUIHelper.h"

ReSearchWidget::ReSearchWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.search_btn, SIGNAL(clicked()), this, SLOT(search()));
	ui_.re->fill(db_.createTable("repeat_expansion", "SELECT id, CONCAT(name, ' - ', region, ' ', repeat_unit) FROM repeat_expansion ORDER BY name ASC"), true);
}

void ReSearchWidget::search()
{
	//clear table
	ui_.table->clear();
	ui_.table->setColumnCount(0);
	ui_.message->clear();

	//no RE selected > do nothing
	QString reg_id = ui_.re->getCurrentId();
	if (reg_id.isEmpty()) return;

	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		//not for restricted users
		LoginManager::checkRoleNotIn(QStringList{"user_restricted"});

		//prepared SQL query
		QString query_str = "SELECT reg.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as sample, ps.quality as quality_sample, sys.name_manufacturer as system, s.disease_group, s.disease_status, s.id as 'HPO terms', ds.outcome, reg.filter, reg.allele1, reg.allele2, rc.causal, CONCAT(rc.comments, ' // ', rc.comments2) as report_config_comments"
							" FROM repeat_expansion_genotype reg LEFT JOIN report_configuration_re rc ON rc.repeat_expansion_genotype_id=reg.id, processed_sample ps LEFT JOIN diag_status ds ON ds.processed_sample_id=ps.id, processing_system sys, sample s, project p"
							" WHERE s.id=ps.sample_id AND sys.id=ps.processing_system_id AND reg.processed_sample_id=ps.id AND ps.project_id=p.id AND reg.repeat_expansion_id=" + reg_id;

		//RE size cutoff
		int cutoff = ui_.re_min_count->value();
		if (cutoff>0)
		{
			query_str += " AND (reg.allele1>="+QString::number(cutoff)+" OR reg.allele2>="+QString::number(cutoff)+")";
		}

		//restrict samples
		if (ui_.q_ps_good->isChecked() || ui_.q_ps_medium->isChecked() || ui_.q_ps_bad->isChecked() || ui_.q_ps_na->isChecked())
		{
			QStringList tmp;
			if (ui_.q_ps_good->isChecked()) tmp << "ps.quality='good'";
			if (ui_.q_ps_medium->isChecked()) tmp << "ps.quality='medium'";
			if (ui_.q_ps_bad->isChecked()) tmp << "ps.quality='bad'";
			if (ui_.q_ps_na->isChecked()) tmp << "ps.quality='n/a'";
			query_str += " AND (" + tmp.join(" OR ") + ")";
		}
		if (ui_.p_diagnostic->isChecked() || ui_.p_research->isChecked() || ui_.p_external->isChecked() || ui_.p_test->isChecked())
		{
			QStringList tmp;
			if (ui_.p_diagnostic->isChecked()) tmp << "p.type='diagnostic'";
			if (ui_.p_research->isChecked()) tmp << "p.type='research'";
			if (ui_.p_external->isChecked()) tmp << "p.type='external'";
			if (ui_.p_test->isChecked()) tmp << "p.type='test'";
			query_str += " AND (" + tmp.join(" OR ") + ")";
		}

		//execute
		DBTable table = db_.createTable("repeat_expansion_genotype", query_str);

		//add HPO terms
		int hpo_col_index = table.columnIndex("HPO terms");
		QStringList sample_ids = table.extractColumn(hpo_col_index);
		QStringList hpo_terms;
		foreach(const QString& sample_id, sample_ids)
		{
			hpo_terms << db_.samplePhenotypes(sample_id).toString();
		}
		table.setColumn(hpo_col_index, hpo_terms);

		//show data
		ui_.table->setData(table);
		ui_.table->showTextAsTooltip("report_config_comments");
		ui_.message->setText("Found " + QString::number(ui_.table->rowCount()) + " matching REs in NGSD.");

		QApplication::restoreOverrideCursor();
	}
	catch(Exception& e)
	{
		GUIHelper::showException(this, e, "RE search could not be performed");
	}
}
