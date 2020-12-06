#include "ReportDialog.h"
#include "GUIHelper.h"
#include "DiseaseInfoWidget.h"
#include "SampleDiseaseInfoWidget.h"
#include <QTableWidgetItem>
#include <QMenu>


ReportDialog::ReportDialog(QString ps, ReportSettings& settings, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs, QString target_region, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, ps_(ps)
	, settings_(settings)
	, variants_(variants)
	, cnvs_(cnvs)
	, svs_(svs)
	, roi_file_(target_region.trimmed())
	, roi_()
{
	ui_.setupUi(this);
	initGUI();

	//variant types
	connect(ui_.report_type, SIGNAL(currentTextChanged(QString)), this, SLOT(updateGUI()));

	//disable ok button if there is a problem
	connect(ui_.meta_data_check_btn, SIGNAL(clicked(bool)), this, SLOT(checkMetaData()));
	connect(ui_.report_type, SIGNAL(currentTextChanged(QString)), this, SLOT(activateOkButtonIfValid()));

	//enable/disable low-coverage settings
	connect(ui_.details_cov, SIGNAL(stateChanged(int)), this, SLOT(updateGUI()));

	//write settings if accepted
	connect(this, SIGNAL(accepted()), this, SLOT(writeBackSettings()));

	//handle ROI
	if (roi_file_!="")
	{
		roi_.load(roi_file_);
	}

	updateGUI();
}

void ReportDialog::checkMetaData()
{
	//clear
	ui_.meta_data_check_output->clear();
	ui_.meta_data_edit_btn->setEnabled(false);

	//check
	QString ps_id = db_.processedSampleId(ps_);
	QHash<QString, QStringList> errors = db_.checkMetaData(ps_id, variants_, cnvs_, svs_);
	QStringList sample_names = errors.keys();

	//show messages
	QStringList display_messages;
	foreach(QString sample_name, sample_names)
	{
		QStringList sample_messages = errors[sample_name];
		foreach(QString sample_message, sample_messages)
		{
			display_messages << sample_name + ": " + sample_message;
		}
	}
	ui_.meta_data_check_output->setText("<font color='red'>" + display_messages.join("<br>") + "</font>");

	//add edit menu entries
	QMenu* menu = new QMenu(this);
	foreach(QString sample_name, sample_names)
	{
		QAction* action = menu->addAction(sample_name + ": disease group/status");
		action->setData(sample_name);
		connect(action, SIGNAL(toggled(bool)), this, SLOT(editDiseaseGroupStatus()));

		action = menu->addAction(sample_name + ": disease details (HPO, OMIM, Optha, ...)");
		action->setData(sample_name);
		connect(action, SIGNAL(toggled(bool)), this, SLOT(editDiseaseDetails()));
	}
	ui_.meta_data_edit_btn->setMenu(menu);
	ui_.meta_data_edit_btn->setEnabled(!menu->isEmpty());

	//update button box
	activateOkButtonIfValid();
}


void ReportDialog::initGUI()
{
	//report types
	ui_.report_type->addItem("");
	ui_.report_type->addItems(ReportVariantConfiguration::getTypeOptions());

	//settings
	ui_.details_cov->setChecked(settings_.show_coverage_details);
	ui_.min_cov->setValue(settings_.min_depth);
	ui_.details_cov_roi->setChecked(settings_.roi_low_cov);
	ui_.depth_calc->setChecked(settings_.recalculate_avg_depth);
	ui_.tool_details->setChecked(settings_.show_tool_details);
	ui_.omim_table->setChecked(settings_.show_omim_table);
	ui_.class_info->setChecked(settings_.show_class_details);
	ui_.language->setCurrentText(settings_.language);

	//no ROI > no roi options
	if (roi_file_=="")
	{
		ui_.details_cov->setChecked(false);
		ui_.details_cov->setEnabled(false);

		ui_.depth_calc->setChecked(false);
		ui_.depth_calc->setEnabled(false);

		ui_.details_cov_roi->setChecked(false);
		ui_.details_cov_roi->setEnabled(false);

		ui_.min_cov_label->setEnabled(false);
		ui_.min_cov->setEnabled(false);

		ui_.omim_table->setChecked(false);
		ui_.omim_table->setEnabled(false);
	}
}

void ReportDialog::updateGUI()
{
	checkMetaData();

	//init
	ui_.vars->setRowCount(0);
	int row = 0;

	//add small variants
	int geno_idx = variants_.getSampleHeader().infoByStatus(true).column_index;
	int gene_idx = variants_.annotationIndexByName("gene");
	int class_idx = variants_.annotationIndexByName("classification");
	foreach(int i, settings_.report_config->variantIndices(VariantType::SNVS_INDELS, true, type()))
	{
		const Variant& variant = variants_[i];
		if (roi_file_!="" && !roi_.overlapsWith(variant.chr(), variant.start(), variant.end())) continue;
		const ReportVariantConfiguration& var_conf = settings_.report_config->get(VariantType::SNVS_INDELS,i);

		ui_.vars->setRowCount(ui_.vars->rowCount()+1);
		ui_.vars->setItem(row, 0, new QTableWidgetItem(var_conf.report_type + (var_conf.causal ? " (causal)" : "")));
		QString tmp = variant.toString(false, 30) + " (" + variant.annotations().at(geno_idx) + ")";
		ui_.vars->setItem(row, 1, new QTableWidgetItem(tmp));
		ui_.vars->setItem(row, 2, new QTableWidgetItem(variant.annotations().at(gene_idx), QTableWidgetItem::Type));
		ui_.vars->setItem(row, 3, new QTableWidgetItem(variant.annotations().at(class_idx), QTableWidgetItem::Type));
		++row;
	}


	//add CNVs
	foreach(int i, settings_.report_config->variantIndices(VariantType::CNVS, true, type()))
	{
		const CopyNumberVariant& cnv = cnvs_[i];
		if (roi_file_!="" && !roi_.overlapsWith(cnv.chr(), cnv.start(), cnv.end())) continue;
		const ReportVariantConfiguration& var_conf = settings_.report_config->get(VariantType::CNVS,i);

		ui_.vars->setRowCount(ui_.vars->rowCount()+1);
		ui_.vars->setItem(row, 0, new QTableWidgetItem(var_conf.report_type + (var_conf.causal ? " (causal)" : "")));
		ui_.vars->setItem(row, 1, new QTableWidgetItem("CNV " + cnv.toStringWithMetaData() + " cn=" + QString::number(cnv.copyNumber(cnvs_.annotationHeaders()))));
		ui_.vars->setItem(row, 2, new QTableWidgetItem(cnv.genes().join(", "), QTableWidgetItem::Type));
		ui_.vars->setItem(row, 3, new QTableWidgetItem(var_conf.classification));
		++row;
	}

	//add SVs
	foreach(int i, settings_.report_config->variantIndices(VariantType::SVS, true, type()))
	{
		const BedpeLine& sv = svs_[i];
		BedFile affected_region = sv.affectedRegion();
		if (roi_file_!="")
		{
			if (sv.type() != StructuralVariantType::BND)
			{
				if (!roi_.overlapsWith(affected_region[0].chr(), affected_region[0].start(), affected_region[0].end())) continue;
			}
			else
			{
				if (!roi_.overlapsWith(affected_region[0].chr(), affected_region[0].start(), affected_region[0].end())
					&& !roi_.overlapsWith(affected_region[1].chr(), affected_region[1].start(), affected_region[1].end())) continue;
			}
		}
		const ReportVariantConfiguration& var_conf = settings_.report_config->get(VariantType::SVS,i);

		QString sv_string = "SV " + affected_region[0].toString(true);
		if (sv.type()==StructuralVariantType::BND) sv_string += " <-> " + affected_region[1].toString(true);
		sv_string += " type=" + BedpeFile::typeToString(sv.type());

		ui_.vars->setRowCount(ui_.vars->rowCount()+1);
		ui_.vars->setItem(row, 0, new QTableWidgetItem(var_conf.report_type + (var_conf.causal ? " (causal)" : "")));
		ui_.vars->setItem(row, 1, new QTableWidgetItem(sv_string));
		ui_.vars->setItem(row, 2, new QTableWidgetItem(sv.genes(svs_.annotationHeaders()).join(", "), QTableWidgetItem::Type));
		ui_.vars->setItem(row, 3, new QTableWidgetItem(var_conf.classification));
		++row;
	}


	//resize table cells
	GUIHelper::resizeTableCells(ui_.vars);

	//enable coverage detail settings only if necessary
	if (roi_file_!="")
	{
		bool add_cov_details = ui_.details_cov->isChecked();
		ui_.min_cov->setEnabled(add_cov_details);
		ui_.min_cov_label->setEnabled(add_cov_details);
		ui_.depth_calc->setEnabled(add_cov_details);
		if (!add_cov_details) ui_.depth_calc->setChecked(false);
		ui_.details_cov_roi->setEnabled(add_cov_details);
		if (!add_cov_details) ui_.details_cov_roi->setChecked(false);
	}

	//buttons
	activateOkButtonIfValid();
}

void ReportDialog::editDiseaseGroupStatus()
{
	QAction* action = qobject_cast<QAction*>(sender());
	QString sample = action->data().toString();
	QString sample_id = db_.sampleId(sample);

	//get disease group/status
	DiseaseInfoWidget* widget = new DiseaseInfoWidget(sample, sample_id, this);
	auto dlg = GUIHelper::createDialog(widget, "Disease information of '" + sample + "'", "", true);
	if (dlg->exec() != QDialog::Accepted) return;

	//update
	db_.setSampleDiseaseData(sample_id, widget->diseaseGroup(), widget->diseaseStatus());
	checkMetaData();
}

void ReportDialog::editDiseaseDetails()
{
	QAction* action = qobject_cast<QAction*>(sender());
	QString sample = action->data().toString();
	QString sample_id = db_.sampleId(sample);

	//get disease details
	SampleDiseaseInfoWidget* widget = new SampleDiseaseInfoWidget(sample, this);
	widget->setDiseaseInfo(db_.getSampleDiseaseInfo(sample_id));
	auto dlg = GUIHelper::createDialog(widget, "Sample disease details", "", true);
	if (dlg->exec() != QDialog::Accepted) return;

	//update
	db_.setSampleDiseaseInfo(sample_id, widget->diseaseInfo());
	checkMetaData();
}

void ReportDialog::writeBackSettings()
{
	settings_.show_coverage_details = ui_.details_cov->isChecked();
	settings_.min_depth = ui_.min_cov->value();
	settings_.roi_low_cov = ui_.details_cov_roi->isChecked();
	settings_.recalculate_avg_depth = ui_.depth_calc->isChecked();
	settings_.show_tool_details = ui_.tool_details->isChecked();
	settings_.show_omim_table = ui_.omim_table->isChecked();
	settings_.show_class_details = ui_.class_info->isChecked();
	settings_.language = ui_.language->currentText();
}

void ReportDialog::activateOkButtonIfValid()
{
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	if (ui_.report_type->currentIndex()==0) return;
	if (!ui_.meta_data_check_output->text().isEmpty()) return;

	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}
