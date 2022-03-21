#include "ReportDialog.h"
#include "GUIHelper.h"
#include "DiseaseInfoWidget.h"
#include "SampleDiseaseInfoWidget.h"
#include "DiagnosticStatusWidget.h"
#include <QTableWidgetItem>
#include <QMenu>


ReportDialog::ReportDialog(QString ps, ReportSettings& settings, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs, const TargetRegionInfo& roi, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, ps_(ps)
	, settings_(settings)
	, variants_(variants)
	, cnvs_(cnvs)
	, svs_(svs)
	, roi_(roi)
{
	ui_.setupUi(this);
	setWindowTitle(windowTitle() + ps);
	connect(ui_.report_type, SIGNAL(currentTextChanged(QString)), this, SLOT(updateVariantTable()));
	connect(ui_.meta_data_check_btn, SIGNAL(clicked(bool)), this, SLOT(checkMetaData()));
	connect(ui_.details_cov, SIGNAL(stateChanged(int)), this, SLOT(updateCoverageCheckboxStatus()));
	connect(this, SIGNAL(accepted()), this, SLOT(writeBackSettings()));

	initGUI();
}

void ReportDialog::checkMetaData()
{
	//clear
	ui_.meta_data_check_output->clear();

	//check
	QString ps_id = db_.processedSampleId(ps_);
	QHash<QString, QStringList> errors = db_.checkMetaData(ps_id, variants_, cnvs_, svs_);

	//sort sample names and make current sample the first one
	QStringList sample_names = errors.keys();
	sample_names.sort();
	QString sample = db_.getSampleData(db_.sampleId(ps_)).name;
	sample_names.removeAll(sample);
	sample_names.prepend(sample);

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
	if (display_messages.count()>0)
	{
		ui_.meta_data_check_output->setText("<font color='red'>" + display_messages.join("<br>") + "</font>");
	}

	//add edit menu entries
	QMenu* menu = new QMenu(this);
	for(int i=0; i<sample_names.count(); ++i)
	{
		QString sample_name = sample_names[i];

		if (i>0) menu->addSeparator();

		QAction* action = menu->addAction(sample_name + ": disease group/status");
		action->setData(sample_name);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(editDiseaseGroupStatus()));

		action = menu->addAction(sample_name + ": disease details (HPO, OMIM, Optha, ...)");
		action->setData(sample_name);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(editDiseaseDetails()));

		if (ps_.startsWith(sample_name)) //only for current sample, because processed sample is needed
		{
			action = menu->addAction(sample_name + ": diagnostic status");
			action->setData(sample_name);
			connect(action, SIGNAL(triggered(bool)), this, SLOT(editDiagnosticStatus()));
		}
	}
	ui_.meta_data_edit_btn->setMenu(menu);

	//update button box
	activateOkButtonIfValid();
}

void ReportDialog::initGUI()
{
	//report types
	ui_.report_type->addItems(ReportVariantConfiguration::getTypeOptions());
	ui_.report_type->setCurrentIndex(0);

	//settings
	ui_.details_cov->setChecked(settings_.show_coverage_details);
	ui_.min_cov->setValue(settings_.min_depth);
	ui_.details_cov_roi->setChecked(settings_.roi_low_cov);
	ui_.depth_calc->setChecked(settings_.recalculate_avg_depth);
	ui_.omim_table->setChecked(settings_.show_omim_table);
	ui_.omim_table_one_only->setChecked(settings_.show_one_entry_in_omim_table);
	ui_.class_info->setChecked(settings_.show_class_details);
	ui_.language->setCurrentText(settings_.language);

	//no ROI > no roi options
	if (!roi_.isValid())
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
		ui_.omim_table_one_only->setChecked(false);
		ui_.omim_table_one_only->setEnabled(false);
	}

	//check box status
	updateCoverageCheckboxStatus();

	//meta data
	checkMetaData();
}

void ReportDialog::updateVariantTable()
{
	//init
	ui_.vars->setRowCount(0);
	int row = 0;

	//add small variants
	int geno_idx = variants_.getSampleHeader().infoByID(ps_).column_index;
	int gene_idx = variants_.annotationIndexByName("gene");
	int class_idx = variants_.annotationIndexByName("classification");
	foreach(int i, settings_.report_config->variantIndices(VariantType::SNVS_INDELS, true, type()))
	{
		const Variant& variant = variants_[i];
		const ReportVariantConfiguration& var_conf = settings_.report_config->get(VariantType::SNVS_INDELS, i);

		bool in_roi = true;
		if (roi_.isValid() && !roi_.regions.overlapsWith(variant.chr(), variant.start(), variant.end())) in_roi = false;

		QByteArray genotype = variant.annotations().at(geno_idx).trimmed();

		ui_.vars->setRowCount(ui_.vars->rowCount()+1);
		addCheckBox(row, 0, in_roi && genotype!="wt", !in_roi)->setData(Qt::UserRole, i);
		addTableItem(row, 1, var_conf.report_type + (var_conf.causal ? " (causal)" : ""));
		addTableItem(row, 2, variantTypeToString(VariantType::SNVS_INDELS));
		addTableItem(row, 3, variant.toString(false, 30) + " (" + genotype + ")");
		addTableItem(row, 4, variant.annotations().at(gene_idx));
		addTableItem(row, 5, variant.annotations().at(class_idx));
		++row;
	}


	//add CNVs
	foreach(int i, settings_.report_config->variantIndices(VariantType::CNVS, true, type()))
	{
		const CopyNumberVariant& cnv = cnvs_[i];
		const ReportVariantConfiguration& var_conf = settings_.report_config->get(VariantType::CNVS,i);

		bool in_roi = true;
		if (roi_.isValid() && !roi_.regions.overlapsWith(cnv.chr(), cnv.start(), cnv.end())) in_roi = false;

		ui_.vars->setRowCount(ui_.vars->rowCount()+1);
		addCheckBox(row, 0, in_roi, !in_roi)->setData(Qt::UserRole, i);
		addTableItem(row, 1, var_conf.report_type + (var_conf.causal ? " (causal)" : ""));
		addTableItem(row, 2, variantTypeToString(VariantType::CNVS));
		addTableItem(row, 3, cnv.toStringWithMetaData() + " cn=" + QString::number(cnv.copyNumber(cnvs_.annotationHeaders())));
		addTableItem(row, 4, cnv.genes().join(", "));
		addTableItem(row, 5, var_conf.classification);
		++row;
	}

	//add SVs
	if (svs_.isValid())
	{
		int sv_format_idx = svs_.annotationIndexByName("FORMAT");
		int sv_sample_idx = sv_format_idx + 1;
		if (svs_.format() == BedpeFileFormat::BEDPE_GERMLINE_MULTI || svs_.format() == BedpeFileFormat::BEDPE_GERMLINE_TRIO)
		{
			try
			{
				sv_sample_idx = svs_.sampleHeaderInfo().infoByStatus(true).column_index;
			}
			catch (...)
			{
				sv_sample_idx = -1;
			}

		}
		foreach(int i, settings_.report_config->variantIndices(VariantType::SVS, true, type()))
		{
			const BedpeLine& sv = svs_[i];
			const ReportVariantConfiguration& var_conf = settings_.report_config->get(VariantType::SVS,i);

			QByteArray genotype;
			if (sv_sample_idx != -1)
			{
				int gt_idx = sv.annotations().at(sv_format_idx).split(':').indexOf("GT");
				if (gt_idx != -1)
				{
					genotype = sv.annotations().at(sv_sample_idx).split(':').at(gt_idx).trimmed();
				}
			}

			bool in_roi = true;
			BedFile affected_region = sv.affectedRegion();
			if (roi_.name!="")
			{
				if (sv.type() != StructuralVariantType::BND)
				{
					if (!roi_.regions.overlapsWith(affected_region[0].chr(), affected_region[0].start(), affected_region[0].end())) in_roi = false;
				}
				else
				{
					if (!roi_.regions.overlapsWith(affected_region[0].chr(), affected_region[0].start(), affected_region[0].end())
						&& !roi_.regions.overlapsWith(affected_region[1].chr(), affected_region[1].start(), affected_region[1].end())) in_roi = false;
				}
			}

			ui_.vars->setRowCount(ui_.vars->rowCount()+1);
			addCheckBox(row, 0, in_roi && genotype!="0/0", !in_roi)->setData(Qt::UserRole, i);
			addTableItem(row, 1, var_conf.report_type + (var_conf.causal ? " (causal)" : ""));
			addTableItem(row, 2, variantTypeToString(VariantType::SVS));
			addTableItem(row, 3, affected_region[0].toString(true) + (sv.type()==StructuralVariantType::BND ? (" <-> " + affected_region[1].toString(true)) : "") + " type=" + BedpeFile::typeToString(sv.type()));
			addTableItem(row, 4, sv.genes(svs_.annotationHeaders()).join(", "));
			addTableItem(row, 5, var_conf.classification);
			++row;
		}
	}

	//resize table cells
	GUIHelper::resizeTableCells(ui_.vars);
}

void ReportDialog::updateCoverageCheckboxStatus()
{
	//enable coverage detail settings only if necessary
	if (roi_.isValid())
	{
		bool add_cov_details = ui_.details_cov->isChecked();
		ui_.min_cov->setEnabled(add_cov_details);
		ui_.min_cov_label->setEnabled(add_cov_details);
		ui_.depth_calc->setEnabled(add_cov_details);
		if (!add_cov_details) ui_.depth_calc->setChecked(false);
		ui_.details_cov_roi->setEnabled(add_cov_details);
		if (!add_cov_details) ui_.details_cov_roi->setChecked(false);
	}

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
	auto dlg = GUIHelper::createDialog(widget, "Sample disease details of '" + sample + "'", "", true);
	if (dlg->exec() != QDialog::Accepted) return;

	//update
	db_.setSampleDiseaseInfo(sample_id, widget->diseaseInfo());
	checkMetaData();
}

void ReportDialog::editDiagnosticStatus()
{
	QString ps_id = db_.processedSampleId(ps_);
	DiagnosticStatusWidget* widget = new DiagnosticStatusWidget(this);
	widget->setStatus(db_.getDiagnosticStatus(ps_id));
	auto dlg = GUIHelper::createDialog(widget, "Diagnostic status of '" + ps_, "'", true);
	if (dlg->exec()!=QDialog::Accepted) return;

	db_.setDiagnosticStatus(ps_id, widget->status());
	checkMetaData();
}

QTableWidgetItem* ReportDialog::addTableItem(int row, int col, QString text)
{
	QTableWidgetItem* item = new QTableWidgetItem();

	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
	item->setText(text);

	ui_.vars->setItem(row, col, item);

	return item;
}

QTableWidgetItem*ReportDialog::addCheckBox(int row, int col, bool is_checked, bool check_state_editable)
{
	QTableWidgetItem* item = new QTableWidgetItem();

	if (is_checked)
	{
		item->setCheckState(Qt::Checked);
	}
	else
	{
		item->setCheckState(Qt::Unchecked);
	}

	if (check_state_editable)
	{
		item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
	}
	else
	{
		item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
		item->setToolTip("Variants inside the target region are always seleted.\nVariants with genotype 'wt' cannot be selected.");
	}

	ui_.vars->setItem(row, col, item);

	return item;
}

void ReportDialog::writeBackSettings()
{
	settings_.selected_variants.clear();
	for (int r=0; r<ui_.vars->rowCount(); ++r)
	{
		if (ui_.vars->item(r, 0)->checkState()!=Qt::Checked) continue;

		VariantType type = stringToVariantType(ui_.vars->item(r, 2)->text());
		int index = ui_.vars->item(r, 0)->data(Qt::UserRole).toInt();
		settings_.selected_variants << qMakePair(type, index);
	}

	settings_.show_coverage_details = ui_.details_cov->isChecked();
	settings_.min_depth = ui_.min_cov->value();
	settings_.roi_low_cov = ui_.details_cov_roi->isChecked();
	settings_.recalculate_avg_depth = ui_.depth_calc->isChecked();
	settings_.show_omim_table = ui_.omim_table->isChecked();
	settings_.show_one_entry_in_omim_table = ui_.omim_table_one_only->isChecked();
	settings_.show_class_details = ui_.class_info->isChecked();
	settings_.language = ui_.language->currentText();
}

void ReportDialog::activateOkButtonIfValid()
{
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	ui_.buttonBox->setToolTip("");

	//meta data ok?
	if (!ui_.meta_data_check_output->text().trimmed().isEmpty())
	{
		ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
		ui_.buttonBox->setToolTip("Correct meta data errors to continue!");
		return;
	}

}
