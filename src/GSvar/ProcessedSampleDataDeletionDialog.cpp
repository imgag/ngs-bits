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

	NGSD db;

	DBTable ps_table = db.createTable("processed_sample", "SELECT " + fields.join(", ") + " FROM " + tables.join(", ") +" WHERE " + conditions.join(" AND ") + " ORDER BY s.name ASC, ps.process_id ASC");
	ui_.sample_table->setData(ps_table);

	//Activate option for somatic variants/cnvs/report configuration if ids contains matched tumor-control
	for(const auto& ps_id: ps_ids_)
	{
		if(db.getProcessedSampleData(ps_id).normal_sample_name != "")
		{
			ui_.somatic_report_config->setEnabled(true);
			ui_.somatic_var_small->setEnabled(true);
			ui_.somatic_var_cnv->setEnabled(true);
			break;
		}
	}

}

void ProcessedSampleDataDeletionDialog::deleteData()
{
	//check consistency of selection
	if (!ui_.report_config->isChecked() && (ui_.var_small->isChecked() || ui_.var_cnv->isChecked() || ui_.var_sv->isChecked()))
	{
		QMessageBox::information(this, "Deleting variants from NGSD", "You cannot delete variants and keep the report configuration.\nPlease correct the selection and try again!");
		return;
	}
	if(!ui_.somatic_report_config->isChecked() && (ui_.somatic_var_small->isChecked() || ui_.somatic_var_cnv->isChecked()))
	{
		QMessageBox::information(this, "Deleting somatic variants from NGSD", "You cannot delete somatic variants and keep the somatic report configuration.\nPlease correct the selection and try again!");
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

	//check if report config if finalized
	if (ui_.report_config->isChecked())
	{
		QStringList finalized_ps;
		foreach(const QString& ps_id, ps_ids_)
		{
			int conf_id = db.reportConfigId(ps_id);
			if (conf_id!=-1 && db.reportConfigIsFinalized(conf_id))
			{
				finalized_ps << db.processedSampleName(ps_id);
			}
		}
		if (!finalized_ps.isEmpty())
		{
			QMessageBox::warning(this, "Deleting report configuration", "The report configuration of the following processed samples is finalized and cannot be deleted:\n" + finalized_ps.join("\n"));
			return;
		}
	}

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

			db.getQuery().exec("DELETE FROM evaluation_sheet_data WHERE processed_sample_id=" + ps_id);
		}
	}

	//somatic report config first (it references som. variants)
	if (ui_.somatic_report_config->isChecked())
	{
		for(const auto& ps_tumor_id : ps_ids_)
		{
			QString ps_normal_id = matchedNormalPsID(db, ps_tumor_id);
			if(ps_normal_id == "") continue;

			int conf_id = db.somaticReportConfigId(ps_tumor_id, ps_normal_id);
			if(conf_id == -1) continue;

			db.deleteSomaticReportConfig(conf_id);
			emit somRepDeleted();
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

	//variants
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

	if (ui_.var_sv->isChecked())
	{
		foreach(const QString& ps_id, ps_ids_)
		{
			db.deleteVariants(ps_id, VariantType::SVS);
		}
	}

	//somatic variants
	if (ui_.somatic_var_small->isChecked())
	{
		for(const auto& ps_tumor_id : ps_ids_)
		{
			QString ps_normal_id = matchedNormalPsID(db, ps_tumor_id);
			if(ps_normal_id == "") continue;
			db.deleteSomaticVariants(ps_tumor_id, ps_normal_id, VariantType::SNVS_INDELS);
		}
	}

	if (ui_.somatic_var_cnv->isChecked())
	{
		for(const auto& ps_tumor_id : ps_ids_)
		{
			QString ps_normal_id = matchedNormalPsID(db, ps_tumor_id);
			if(ps_normal_id == "") continue;
			db.deleteSomaticVariants(ps_tumor_id, ps_normal_id, VariantType::CNVS);
		}
	}

	//processed sample
	if (ui_.processed_sample->isChecked())
	{
		foreach(const QString& ps_id, ps_ids_)
		{
			//delete gap data
			db.getQuery().exec("DELETE FROM gaps WHERE processed_sample_id=" + ps_id);

			//delete study data
			db.getQuery().exec("DELETE FROM study_sample WHERE processed_sample_id=" + ps_id);

			//delete merged processed samples
			db.getQuery().exec("DELETE FROM merged_processed_samples WHERE processed_sample_id='" + ps_id + "' OR merged_into='" + ps_id + "'");

			//delete analysis jobs
			QStringList analysis_job_ids = db.getValues("SELECT analysis_job_id FROM analysis_job_sample WHERE processed_sample_id='" + ps_id + "'");
			foreach(QString job_id, analysis_job_ids)
			{
				db.deleteAnalysis(job_id.toInt());
			}

			//delete QC data
			db.getQuery().exec("DELETE FROM processed_sample_qc WHERE processed_sample_id='" + ps_id + "'");

			//set referencing "normal sample" entries to NULL
			db.getQuery().exec("UPDATE `processed_sample` SET `normal_id`=NULL WHERE normal_id='" + ps_id + "'");

			//delete ancestry data
			db.getQuery().exec("DELETE FROM processed_sample_ancestry WHERE processed_sample_id='" + ps_id + "'");

			//delete processed sample
			try
			{
				db.getQuery().exec("DELETE FROM processed_sample WHERE id='" + ps_id + "'");
			}
			catch(DatabaseException& e)
			{
				QMessageBox::warning(this, "Error deleting processed sample", "Could not delete sample '" + db.processedSampleName(ps_id, false) + "'.\nProbably associated data is still present, which has to be deleted before.\n\nDatabase error:\n" + e.message());
				break;
			}
		}
	}

	QApplication::restoreOverrideCursor();

	QMessageBox::information(this, "Deleting processed sample data", "Data was deleted according to your selection.");
}
