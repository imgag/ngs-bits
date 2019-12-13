#include "ProcessedSampleDataDeletionDialog.h"
#include "NGSD.h"
#include "BusyDialog.h"
#include <QMessageBox>

ProcessedSampleDataDeletionDialog::ProcessedSampleDataDeletionDialog(QWidget* parent, QStringList ids)
	: QDialog(parent)
	, ui_()
	, ps_ids_(ids)
{
	ui_.setupUi(this);
	connect(ui_.delete_btn, SIGNAL(clicked(bool)), this, SLOT(deleteData()));

	//fill table
	QStringList fields;
	fields	<< "ps.id"
			<< "CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as name"
			<< "s.name_external as name_external"
			<< "s.gender as gender"
			<< "s.tumor as is_tumor"
			<< "s.ffpe as is_ffpe"
			<< "ps.quality as quality"
			<< "sys.name_manufacturer as system_name"
			<< "sys.name_short as system_name_short"
			<< "sys.type as system_type"
			<< "p.name as project_name"
			<< "p.type as project_type"
			<< "r.name as run_name"
			<< "r.flowcell_type as run_flowcell_type"
			<< "r.recipe as run_recipe"
			<< "r.quality as run_quality"
			<< "s.disease_group as disease_group"
			<< "s.disease_status as disease_status";
	QStringList tables;
	tables	<< "sample s"
			<< "processing_system sys"
			<< "project p"
			<< "processed_sample ps LEFT JOIN sequencing_run r ON r.id=ps.sequencing_run_id"; //sequencing_run is optional
	QStringList conditions;
	conditions	<< "ps.sample_id=s.id"
				<< "ps.processing_system_id=sys.id"
				<< "ps.project_id=p.id"
				<< "(ps.id=" + ids.join(" OR ps.id=") + ")";

	DBTable ps_table = NGSD().createTable("processed_sample", "SELECT " + fields.join(", ") + " FROM " + tables.join(", ") +" WHERE " + conditions.join(" AND ") + " ORDER BY s.name ASC, ps.process_id ASC");
	ui_.sample_table->setData(ps_table);
}

void ProcessedSampleDataDeletionDialog::deleteData()
{
	//check consistency of selection
	if (!ui_.report_config->isChecked() && (ui_.var_small->isChecked() || ui_.var_cnv->isChecked()))
	{
		QMessageBox::question(this, "Deleting variants from NGSD", "You cannot delete variants and keep the report configuration.\nPlease correct the selection and try again!");
		return;
	}

	//check if the user is sure about deleting unrecoverable data
	if(ui_.report_config->isChecked() && QMessageBox::Yes!=QMessageBox::question(this, "Deleting report configuration from NGSD", "You are about to delete report configuration data from the NGSD.\nThis is permanent and cannot be undone.\n\nDo you really want to delete it?"))
	{
		return;
	}
	if (ui_.diag_status->isChecked() && QMessageBox::Yes!=QMessageBox::question(this, "Deleting diagnostic status from NGSD", "You are about to delete diagnostic status data from the NGSD.\nThis is permanent and cannot be undone.\n\nDo you really want to delete it?"))
	{
		return;
	}

	//delete data
	NGSD db;
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//report config first (it references variants)
	if (ui_.report_config->isChecked())
	{
		foreach(const QString& ps_id, ps_ids_)
		{
			int conf_id = db.reportConfigId(ps_id);
			if (conf_id!=-1)
			{
				db.deleteReportConfig(conf_id);
			}
		}
	}

	if (ui_.kasp->isChecked())
	{
		db.getQuery().exec("DELETE FROM kasp_status WHERE processed_sample_id=" + ps_ids_.join(" OR processed_sample_id="));
	}

	if (ui_.diag_status->isChecked())
	{
		db.getQuery().exec("DELETE FROM diag_status WHERE processed_sample_id=" + ps_ids_.join(" OR processed_sample_id="));
	}

	if (ui_.var_small->isChecked())
	{
		foreach(const QString& ps_id, ps_ids_)
		{
			db.deleteVariants(ps_id, VariantType::SNVS_INDELS);
		}
	}

	if (ui_.var_cnv->isChecked())
	{
		foreach(const QString& ps_id, ps_ids_)
		{
			db.deleteVariants(ps_id, VariantType::CNVS);
		}
	}

	QApplication::restoreOverrideCursor();
}
